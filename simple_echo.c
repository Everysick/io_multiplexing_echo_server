#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define PORT 8080

int main(int argc, char** argv) {
	struct sockaddr_in saddr, caddr;
	socklen_t saddrlen, caddrlen;
	int soc, acc;
	char buf[1000];

	memset(buf, 0, sizeof(buf));

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

	if (listen(soc, 10) == -1) {
		fprintf(stderr, "Cannot listen socket\n");
		return 1;
	}

	while(1) {
		caddrlen = sizeof(caddr);
		if ((acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			return 1;
		}

		read(acc, buf, sizeof(buf));
		write(acc, buf, strlen(buf));

		close(acc);
	}

	close(soc);
	return 0;
}
