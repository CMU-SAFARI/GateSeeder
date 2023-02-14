#ifdef CPU_EX
#include "demeter_util.h"
#include "driver.h"
#include <string.h>

extern unsigned SE_K;
extern unsigned SE_W;

typedef struct {
	uint64_t hash; // size: 2*K
	uint32_t loc;
	unsigned str : 1;
	unsigned EOR : 1;
} seed_t;

typedef struct {
	uint32_t capacity;
	uint32_t len;
	seed_t *a;
} seed_v;

static int push_seed(seed_v *const minimizers, const seed_t minimizer) {
	if (minimizers->capacity == minimizers->len) {
		minimizers->capacity = (minimizers->capacity == 0) ? 1 : minimizers->capacity << 1;
		REALLOC(minimizers->a, seed_t, minimizers->capacity);
	}

	// printf("hash: %lu\n", minimizer.hash);
	minimizers->a[minimizers->len] = minimizer;
	minimizers->len++;
	return 0;
}

static inline uint64_t hash64(uint64_t key, const uint64_t mask) {
	key = (~key + (key << 21)) & mask; // key = (key << 21) - key - 1;
	key = key ^ key >> 24;
	key = ((key + (key << 3)) + (key << 8)) & mask; // key * 265
	key = key ^ key >> 14;
	key = ((key + (key << 2)) + (key << 4)) & mask; // key * 21
	key = key ^ key >> 28;
	key = (key + (key << 31)) & mask;
	return key;
}

#define ENCODE(c) (c & 0x0f) >> 1
#define END_OF_READ_BASE 'E'

static void extract_seeds(const uint8_t *seq, const uint32_t len, seed_v *const minimizers) {
	uint64_t kmer[2] = {0, 0};
	seed_t buf[256];
	unsigned int l = 0; // l counts the number of bases and is reset to 0 each
	                    // time there is an ambiguous base

	uint32_t location = 0;
	unsigned buf_pos  = 0;
	unsigned min_pos  = 0;
	seed_t minimizer  = {.hash = UINT64_MAX, .loc = 0, .str = 0, .EOR = 0};
	unsigned SHIFT    = 2 * (SE_K - 1);
	uint64_t MASK     = (1ULL << 2 * SE_K) - 1;

	for (uint32_t i = 0; i < len; i++) {
		if (seq[i] == END_OF_READ_BASE) {
			if (l >= SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
				push_seed(minimizers, minimizer);
			}
			l           = 0;
			location    = 0;
			buf_pos     = 0;
			min_pos     = 0;
			minimizer   = (seed_t){.hash = UINT64_MAX, .loc = 0, .str = 0, .EOR = 0};
			seed_t seed = {.hash = 0, .loc = 0, .str = 0, .EOR = 1};
			push_seed(minimizers, seed);
			continue;
		}

		uint8_t c           = ENCODE(seq[i]);
		seed_t current_seed = {.hash = UINT64_MAX, .loc = 0, .str = 0, .EOR = 0};
		if (c < 4) {                                            // not an ambiguous base
			kmer[0] = (kmer[0] << 2 | c) & MASK;            // forward k-mer
			kmer[1] = (kmer[1] >> 2) | (2ULL ^ c) << SHIFT; // reverse k-mer
			l++;
			if (l >= SE_K) {
				// skip "symmetric k-mers"
				if (kmer[0] == kmer[1]) {
					current_seed.hash = UINT64_MAX;
				} else {
					unsigned char z   = kmer[0] > kmer[1]; // strand
					current_seed.hash = hash64(kmer[z], MASK);
					current_seed.loc  = location;
					current_seed.str  = z;
				}
			}
		} else {
			if (l >= SE_W + SE_K - 1 && minimizer.hash != UINT64_MAX) {
				push_seed(minimizers, minimizer);
			}
			l = 0;
		}
		buf[buf_pos] = current_seed;

		if (current_seed.hash < minimizer.hash) {
			if (l >= SE_W + SE_K && minimizer.hash != UINT64_MAX) {
				push_seed(minimizers, minimizer);
			}
			minimizer = current_seed;
			min_pos   = buf_pos;

		} else if (buf_pos == min_pos) {
			if (l >= SE_W + SE_K - 1) {
				push_seed(minimizers, minimizer);
			}
			minimizer.hash = UINT64_MAX;
			for (unsigned j = buf_pos + 1; j < SE_W; j++) {
				if (minimizer.hash > buf[j].hash) {
					minimizer = buf[j];
					min_pos   = j;
				}
			}
			for (unsigned j = 0; j <= buf_pos; j++) {
				if (minimizer.hash > buf[j].hash) {
					minimizer = buf[j];
					min_pos   = j;
				}
			}
		}
		buf_pos = (buf_pos == SE_W - 1) ? 0 : buf_pos + 1;
		location++;
	}
	seed_t seed = {.hash = 0, .loc = 0, .str = 1, .EOR = 1};
	push_seed(minimizers, seed);
}

