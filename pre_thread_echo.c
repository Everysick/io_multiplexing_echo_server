#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define PORT 8080
#define CONNECTION 1000 // upper limit: net.core.somaxconn
#define WORKER 30
#define MUL 50

int thread_cnt;
pthread_mutex_t mut;

void* event(void* arg) {
	struct sockaddr_in caddr;
	socklen_t caddrlen;
	int* soc = (int *)arg;
	char buf[256];
	int acc;

	caddrlen = sizeof(caddr);

	while(1) {
		if ((acc = accept(*soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			pthread_exit(NULL);
			return 0;
		}

		do {
			read(acc, buf, sizeof(buf));
			write(acc, buf, strlen(buf));
		} while (strcmp(buf, "Bye!!") != 0);

		read(acc, buf, sizeof(buf));

		close(acc);
	}

	pthread_exit(NULL);
	return 0;
}

int main(int argc, char** argv) {
	pthread_t th[WORKER];
	struct sockaddr_in saddr;
	socklen_t saddrlen;
	int soc;

	thread_cnt = 0;
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

	for (int i = 0; i < WORKER; i++) {
		pthread_create(&th[i], NULL, &event, &soc);
	}

	for (int i = 0; i < WORKER; i++) {
		pthread_join(th[i], NULL);
	}

	close(soc);
	return 0;
}
