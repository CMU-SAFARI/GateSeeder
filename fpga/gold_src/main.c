#include "err.h"
#include "extraction.h"
#include "parsing.h"
#include "util.h"
#include <fcntl.h>

unsigned SE_W        = 12;
unsigned SE_K        = 15;
unsigned IDX_B       = 26;
unsigned IDX_MAX_OCC = 500;
unsigned MS_SIZE     = 1 << 28;
unsigned BATCH_SIZE  = 200;

int main(int argc, char *argv[]) {
	if (argc != 4) {
		errx(1, "[ERROR] Wrong number of arguments");
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		err(1, "open %s", argv[1]);
	}
	open_reads(fd);

	read_v reads;
	MALLOC(reads.reads, read_t, BATCH_SIZE);

	parse_reads(&reads);

	extraction_init();

	seed_v minimizers = {.capacity = 1 << 10};
	MALLOC(minimizers.seeds, seed_t, 1 << 10);

	for (unsigned i = 0; i < reads.nb_reads; i++) {
		extract_seeds_mapping(reads.reads[i].seq, reads.reads[i].len, &minimizers, 0);
		for (unsigned j = 0; j < minimizers.len; j++) {
			printf("hash: %010lx, loc: %u\n", minimizers.seeds[j].hash, minimizers.seeds[j].loc);
		}
	}

	return 0;
}
