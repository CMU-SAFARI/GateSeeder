#define _XOPEN_SOURCE 600
#include "demeter_util.h"
#include <fcntl.h>
int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		int fd;
		OPEN(fd, argv[i], O_RDONLY);
		if (fdatasync(fd) == -1) {
			err(1, "%s:%d, fdatasync", __FILE__, __LINE__);
		}
		if (posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) != 0) {
			errx(1, "%s:%d, posix_fadvise", __FILE__, __LINE__);
		}
		CLOSE(fd);
	}
	return 0;
}
