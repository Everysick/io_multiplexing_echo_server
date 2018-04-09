#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 1
#define CONNECTION 100
#define WORKER 10

static char reply[256] = "Reply";
pthread_mutex_t mut;
int epfd;

static int epoll_opt = EPOLLONESHOT | EPOLLET;

pid_t gettid(void)
{
	return syscall(SYS_gettid);
}

void* event_loop() {
	struct epoll_event current_ev;
	struct epoll_event events[MAX_EVENTS];
	char buf[256];
	int nfd = 0;
	//pid_t self = gettid();

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
			for (int i = 0; i < nfd; i++) {
				//epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
			}

			break;
		}
		pthread_mutex_unlock(&mut);

		for (int i = 0; i < nfd; i++) {
			current_ev = events[i];

			if (current_ev.events & EPOLLIN) {
				read(current_ev.data.fd, buf, sizeof(buf));
				//printf("[%d] Read from %d: %s\n", (int)self, current_ev.data.fd, buf);

				if (strcmp("Hello", buf) == 0) {
					current_ev.events = epoll_opt | EPOLLOUT;
					epoll_ctl(epfd, EPOLL_CTL_MOD, current_ev.data.fd, &current_ev);
				} else {
					//printf("[%d] Close: %d\n", (int)self, current_ev.data.fd);
					close(current_ev.data.fd);
				}
			} else if (current_ev.events & EPOLLOUT) {
				//printf("[%d] Write to %d: %s\n", (int)self, current_ev.data.fd, reply);

				write(current_ev.data.fd, reply, strlen(reply));

				current_ev.events = epoll_opt | EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_MOD, current_ev.data.fd, &current_ev);
			}
		}

	}

	pthread_exit(NULL);
	return 0;
}

int main(int argc, char** argv) {
	pthread_t th[WORKER];
	struct sockaddr_in saddr, caddr;
	struct epoll_event ev;
	socklen_t saddrlen, caddrlen;
	int soc, acc;

	pthread_mutex_init(&mut, NULL);

	if ((soc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Cannot make socket\n");
		return 1;
	}

	int on = 1;
	setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

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

	/* ev.data.fd = soc; */
	/* ev.events = EPOLLIN; */

	/* if (epoll_ctl(epfd, EPOLL_CTL_ADD, soc, &ev) == -1) { */
	/* 	fprintf(stderr, "epoll create failed\n"); */
	/* } */

	for (int i = 0; i < WORKER; i++) {
		pthread_create(&th[i], NULL, &event_loop, NULL);
	}

	while(1) {
		if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			pthread_exit(NULL);
			return 0;
		}

		ev.data.fd = acc;
		ev.events = epoll_opt | EPOLLIN;

		//printf("Hello %d\n", acc);

		epoll_ctl(epfd, EPOLL_CTL_ADD, acc, &ev);
	}

	for (int i = 0; i < WORKER; i++) {
		pthread_join(th[i], NULL);
	}

	close(soc);
	return 0;
}
