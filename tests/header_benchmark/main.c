
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#include <webserver.h>

void reCopyHeaderBuffer(socket_info* sock, unsigned int end);

#define ITERATIONS 1000000

int main(int argc, char** argv) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <http-request-file> [iterations]\n", argv[0]);
		return 1;
	}

	int iterations = ITERATIONS;
	if (argc >= 3) {
		iterations = atoi(argv[2]);
		if (iterations <= 0) iterations = ITERATIONS;
	}

	/* Read input file */
	struct stat st;
	if (stat(argv[1], &st) != 0) {
		perror("stat");
		return 1;
	}
	int file_size = st.st_size;

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	char *file_buf = malloc(file_size + 1);
	int bytes_read = read(fd, file_buf, file_size);
	close(fd);
	if (bytes_read != file_size) {
		fprintf(stderr, "Short read\n");
		return 1;
	}
	file_buf[file_size] = '\0';

	printf("Input:   %s (%d bytes)\n", argv[1], file_size);
	printf("Iterations: %d\n\n", iterations);

	struct timespec t_start, t_end;
	clock_gettime(CLOCK_MONOTONIC, &t_start);

	for (int iter = 0; iter < iterations; iter++) {
		socket_info sock;
		HttpRequestHeader *header = WebserverMallocHttpRequestHeader();

		memset(&sock, 0, sizeof(socket_info));
		sock.header = header;
		sock.header_buffer = malloc(100000);
		sock.header_buffer_size = 100000;
		memset(sock.header_buffer, 0, 100000);
		memcpy(sock.header_buffer, file_buf, file_size);
		sock.header_buffer_pos = file_size;

		unsigned int bytes_parsed;
		while (sock.header_buffer_pos > 0) {
			int ret = ParseHeader(&sock, header, sock.header_buffer, sock.header_buffer_pos, &bytes_parsed);
			if (ret > 0) {
				reCopyHeaderBuffer(&sock, bytes_parsed);
				continue;
			}
			break;
		}

		free(sock.header_buffer);
		WebserverFreeHttpRequestHeader(header);
	}

	clock_gettime(CLOCK_MONOTONIC, &t_end);

	double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
	double ns_per_iter = (elapsed * 1e9) / iterations;
	double iters_per_sec = iterations / elapsed;

	printf("Results:\n");
	printf("  Total time:   %.3f s\n", elapsed);
	printf("  Per iteration: %.0f ns\n", ns_per_iter);
	printf("  Throughput:   %.0f iterations/s\n", iters_per_sec);

	free(file_buf);
	return 0;
}
