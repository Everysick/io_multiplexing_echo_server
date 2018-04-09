#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 1
#define CONNECTION 100

static int epoll_mask = EPOLLONESHOT | EPOLLET;

int main(int argc, char** argv) {
	struct epoll_event current_ev, ev, events[MAX_EVENTS];
	static char reply[256] = "Reply";
	char buf[256];
	struct sockaddr_in saddr, caddr;
	socklen_t saddrlen, caddrlen;
	int acc, soc, epfd, nfd;

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
	caddrlen = sizeof(caddr);

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

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1) {
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
						return 0;
					}

					current_ev.data.fd = acc;
					current_ev.events = EPOLLIN | epoll_mask;

					epoll_ctl(epfd, EPOLL_CTL_ADD, acc, &current_ev);
				} else if (current_ev.events & EPOLLIN) {
					read(current_ev.data.fd, buf, sizeof(buf));

					if (strcmp("Hello", buf) == 0) {
						current_ev.events = EPOLLOUT | epoll_mask;
						epoll_ctl(epfd, EPOLL_CTL_MOD, current_ev.data.fd, &current_ev);
					} else {
						epoll_ctl(epfd, EPOLL_CTL_DEL, current_ev.data.fd, NULL);
						close(current_ev.data.fd);
					}
				} else if (current_ev.events & EPOLLOUT) {
					write(current_ev.data.fd, reply, strlen(reply));

					current_ev.events = EPOLLIN | epoll_mask;
					epoll_ctl(epfd, EPOLL_CTL_MOD, current_ev.data.fd, &current_ev);
				}
			}
			break;
		}
	}

	close(soc);
	return 0;
}
