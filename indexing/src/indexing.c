#include "extraction.h"
#include "indexing.h"
#include "types.h"
#include "util.h"
#include <pthread.h>
#include <string.h>

extern unsigned NB_THREADS;
#define IDX_MAGIC "ALOHA"

// TODO: Proper multithread, each thread takes one sequence, it extracts and sorts the seeds, in the end all the
// sequences are merged

typedef struct {
	key128_v minimizers;
	unsigned i;
} thread_sort_t;

static inline void merge(key128_t *const keys, const uint32_t l, const uint32_t m, const uint32_t r) {
	uint32_t i, j;
	uint32_t n1 = m - l + 1;
	uint32_t n2 = r - m;

	key128_t *L = (key128_t *)malloc(n1 * sizeof(key128_t));
	key128_t *R = (key128_t *)malloc(n2 * sizeof(key128_t));
	if (L == NULL || R == NULL) {
		err(1, "malloc");
	}

	for (i = 0; i < n1; i++) {
		L[i] = keys[l + i];
	}
	for (j = 0; j < n2; j++) {
		R[j] = keys[m + 1 + j];
	}

	i        = 0;
	j        = 0;
	size_t k = l;

	while (i < n1 && j < n2) {
		const uint64_t L_hash = L[i].hash >> 1;
		const uint64_t R_hash = R[j].hash >> 1;
		const int flag        = (R_hash - L_hash) >> 63;
		keys[k].loc           = flag * R[j].loc + (1 - flag) * L[i].loc;
		keys[k].hash          = flag * R[j].hash + (1 - flag) * L[i].hash;
		k++;
		j += flag;
		i += (1 - flag);
	}

	if (i < n1) {
		const unsigned len = n1 - i;
		memcpy(&keys[k], &L[i], len * sizeof(key128_t));
	} else {
		const unsigned len = n2 - j;
		memcpy(&keys[k], &R[j], len * sizeof(key128_t));
	}
	free(L);
	free(R);
}

static void merge_sort(key128_t *keys, uint32_t l, uint32_t r) {
	if (l < r) {
		uint32_t m = l + (r - l) / 2;
		merge_sort(keys, l, m);
		merge_sort(keys, m + 1, r);
		merge(keys, l, m, r);
	}
}

static void final_merge(key128_t *keys, uint32_t n, uint32_t l, uint32_t r) {
	if (r == l + 2) {
		merge(keys, (uint64_t)l * n / NB_THREADS, (uint64_t)(l + 1) * n / NB_THREADS - 1,
		      (uint64_t)r * n / NB_THREADS - 1);
	}

	else if (r > l + 2) {
		uint32_t m = (r + l) / 2;
		final_merge(keys, n, l, m);
		final_merge(keys, n, m, r);
		merge(keys, (uint64_t)l * n / NB_THREADS, (uint64_t)m * n / NB_THREADS - 1,
		      (uint64_t)r * n / NB_THREADS - 1);
	}
}

static void *thread_merge_sort(void *arg) {
	thread_sort_t *param = (thread_sort_t *)arg;
	key128_v minimizers  = param->minimizers;
	unsigned i           = param->i;
	uint32_t l           = (uint64_t)i * minimizers.len / NB_THREADS;
	uint32_t r           = (uint64_t)(i + 1) * minimizers.len / NB_THREADS - 1;
	if (l < r) {
		uint32_t m = l + (r - l) / 2;
		merge_sort(minimizers.keys, l, m);
		merge_sort(minimizers.keys, m + 1, r);
		merge(minimizers.keys, l, m, r);
	}
	return (void *)NULL;
}

