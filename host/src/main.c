#include "demeter_util.h"
#include "driver.h"
#include "formating.h"
#include "mapping.h"
#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

uint32_t SE_K;
uint32_t BATCH_CAPACITY   = 16777216;
uint32_t MAX_NB_MAPPING   = 3;
uint32_t VT_DISTANCE      = 300;
float VT_THRESHOLD_FRAC   = 0.02;
uint32_t VT_THRESHOLD_MAX = 20;

// TODO: try with cache flag
// TODO: Evaluate the impact of the size of the FIFOs in the device

FILE *OUTPUT;
target_t TARGET;

typedef struct {
	unsigned nb_cus;
	index_t index;
	const char *binary_file;
	unsigned nb_threads;
} arguments;

const char *argp_program_version     = "demeter 0.1.0";
const char *argp_program_bug_address = "<julien@eudine.fr>";
static char doc[]                    = "FPGA + HBM based read mapper";
static char args_doc[]               = "<pdi.xclbin> <index.dti> <query.fastq>";
static struct argp_option options[]  = {
     /*
{0, 0, 0, 0, "Sorting:", 0},
{"radix_sort", 'R', 0, 0, "use radix sort instead of merge sort", 0},
*/

    {0, 0, 0, 0, "Mapping:", 0},
    {"max_nb_mapping", 'm', "UINT", 0, "[3]", 0},
    {"vt_distance", 'd', "UINT", 0, "[300]", 0},
    {"vt_threshold", 'h', "FLOAT[,UINT]", 0, "[0.02,20]", 0},

    {0, 0, 0, 0, "Ressources:", 0},
    {"nb_threads", 't', "UINT", 0, "number of CPU threads [4]", 0},
    {"batch_capacity", 'b', "UINT", 0, "batch capacity (in bases) [16777216]", 0},
    {"compute_units", 'u', "UINT", 0, "number of FPGA compute units [8]", 0},

    {0, 0, 0, 0, "Output:", 0},
    {"output", 'o', "OUTPUT_FILE", 0, "output file [stdout]", 0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments *args = state->input;
	switch (key) {
		case 'm':
			MAX_NB_MAPPING = strtoul(arg, NULL, 10);
			break;
		case 'd':
			VT_DISTANCE = strtoul(arg, NULL, 10);
			break;
		case 'h': {
			char *p;
			VT_THRESHOLD_FRAC = strtod(arg, &p);
			if (*p == ',') {
				VT_THRESHOLD_MAX = strtoul(p + 1, NULL, 10);
			}
		} break;
		case 't':
			args->nb_threads = strtoul(arg, NULL, 10);
			break;
		case 'b':
			BATCH_CAPACITY = strtoul(arg, NULL, 10);
			break;
		case 'u':
			args->nb_cus = strtoul(arg, NULL, 10);
			break;
		case 'o':
			FOPEN(OUTPUT, arg, "w");
			break;
		case ARGP_KEY_ARG:
			switch (state->arg_num) {
				case 0:
					args->binary_file = arg;
					break;
				case 1:
					args->index = index_parse(arg);
					// puts("INDEX PARSED");
					break;
				case 2:
					fastq_open(OPEN_MMAP, arg);
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
	struct timespec start, init, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	arguments args = {.nb_cus = 8, .nb_threads = 4};

	OUTPUT = stdout;
	argp_parse(&argp, argc, argv, 0, 0, &args);

	demeter_fpga_init(args.nb_cus, args.binary_file, args.index);
	index_destroy_key_map(args.index);

	TARGET =
	    (target_t){.nb_seq = args.index.nb_seq, .seq_name = args.index.seq_name, .seq_len = args.index.seq_len};
	fprintf(stderr, "[INFO] w: %u, k: %u, map_size: %u, max_occ: %u\n", args.index.w, args.index.k,
	        args.index.map_size, args.index.max_occ);

	SE_K = args.index.k;

	clock_gettime(CLOCK_MONOTONIC, &init);
	fprintf(stderr, "[INFO] Initialization time %f sec\n",
	        init.tv_sec - start.tv_sec + (init.tv_nsec - start.tv_nsec) / 1000000000.0);
	mapping_run(args.nb_threads);
	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "[INFO] Total execution time %f sec\n",
	        end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

	paf_write_destroy();
	index_destroy_target(args.index);
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	fprintf(stderr, "[INFO] Peak RSS: %f GB\n", r.ru_maxrss / 1048576.0);

	return 0;
}
