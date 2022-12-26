#include "demeter_util.h"
#include "driver.h"
#include "mapping.h"
#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>

unsigned BATCH_SIZE = 3;

unsigned IDX_MAP_SIZE = 27;
unsigned IDX_MAX_OCC  = 200;
unsigned SE_W         = 10;
unsigned SE_K         = 15;
// TODO: don't need to set the previous parameters

FILE *OUTPUT;

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

{0, 0, 0, 0, "Voting:", 0},
{"vt_thresholds", 'd', "FLOAT1,FLOAT2", 0, "[0.2,0.1]", 0},
{"vt_max_nb_locs", 'm', "UINT", 0, "maximum number of potential locations after voting [50]", 0},
{"vt_dist", 'x', "FLOAT,UINT1,UINT2", 0, "[0.05, 200, 1500]", 0},
{0, 0, 0, 0, "Mapping:", 0},
{"ma_secondaries", 's', "UINT", 0, "output at most UINT secondary alignments [5]", 0},
*/

    {0, 0, 0, 0, "Ressources:", 0},
    {"nb_threads", 't', "UINT", 0, "number of CPU threads [4]", 0},
    {"batch_size", 'b', "UINT", 0, "batch size (in reads) processed by each cpu threads [3]", 0},
    {"compute_units", 'u', "UINT", 0, "number of FPGA compute units [7]", 0},
    {0, 0, 0, 0, "Input/Output:", 0},
    {"output", 'o', "OUTPUT_FILE", 0, "output file [stdout]", 0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	arguments *args = state->input;
	switch (key) {
		case 't':
			args->nb_threads = strtoul(arg, NULL, 10);
			break;
		case 'b':
			BATCH_SIZE = strtoul(arg, NULL, 10);
			break;
		case 'o':
			FOPEN(OUTPUT, arg, "w");
			break;
		case 'u':
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

	arguments args = {.nb_cus = 7, .nb_threads = 4};

	OUTPUT = stdout;
	argp_parse(&argp, argc, argv, 0, 0, &args);

	demeter_fpga_init(args.nb_cus, args.binary_file, args.index);
	// TODO: check if we can destroy index
	index_destroy(args.index);

	clock_gettime(CLOCK_MONOTONIC, &init);
	fprintf(stderr, "[INFO] Initialization time %f sec\n",
	        init.tv_sec - start.tv_sec + (init.tv_nsec - start.tv_nsec) / 1000000000.0);
	mapping_run(args.nb_threads);
	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stderr, "[INFO] Total execution time %f sec\n",
	        end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0);

	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	fprintf(stderr, "[INFO] Peak RSS: %f GB\n", r.ru_maxrss / 1048576.0);

	return 0;
}
