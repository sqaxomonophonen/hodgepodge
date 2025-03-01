// cc -Wall -std=c99 read_from_tty.c -o read_from_tty

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

char * strerror(int errnum); // hmm

int main(int argc, char** argv)
{
	int e;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s path/to/tty\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	const char* tty_path = argv[1];
	const int fd = open(tty_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1) {
		fprintf(stderr, "%s: %s\n", tty_path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!isatty(fd)) {
		fprintf(stderr, "%s: not a TTY\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}

	e = flock(fd, LOCK_EX | LOCK_NB);
	if (e == -1) {
		fprintf(stderr, "%s: could not lock\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}

	tcflush(fd, TCIOFLUSH);

	struct termios term = {0};
	if (tcgetattr(fd, &term) == -1) {
		fprintf(stderr, "%s: tcgetattr failed\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}

	term.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	term.c_oflag = 0;
	term.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	term.c_cflag &= ~(CSIZE | PARENB);
	term.c_cflag |= CS8;
	term.c_cc[VMIN]  = 1;
	term.c_cc[VTIME] = 0;
	const speed_t baudrate = B115200;
	if (cfsetispeed(&term, baudrate) == -1) {
		fprintf(stderr, "%s: cfsetispeed failed\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}
	if (cfsetospeed(&term, baudrate) == -1) {
		fprintf(stderr, "%s: cfsetospeed failed\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}

	if (tcsetattr(fd, TCSAFLUSH, &term) == -1) {
		fprintf(stderr, "%s: tcsetattr failed\n", tty_path);
		assert(close(fd) == 0);
		exit(EXIT_FAILURE);
	}

	const int N = 1<<5;
	int i = 0;
	char ringbuf[N];
	for (;;) {
		char ch;
		if (read(fd, &ch, sizeof ch) == -1) {
			if (errno == EAGAIN) continue;
			printf("%d / %s\n", errno, strerror(errno));
			assert(close(fd) == 0);
			exit(EXIT_FAILURE);
		}
		const int mask = N-1;
		ringbuf[(i++) & mask] = ch;
		if (ch == '\r') {
			int j;
			int multiplier = 1;
			int value = 0;
			for (j=0; j<6; ++j, multiplier*=10) {
				int ji = i-2-j;
				if (ji < 0) break;
				const char c2 = ringbuf[ji & mask];
				if ('0' <= c2 && c2 <= '9') {
					value += (c2-'0') * multiplier;
					continue;
				} else {
					break;
				}
			}
			if (j > 0) {
				printf("read value [%d]\n", value);
			}
		}
	}

	assert(close(fd) == 0);
	return EXIT_SUCCESS;
}