typedef struct {
	uint32_t capacity;
	uint32_t len;
	uint64_t *a;
} location_v;

extern uint32_t IDX_MAP_SIZE;
#define BUCKET_MASK ((1 << IDX_MAP_SIZE) - 1)
#define LOC_SHIFT 42
#define LOC_OFFSET (1 << 21)

static inline void push_loc(const uint64_t location, location_v *const locations) {
	if (locations->capacity == locations->len) {
		locations->capacity = (locations->capacity == 0) ? 1 : locations->capacity << 1;
		REALLOC(locations->a, uint64_t, locations->capacity);
	}
	locations->a[locations->len] = location;
	locations->len++;
}

static inline uint64_t key_2_loc(const uint64_t key, const unsigned query_str, const uint32_t query_loc) {
	// Extract the target_loc, the chrom_id, the str and the seed_id from the key
	const uint64_t key_target_loc = key & UINT32_MAX;
	const uint64_t chrom_id       = (key >> 32) & ((1 << 10) - 1);
	const uint64_t str            = ((key >> LOC_SHIFT) & 1) ^ query_str;

	// Compute the shifted target_loc
	const uint64_t target_loc = key_target_loc + (str ? query_loc : LOC_OFFSET - query_loc);

	// Pack
	const uint64_t loc = (chrom_id << 53) | (target_loc << 23) | (query_loc << 1) | str;
	return loc;
}

static void query_index(seed_v seeds, location_v *locations, const index_t index) {
	locations->len = 0;
	for (uint32_t i = 0; i < seeds.len; i++) {
		const seed_t seed = seeds.a[i];
		if (seed.EOR) {
			if (seed.str) {
				push_loc(UINT64_MAX, locations);
				return;
			} else {
				push_loc(1ULL << 63, locations);
				continue;
			}
		}

		const uint32_t bucket_id = seed.hash & BUCKET_MASK;
		const uint32_t seed_id   = seed.hash >> IDX_MAP_SIZE;
		/*
		printf("seed_id: %u, b_id: %u\n", seed_id, bucket_id);
		printf("B_MASK: %u, MAP_SIZE: %u\n", BUCKET_MASK, IDX_MAP_SIZE);
		*/

		const uint32_t start = (bucket_id == 0) ? 0 : index.map[bucket_id - 1];
		const uint32_t end   = index.map[bucket_id];

		for (uint32_t i = start; i < end; i++) {
			const uint64_t key      = index.key[i];
			const uint32_t kseed_id = key >> (LOC_SHIFT + 1);
			if (kseed_id == seed_id) {
				const uint64_t loc = key_2_loc(key, seed.str, seed.loc);
				push_loc(loc, locations);
			}
		}
	}
}

extern uint32_t BATCH_CAPACITY;
static index_t INDEX;
static unsigned NB_WORKERS;

typedef struct {
	uint32_t seq_len;
	uint32_t i_batch_id;
	uint32_t o_batch_id;
	read_metadata_t *i_metadata;
	read_metadata_t *o_metadata;
	uint8_t *seq;
	location_v loc;
	volatile int is_complete : 1;
} host_t;

static d_worker_t *worker_buf;
static host_t *host_buf;

void demeter_fpga_init(const unsigned nb_cus, __attribute__((unused)) const char *const binary_file,
                       const index_t index) {
	INDEX      = index;
	NB_WORKERS = nb_cus;
	MALLOC(worker_buf, d_worker_t, NB_WORKERS)
	MALLOC(host_buf, host_t, NB_WORKERS)
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		// Initialize read buf
		read_buf_init(&worker_buf[i].read_buf, BATCH_CAPACITY);
		// Initialize loc buf
		MALLOC(worker_buf[i].loc_buf.loc, uint64_t, LB_SIZE / sizeof(uint64_t));
		MALLOC(host_buf[i].seq, uint8_t, BATCH_CAPACITY);
		MALLOC(host_buf[i].loc.a, uint64_t, LB_SIZE / sizeof(uint64_t));
		host_buf[i].loc.capacity = LB_SIZE / sizeof(uint64_t);

		// Initialize the metadata buffers
		host_buf[i].i_metadata = NULL;
		host_buf[i].o_metadata = NULL;

		host_buf[i].is_complete = 1;
		worker_buf[i].id        = i;
		worker_buf[i].input_h   = buf_empty;
		worker_buf[i].input_d   = buf_empty;
		worker_buf[i].output_d  = buf_empty;
		worker_buf[i].output_h  = buf_empty;

		// Initialize the mutex
		pthread_mutex_init(&worker_buf[i].mutex, NULL);
	}
}

