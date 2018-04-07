#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>

#include <netinet/in.h>

#define PORT 8080
#define CONNECTION 100

void event(int acc) {
	char buf[256];

	read(acc, buf, sizeof(buf));
	write(acc, buf, strlen(buf));
	read(acc, buf, sizeof(buf));

	close(acc);

	return;
}

int main(int argc, char** argv) {
	struct sockaddr_in saddr, caddr;
	socklen_t saddrlen, caddrlen;
	int acc, soc, status, process_cnt = 0;
	pid_t pid;

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

	while(1) {
		caddrlen = sizeof(caddr);
		if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			return 1;
		}

		if (process_cnt >= CONNECTION) {
			fprintf(stderr, "Too many connections\n");
			break;
		}

		pid = fork();

		if (pid == 0) {
			event(acc);
			exit(0);
		} else if (pid == -1) {
			fprintf(stderr, "Fork failed\n");
			break;
		}

		// fixme
		process_cnt++;
	}

	close(soc);

	while(1) {
		pid = wait(&status);
		if (pid == -1 && errno == ECHILD) break;
	}

	return 0;
}
