#include "cpu.h"
#include "demeter_util.h"
#include <argp.h>

uint32_t SE_K;
uint32_t SE_W;
uint32_t IDX_MAP_SIZE;

const char *argp_program_version     = "seedfarm_profile 0.1.0";
const char *argp_program_bug_address = "<julien@eudine.fr>";
static char doc[]                    = "Peofiling tool for seedfarm";
static char args_doc[]               = "<pdi.xclbin> <index.sfi> <query.fastq>";

typedef struct {
	unsigned nb_threads;
	unsigned nb_cus;
	uint32_t batch_capacity;
	index_t index;
	const char *binary_file;
} arguments;

static struct argp_option options[] = {{"nb_threads", 't', "UINT", 0, "number of CPU threads [1]", 0},
                                       {"nb_cus", 'c', "UINT", 0, "number of Compute Units [1]", 0},
                                       {"batch_capacity", 'b', "UINT", 0, "batch_capacity [33554432]", 0},
                                       {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments *args = state->input;
	switch (key) {
		case 't':
			args->nb_threads = strtoul(arg, NULL, 10);
			break;
		case 'c':
			args->nb_cus = strtoul(arg, NULL, 10);
			break;
		case ARGP_KEY_ARG:
			switch (state->arg_num) {
				case 0:
					args->binary_file = arg;
					break;
				case 1:
					args->index = index_parse(arg);
					break;
				case 2:
					fa_open(OPEN_MMAP, arg);
					break;
				default:
					argp_usage(state);
					break;
			}
			break;
		case ARGP_KEY_END:
			if (state->arg_num != 3) argp_usage(state);
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

	arguments args = {.nb_threads = 1, .nb_cus = 1, .batch_capacity = 33554432};
	argp_parse(&argp, argc, argv, 0, 0, &args);

	read_buf_t read_buf;
	read_buf_init(&read_buf, args.batch_capacity);
	SE_K         = args.index.k;
	SE_W         = args.index.w;
	IDX_MAP_SIZE = args.index.map_size;

	while (fa_parse(&read_buf) == 0) {
		struct timespec start_exec, end_exec;
		clock_gettime(CLOCK_MONOTONIC, &start_exec);
		cpu_pipeline(read_buf, args.nb_threads, args.index);
		clock_gettime(CLOCK_MONOTONIC, &end_exec);
		fprintf(stderr, "execution time %f sec\n",
		        end_exec.tv_sec - start_exec.tv_sec + (end_exec.tv_nsec - start_exec.tv_nsec) / 1000000000.0);
	}

	read_buf_destroy(read_buf);

	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "[SFM_PROF] Total execution time %f sec\n",
	        end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);
}
