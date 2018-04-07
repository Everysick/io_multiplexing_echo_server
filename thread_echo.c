#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#define PORT 8080
#define CONNECTION 100

int thread_cnt;
pthread_mutex_t mut;

void* event(void* arg) {
	char buf[256];
	int* fd = (int *)arg;

	read(*fd, buf, sizeof(buf));
	write(*fd, buf, strlen(buf));
	read(*fd, buf, sizeof(buf));

	close(*fd);
	free(fd);

	pthread_mutex_lock(&mut);
	thread_cnt--;
	pthread_mutex_unlock(&mut);

	return 0;
}

int main(int argc, char** argv) {
	pthread_t th[CONNECTION];
	struct sockaddr_in saddr, caddr;
	socklen_t saddrlen, caddrlen;
	int soc;

	thread_cnt = 0;
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

	while(1) {
		int* acc = malloc(sizeof(int));

		caddrlen = sizeof(caddr);
		if ((*acc = accept(soc, (struct sockaddr *)&caddr, &caddrlen)) == -1) {
			fprintf(stderr, "Accept failed\n");
			return 1;
		}

		pthread_mutex_lock(&mut);
		if (thread_cnt < CONNECTION) {
			thread_cnt++;
			pthread_create(&th[thread_cnt - 1], NULL, &event, acc);
		} else {
			fprintf(stderr, "Too many connection\n");
			close(*acc);
			free(acc);
		}
		pthread_mutex_unlock(&mut);
	}

	close(soc);
	return 0;
}