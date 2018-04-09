#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 1
#define CONNECTION 100
#define WORKER 10

static char reply[256] = "Reply";

void event_loop(int soc) {
	struct sockaddr_in caddr;
	socklen_t caddrlen;
	struct epoll_event current_ev, ev;
	struct epoll_event events[MAX_EVENTS];
	char buf[256];
	int acc, epfd, nfd = 0;
	//pid_t self = getpid();

	if ((epfd = epoll_create1(0)) == -1) {
		fprintf(stderr, "epoll create failed\n");
	}

	ev.data.fd = soc;
	ev.events = EPOLLIN;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, soc, &ev) == -1) {
		fprintf(stderr, "epoll add failed\n");
	}

	while(1) {
		switch(nfd = epoll_wait(epfd, events, MAX_EVENTS, -1)) {
		case -1:
			fprintf(stderr, "epoll failed\n");
			break;
		case 0:
			fprintf(stderr, "epoll Timeout\n");
			break;
		default:
			for (int i = 0; i < nfd; i++) {
				current_ev = events[i];

				if (current_ev.data.fd == soc) {
					if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
						fprintf(stderr, "Accept failed\n");
						close(soc);
						return;
					}

					//printf("Hello %d\n", acc);

					ev.data.fd = acc;
					ev.events = EPOLLIN | EPOLLOUT;

					epoll_ctl(epfd, EPOLL_CTL_ADD, acc, &ev);
				} else if (current_ev.events & EPOLLIN) {
					read(current_ev.data.fd, buf, sizeof(buf));
					//printf("[%d] Read from %d: %s\n", (int)self, current_ev.data.fd, buf);

					if (strcmp("Hello", buf) != 0) {
						//printf("[%d] Close: %d\n", (int)self, current_ev.data.fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, current_ev.data.fd, NULL);
						close(current_ev.data.fd);
					}
				} else if (current_ev.events & EPOLLOUT) {
					//printf("[%d] Write to %d: %s\n", (int)self, current_ev.data.fd, reply);
					write(current_ev.data.fd, reply, strlen(reply));
				}
			}
			break;
		}
	}

	close(soc);
	return;
}

int main(int argc, char** argv) {
	pid_t pids[CONNECTION];
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	int soc;

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

	for (int i = 0; i < WORKER; i++) {
		pids[i] = fork();

		if (pids[i] == 0) {
			event_loop(soc);
			exit(0);
		}
	}

	close(soc);

	for (int i = 0; i < WORKER; i++) {
		int status;
		waitpid(pids[i], &status, 0);
	}

	return 0;
}
