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
#define CONNECTION 1000
#define WORKER 10

int recv_fd(int server) {
	pid_t message;

	struct msghdr msg;
	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	iov.iov_base = &message;
	iov.iov_len = sizeof(message);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = MSG_WAITALL;

	if (recvmsg(server, &msg, 0) < 0) {
		return -1;
	}

	struct cmsghdr *cmsg = (struct cmsghdr*)cmsgbuf;
	return *((int *)CMSG_DATA(cmsg));
}

int send_fd(int client, int fd) {
	pid_t message = getpid();

	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	iov.iov_base = &message;
	iov.iov_len = sizeof(message);

	struct cmsghdr *cmsg = (struct cmsghdr*)cmsgbuf;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsg)) = fd;

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	if (sendmsg(client, &msg, 0) < 0) {
		return -1;
	}

	return 0;
}

static int epoll_mask = EPOLLONESHOT | EPOLLET;
static char reply[256] = "Reply";

void event_loop(int pfd) {
	struct epoll_event current_ev, ev;
	struct epoll_event events[MAX_EVENTS];
	char buf[256];
	int acc, epfd, nfd = 0;
	//pid_t self = getpid();

	if ((epfd = epoll_create1(0)) == -1) {
		fprintf(stderr, "epoll create failed\n");
	}

	ev.data.fd = pfd;
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

				if (current_ev.data.fd == pfd) {
					if ((acc = recv_fd(pfd)) == -1) {
						fprintf(stderr, "Recv failed\n");
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

	close(pfd);
	return;
}

int main(int argc, char** argv) {
	pid_t pids[WORKER];
	struct sockaddr_in saddr, caddr;
	socklen_t saddrlen, caddrlen;
	int acc, soc, current_client = 0, cfds[WORKER];

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

	for (int i = 0; i < WORKER; i++) {
		int fds[2];

		socketpair(AF_LOCAL, SOCK_DGRAM, 0, fds);

		cfds[i] = fds[0];
		pids[i] = fork();

		if (pids[i] == 0) {
			for (int j = 0; j < i; j++) {
				close(cfds[j]);
			}

			close(fds[0]);
			event_loop(fds[1]);
			exit(0);
		}

		close(fds[1]);
	}

	while (1) {
		if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			return 0;
		}

		if (send_fd(cfds[current_client], acc) == -1) {
			fprintf(stderr, "Send failed\n");
			return 0;
		}

		current_client = (current_client + 1) % WORKER;
		close(acc);
	}

	for (int i = 0; i < WORKER; i++) {
		int status;
		waitpid(pids[i], &status, 0);
	}

	return 0;
}
