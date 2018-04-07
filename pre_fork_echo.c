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

void event(int soc) {
	struct sockaddr_in caddr;
	socklen_t caddrlen;
	int acc;
	char buf[256];

	while(1) {
		caddrlen = sizeof(caddr);
		if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			return;
		}

		read(acc, buf, sizeof(buf));
		write(acc, buf, strlen(buf));
		read(acc, buf, sizeof(buf));

		close(acc);
	}

	close(soc);
	return;
}

int main(int argc, char** argv) {
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	pid_t pid, pids[CONNECTION];
	int soc;

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

	for (int i = 0; i < CONNECTION; i++) {
		pid = fork();
		pids[i] = pid;

		if (pid == 0) {
			event(soc);
			exit(0);
		}
	}

	close(soc);

	for (int i = 0; i < CONNECTION; i++) {
		int status;
		waitpid(pids[i], &status, 0);
	}
	return 0;
}