static void sort(const key128_v minimizers) {
	pthread_t *threads;
	MALLOC(threads, pthread_t, NB_THREADS);
	thread_sort_t *params;
	MALLOC(params, thread_sort_t, NB_THREADS);
	for (size_t i = 0; i < NB_THREADS; i++) {
		params[i].minimizers = minimizers;
		params[i].i          = i;
		pthread_create(&threads[i], NULL, thread_merge_sort, (void *)&params[i]);
	}

	for (size_t i = 0; i < NB_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	final_merge(minimizers.keys, minimizers.len, 0, NB_THREADS);
	free(threads);
	free(params);
}

index_t gen_index(const target_t target, const unsigned w, const unsigned k, const unsigned b, const unsigned max_occ) {
	uint64_t nb_bases   = 0;
	key128_v minimizers = {.capacity = 1 << 10, .len = 0};
	MALLOC(minimizers.keys, key128_t, 1 << 10);

	for (unsigned i = 0; i < target.nb_sequences; i++) {
		extract_seeds(target.seq[i], target.len[i], i, &minimizers, w, k);
		nb_bases += target.len[i];
	}
	fprintf(stderr, "[INFO] %u extrcated minimizers (%u sequences, %lu bases)\n", minimizers.len,
	        target.nb_sequences, nb_bases);

	sort(minimizers);
	fputs("[INFO] minimizers sorted\n", stderr);

	index_t index = {.map_len = 1 << b};
	MALLOC(index.map, uint32_t, index.map_len);
	MALLOC(index.key, key128_t, minimizers.len);

	// hash: after extraction
	// |--- BUCKET_ID ---|--- SEED_ID ---|--- STR ---|
	//  2K         2K-B+1 2K-B          1           0

	unsigned bucket_shift = 2 * k - b;

	uint64_t hash      = minimizers.keys[0].hash >> 1;
	uint32_t bucket_id = hash >> bucket_shift;
	for (uint32_t i = 0; i < bucket_id; i++) {
		index.map[i] = 0;
	}
	uint32_t key_pos   = 0;
	uint32_t map_start = 0;

	for (uint32_t i = 1; i < minimizers.len; i++) {
		uint64_t tmp_hash = minimizers.keys[i].hash >> 1;
		if (tmp_hash != hash) {
			if ((i - map_start) <= max_occ) {
				for (uint32_t j = map_start; j < i; j++) {
					index.key[key_pos] = minimizers.keys[j];
					key_pos++;
				}
			}
			map_start = i;
			hash      = tmp_hash;
			while (bucket_id != (hash >> bucket_shift)) {
				index.map[bucket_id] = key_pos;
				bucket_id++;
			}
		}
	}
	if ((minimizers.len - map_start) <= max_occ) {
		for (uint32_t j = map_start; j < minimizers.len; j++) {
			index.key[key_pos] = minimizers.keys[j];
			key_pos++;
		}
	}
	for (uint32_t i = bucket_id; i < (uint32_t)(1 << b); i++) {
		index.map[i] = key_pos;
	}
	index.map_len = 1 << b;
	index.key_len = key_pos;
	free(minimizers.keys);
	return index;
}

void write_index(FILE *fp, const index_t index, const target_t target, const unsigned w, const unsigned k,
                 const unsigned b, const unsigned max_occ) {
	fwrite(IDX_MAGIC, sizeof(char), 5, fp);
	fwrite(&w, sizeof(unsigned), 1, fp);
	fwrite(&k, sizeof(unsigned), 1, fp);
	fwrite(&b, sizeof(unsigned), 1, fp);
	fwrite(&max_occ, sizeof(unsigned), 1, fp);
	fwrite(&index.key_len, sizeof(uint32_t), 1, fp);
	fwrite(&index.map_len, sizeof(uint32_t), 1, fp);
	fwrite(index.key, sizeof(uint64_t), index.key_len, fp);
	fwrite(index.map, sizeof(uint32_t), index.map_len, fp);
	fwrite(&target.nb_sequences, sizeof(unsigned), 1, fp);
	for (unsigned i = 0; i < target.nb_sequences; i++) {
		uint8_t len = strlen(target.name[i]);
		fwrite(&len, sizeof(uint8_t), 1, fp);
		fwrite(target.name[i], sizeof(char), len, fp);
		fwrite(&target.len[i], sizeof(uint32_t), 1, fp);
		fwrite(target.seq[i], sizeof(uint8_t), target.len[i], fp);
	}
}

void index_destroy(const index_t index) {
	free(index.map);
	free(index.key);
}
