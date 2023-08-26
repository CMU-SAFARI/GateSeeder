#include "GateSeeder_util.h"
#include "cpu.h"
#include "fpga.h"
#include <argp.h>

uint32_t SE_K;
uint32_t SE_W;
uint32_t IDX_MAP_SIZE;

const char *argp_program_version     = "seedfarm_profile 0.1.0";
const char *argp_program_bug_address = "<julien@eudine.fr>";
static char doc[]                    = "Profiling tool for seedfarm";
static char args_doc[]               = "<index.sfi> <query.fastq>";

typedef struct {
	unsigned nb_threads_cus;
	uint32_t batch_capacity;
	index_t index;
	const char *binary_file;
} arguments;

static struct argp_option options[] = {{"fpga", 'f', "XCLBIN", 0, "FPGA mode", 0},
                                       {"nb_threads_cus", 't', "UINT", 0, "number of CPU threads [1]", 0},
                                       {"batch_capacity", 'b', "UINT", 0, "batch_capacity [33554432]", 0},
                                       {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments *args = state->input;
	switch (key) {
		case 'f':
			args->binary_file = arg;
			break;
		case 't':
			args->nb_threads_cus = strtoul(arg, NULL, 10);
			break;
		case 'b':
			args->batch_capacity = strtoul(arg, NULL, 10);
			break;
		case ARGP_KEY_ARG:
			switch (state->arg_num) {
				case 0:
					args->index = index_parse(arg);
					break;
				case 1:
					fa_open(OPEN_MMAP, arg);
					break;
				default:
					argp_usage(state);
					break;
			}
			break;
		case ARGP_KEY_END:
			if (state->arg_num != 2) argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};
int main(int argc, char *argv[]) {
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	arguments args = {.nb_threads_cus = 1, .batch_capacity = 33554432, .binary_file = NULL};
	argp_parse(&argp, argc, argv, 0, 0, &args);

	read_buf_t read_buf;
	SE_K         = args.index.k;
	SE_W         = args.index.w;
	IDX_MAP_SIZE = args.index.map_size;
	if (args.binary_file) {
		fpga_init(args.nb_threads_cus, args.batch_capacity, args.binary_file, args.index);
		while (fpga_pipeline() == 0)
			;
		fpga_destroy();
	} else {
		read_buf_init(&read_buf, args.batch_capacity);
		while (fa_parse(&read_buf) == 0) {
			cpu_pipeline(read_buf, args.nb_threads_cus, args.index);
		}
		read_buf_destroy(read_buf);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "[SFM_PROF] Total execution time %f sec\n",
	        end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);
}