d_worker_t *demeter_get_worker(d_worker_t *const worker, const int no_input) {
	if (no_input) {
		d_worker_t *next_worker   = NULL;
		const unsigned current_id = (worker == NULL) ? 0 : (worker->id + 1) % NB_WORKERS;
		for (unsigned i = 0; i < NB_WORKERS; i++) {
			const unsigned id = (i + current_id) % NB_WORKERS;
			LOCK(worker_buf[id].mutex);
			if (worker_buf[id].input_h != buf_empty || worker_buf[id].input_d != buf_empty ||
			    worker_buf[id].output_d != buf_empty || worker_buf[id].output_h != buf_empty) {
				next_worker = &worker_buf[id];
				UNLOCK(worker_buf[id].mutex);
				break;
			}
			UNLOCK(worker_buf[id].mutex);
		}
		return next_worker;
	}

	if (worker == NULL) {
		return &worker_buf[0];
	}
	const unsigned id = (worker->id + 1) % NB_WORKERS;
	return &worker_buf[id];
}

void demeter_load_seq(d_worker_t *const worker) {
	const unsigned id = worker->id;
	memcpy(host_buf[id].seq, worker->read_buf.seq, BATCH_CAPACITY);
	host_buf[id].seq_len    = worker->read_buf.len;
	host_buf[id].i_batch_id = worker->read_buf.batch_id;
	host_buf[id].i_metadata = worker->read_buf.metadata;
	MALLOC(worker->read_buf.metadata, read_metadata_t, worker->read_buf.metadata_capacity);
	LOCK(worker->mutex);
	worker->input_h = buf_empty;
	worker->input_d = buf_full;
	UNLOCK(worker->mutex);
}

void demeter_start_kernel(d_worker_t *const worker) {
	const unsigned id        = worker->id;
	host_buf[id].is_complete = 0;
	host_buf[id].o_batch_id  = host_buf[id].i_batch_id;
	host_buf[id].o_metadata  = host_buf[id].i_metadata;
}

void seedfarm_execute(d_worker_t *const worker) {
	const unsigned id = worker->id;
#ifdef PROFILE
	PROF_INIT;
	PROF_START;
#endif
	seed_v seeds = {.capacity = 0, .len = 0, .a = NULL};
	extract_seeds(host_buf[id].seq, host_buf[id].seq_len, &seeds);
	/*
	for (uint32_t i = 0; i < seeds.len; i++) {
	        printf("%lx\t%x\t%c\n", seeds.a[i].hash, seeds.a[i].loc, "+-"[seeds.a[i].str]);
	}
	printf("len: %u\n", seeds.len);
	*/
	query_index(seeds, &host_buf[id].loc, INDEX);
	/*
	location_v loc = host_buf[id].loc;
	for (uint32_t i = 0; i < loc.len; i++) {
	        printf("%lx\n", loc.a[i]);
	}
	*/
	free(seeds.a);
#ifdef PROFILE
	PROF_END;
	PRINT_PROF("CPU");
#endif
	host_buf[id].is_complete = 1;
}

void demeter_load_loc(d_worker_t *const worker) {
	const unsigned id = worker->id;
	memcpy(worker->loc_buf.loc, host_buf[id].loc.a, LB_SIZE);
	// printf("len: %u\n", host_buf[id].loc.len);
	worker->loc_buf.batch_id = host_buf[id].o_batch_id;
	worker->loc_buf.metadata = host_buf[id].o_metadata;
	LOCK(worker->mutex);
	worker->output_d = buf_empty;
	worker->output_h = buf_full;
	UNLOCK(worker->mutex);
}

int demeter_is_complete(d_worker_t *const worker) {
	const unsigned id = worker->id;
	return host_buf[id].is_complete;
}

void demeter_fpga_destroy() {
	for (unsigned i = 0; i < NB_WORKERS; i++) {
		read_buf_destroy(worker_buf[i].read_buf);
		free(worker_buf[i].loc_buf.loc);
		free(host_buf[i].seq);
		free(host_buf[i].loc.a);
	}
	free(worker_buf);
	free(host_buf);
}
#endif
