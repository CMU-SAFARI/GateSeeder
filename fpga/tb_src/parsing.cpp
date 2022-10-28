
void parse_fastq(int fd, uint8_t *out) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	read_file_len = statbuf.st_size;
	read_file_ptr =
	    (uint8_t *)mmap(NULL, read_file_len, PROT_READ, MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fd, 0);
	if (read_file_ptr == MAP_FAILED) {
		err(1, "%s:%d, mmap", __FILE__, __LINE__);
	}
}
