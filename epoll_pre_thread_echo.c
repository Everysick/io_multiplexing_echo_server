#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 1
#define CONNECTION 100

static char reply[256] = "Reply";
struct epoll_event events[MAX_EVENTS];
pthread_mutex_t mut;
int epfd;

void* event_loop(void* arg) {
	struct epoll_event current_ev;
	struct sockaddr_in caddr;
	socklen_t caddrlen;
	int* soc = (int *)arg;
	char buf[256];
	int acc, nfd;

	caddrlen = sizeof(caddr);

	while(1) {
		pthread_mutex_lock(&mut);
		switch(nfd = epoll_wait(epfd, events, MAX_EVENTS, -1)) {
		case -1:
			fprintf(stderr, "Accept failed\n");
			pthread_exit(NULL);
			break;
		case 0:
			fprintf(stderr, "epoll Timeout\n");
			break;
		default:
			current_ev = events[0];
			epoll_ctl(epfd, EPOLL_CTL_DEL, current_ev.data.fd, NULL);
			break;
		}
		pthread_mutex_unlock(&mut);

		if (current_ev.data.fd == *soc) {

			if ((acc = accept(*soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
				fprintf(stderr, "Accept failed\n");
				pthread_exit(NULL);
				return 0;
			}

			epoll_ctl(epfd, EPOLL_CTL_ADD, *soc, &current_ev);

			current_ev.data.fd = acc;
			current_ev.events = EPOLLIN;

			epoll_ctl(epfd, EPOLL_CTL_ADD, acc, &current_ev);
		} else {
			if (current_ev.events & EPOLLIN) {
				read(current_ev.data.fd, buf, sizeof(buf));

				if (strcmp("Hello", buf) == 0) {
					current_ev.events = EPOLLOUT;
					epoll_ctl(epfd, EPOLL_CTL_ADD, current_ev.data.fd, &current_ev);
				} else {
					close(current_ev.data.fd);
				}
			} else if (current_ev.events & EPOLLOUT) {
				write(current_ev.data.fd, reply, strlen(reply));

				current_ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_ADD, current_ev.data.fd, &current_ev);
			}
		}
	}

	pthread_exit(NULL);
	return 0;
}

int main(int argc, char** argv) {
	pthread_t th[CONNECTION];
	struct sockaddr_in saddr;
	struct epoll_event ev;
	socklen_t saddrlen;
	int soc;

	pthread_mutex_init(&mut, NULL);

	if ((soc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Cannot make socket\n");
		return 1;
	}

	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;

	saddrlen = sizeof(saddr);
	if (bind(soc, (struct sockaddr *)&saddr, saddrlen) == -1) {
		fprintf(stderr, "Cannot bind socket\n");
		return 1;
	}

	if (listen(soc, CONNECTION) == -1) {
		fprintf(stderr, "Cannot listen socket\n");
		return 1;
	}

	if ((epfd = epoll_create1(0)) == -1) {
		fprintf(stderr, "epoll create failed\n");
	}

	ev.data.fd = soc;
	ev.events = EPOLLIN;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, soc, &ev) == -1) {
		fprintf(stderr, "epoll create failed\n");
	}

	for (int i = 0; i < CONNECTION; i++) {
		pthread_create(&th[i], NULL, &event_loop, &soc);
	}

	for (int i = 0; i < CONNECTION; i++) {
		pthread_join(th[i], NULL);
	}

	close(soc);
	return 0;
}
