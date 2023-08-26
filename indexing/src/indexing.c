#include "GateSeeder_util.h"
#include "extraction.h"
#include "indexing.h"
#include "parsing.h"
#include "types.h"
#include <err.h>
#include <pthread.h>
#include <string.h>

#define IDX_MAGIC "ALOHA"

// TODO: multithread for minimizer extraction

typedef struct {
	uint32_t map_len, key_len;
	uint32_t *map;
	gkey_t *key;
} gindex_t;

typedef struct {
	gkey_v minimizers;
	uint32_t i;
	uint32_t nb_threads;
} thread_sort_t;

static inline void merge(gkey_t *const keys, const uint32_t l, const uint32_t m, const uint32_t r) {
	uint32_t i, j;
	uint32_t n1 = m - l + 1;
	uint32_t n2 = r - m;

	gkey_t *L = (gkey_t *)malloc(n1 * sizeof(gkey_t));
	gkey_t *R = (gkey_t *)malloc(n2 * sizeof(gkey_t));
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
		const uint64_t L_hash = ((uint64_t)L[i].bucket_id << 32) | L[i].seed_id;
		const uint64_t R_hash = ((uint64_t)R[j].bucket_id << 32) | R[j].seed_id;
		if (L_hash <= R_hash) {
			keys[k] = L[i];
			i++;
		} else {
			keys[k] = R[j];
			j++;
		}
		k++;
	}

	if (i < n1) {
		const unsigned len = n1 - i;
		memcpy(&keys[k], &L[i], len * sizeof(gkey_t));
	} else {
		const unsigned len = n2 - j;
		memcpy(&keys[k], &R[j], len * sizeof(gkey_t));
	}
	free(L);
	free(R);
}

static void merge_sort(gkey_t *keys, const uint32_t l, const uint32_t r) {
	if (l < r) {
		uint32_t m = l + (r - l) / 2;
		merge_sort(keys, l, m);
		merge_sort(keys, m + 1, r);
		merge(keys, l, m, r);
	}
}

static void final_merge(gkey_t *keys, const uint32_t n, const uint32_t l, const uint32_t r, const uint32_t nb_threads) {
	if (r == l + 2) {
		merge(keys, (uint64_t)l * n / nb_threads, (uint64_t)(l + 1) * n / nb_threads - 1,
		      (uint64_t)r * n / nb_threads - 1);
	}

	else if (r > l + 2) {
		uint32_t m = (r + l) / 2;
		final_merge(keys, n, l, m, nb_threads);
		final_merge(keys, n, m, r, nb_threads);
		merge(keys, (uint64_t)l * n / nb_threads, (uint64_t)m * n / nb_threads - 1,
		      (uint64_t)r * n / nb_threads - 1);
	}
}

static void *thread_merge_sort(void *arg) {
	thread_sort_t *param = (thread_sort_t *)arg;
	gkey_v minimizers    = param->minimizers;
	uint32_t nb_threads  = param->nb_threads;
	uint32_t i           = param->i;
	uint32_t l           = (uint64_t)i * minimizers.len / nb_threads;
	uint32_t r           = (uint64_t)(i + 1) * minimizers.len / nb_threads - 1;
	if (l < r) {
		uint32_t m = l + (r - l) / 2;
		merge_sort(minimizers.keys, l, m);
		merge_sort(minimizers.keys, m + 1, r);
		merge(minimizers.keys, l, m, r);
	}
	return (void *)NULL;
}

static void sort(const gkey_v minimizers, const uint32_t nb_threads) {
	pthread_t *threads;
	MALLOC(threads, pthread_t, nb_threads);
	thread_sort_t *params;
	MALLOC(params, thread_sort_t, nb_threads);
	for (uint32_t i = 0; i < nb_threads; i++) {
		params[i] = (thread_sort_t){.minimizers = minimizers, .i = i, .nb_threads = nb_threads};
		pthread_create(&threads[i], NULL, thread_merge_sort, (void *)&params[i]);
	}

	for (size_t i = 0; i < nb_threads; i++) {
		pthread_join(threads[i], NULL);
	}
	final_merge(minimizers.keys, minimizers.len, 0, nb_threads, nb_threads);
	free(threads);
	free(params);
}

// KEY
// loc -> 32 bits  |
// CH_ID -> 10 bits | 42 bits
#define LOC_SHIFT 42
// STR -> 1 bits
// SEED_ID -> 21 bits

void index_write(const char *const file_name, const gindex_t index, const target_t target, const uint32_t w,
                 const uint32_t k, const uint32_t map_size, const uint32_t max_occ) {

	FILE *fp;
	FOPEN(fp, file_name, "wb");
	fwrite(IDX_MAGIC, sizeof(char), 5, fp);
	fwrite(&w, sizeof(uint32_t), 1, fp);
	fwrite(&k, sizeof(uint32_t), 1, fp);
	fwrite(&map_size, sizeof(uint32_t), 1, fp);
	fwrite(&max_occ, sizeof(uint32_t), 1, fp);
	fwrite(&index.key_len, sizeof(uint32_t), 1, fp);
	fwrite(index.map, sizeof(uint32_t), 1 << map_size, fp);

	// Write the key array
	uint64_t *key;
	MALLOC(key, uint64_t, index.key_len);
	for (uint32_t i = 0; i < index.key_len; i++) {
		key[i] = index.key[i].loc | ((uint64_t)index.key[i].chrom_id << 32) |
		         ((uint64_t)index.key[i].str << LOC_SHIFT) |
		         ((uint64_t)index.key[i].seed_id << (LOC_SHIFT + 1));
	}
	fwrite(key, sizeof(uint64_t), index.key_len, fp);
	// printf("key_len: %u\n", index.key_len);
	fwrite(&target.nb_sequences, sizeof(uint32_t), 1, fp);
	for (uint32_t i = 0; i < target.nb_sequences; i++) {
		uint8_t len = strlen(target.name[i]);
		fwrite(&len, sizeof(uint8_t), 1, fp);
		fwrite(target.name[i], sizeof(char), len, fp);
		fwrite(&target.len[i], sizeof(uint32_t), 1, fp);
		free(target.name[i]);
	}
	free(target.len);
	free(target.name);
	FCLOSE(fp);
}

void index_gen(const uint32_t w, const uint32_t k, const uint32_t map_size, const uint32_t max_occ,
               const uint32_t ms_size, const char *const target_file_name, const char *const index_file_name,
               const uint32_t nb_threads) {

	const target_t target = parse_target(target_file_name);
	fprintf(stderr, "[INFO] nb_sequences: %u\n", target.nb_sequences);
	uint64_t nb_bases = 0;
	gkey_v minimizers = {.capacity = 1 << 10, .len = 0};
	MALLOC(minimizers.keys, gkey_t, minimizers.capacity);

	for (uint32_t i = 0; i < target.nb_sequences; i++) {
		extract_seeds(target.seq[i], target.len[i], i, &minimizers, w, k, map_size);
		nb_bases += target.len[i];

		// free the target sequence
		free(target.seq[i]);
	}
	free(target.seq);

	fprintf(stderr, "[INFO] %u extrcated minimizers (%u sequences, %lu bases)\n", minimizers.len,
	        target.nb_sequences, nb_bases);

	sort(minimizers, nb_threads);

	gindex_t index = {.map_len = 1 << map_size};
	MALLOC(index.map, uint32_t, index.map_len);
	MALLOC(index.key, gkey_t, minimizers.len);

	uint32_t bucket_id = minimizers.keys[0].bucket_id;
	// If the first bucket_id is not 0
	for (uint32_t i = 0; i < bucket_id; i++) {
		index.map[i] = 0;
	}

	uint32_t key_pos   = 0;
	uint32_t map_start = 0;

	uint64_t hash = ((uint64_t)minimizers.keys[0].bucket_id << 32) | minimizers.keys[0].seed_id;

	for (uint32_t i = 1; i < minimizers.len; i++) {
		uint64_t tmp_hash = ((uint64_t)minimizers.keys[i].bucket_id << 32) | minimizers.keys[i].seed_id;

		if (tmp_hash != hash) {
			// Filter if the occurence is too high
			if ((i - map_start) <= max_occ) {
				for (uint32_t j = map_start; j < i; j++) {
					index.key[key_pos] = minimizers.keys[j];
					key_pos++;
				}
			}
			map_start = i;
			hash      = tmp_hash;
			// printf("new hash: %lx\n", hash);
			// printf("bucket_id: %x\n", bucket_id);
			//  Move to the next bucket
			while (bucket_id != minimizers.keys[i].bucket_id) {
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
	for (uint32_t i = bucket_id; i < index.map_len; i++) {
		index.map[i] = key_pos;
	}
	index.key_len = key_pos;
	free(minimizers.keys);
	/*
	// DEBUG
	printf("MAP\n");
	uint32_t prev = 0;
	for (uint32_t i = 0; i < index.map_len; i++) {
	        if (index.map[i] != prev) {
	                printf("%x: %x\n", i, index.map[i]);
	                prev = index.map[i];
	        }
	}
	*/

	fprintf(stderr, "[INFO] map_len: %u (%lu MB), key_len: %u (%lu MB)\n", index.map_len,
	        index.map_len * sizeof(index.map[0]) >> 20, index.key_len, (index.key_len * 8L) >> 20);
	index_write(index_file_name, index, target, w, k, map_size, max_occ);

	const uint32_t map_ms_len = 1 << (ms_size - 2);
	const uint32_t key_ms_len = 1 << (ms_size - 3);
	fprintf(stderr, "[INFO] %u MS(s) required for the map array\n",
	        index.map_len / map_ms_len + ((index.map_len % map_ms_len) != 0));
	fprintf(stderr, "[INFO] %u MS(s) required for the key array\n",
	        index.key_len / key_ms_len + ((index.key_len % key_ms_len != 0)));
	free(index.map);
	free(index.key);
}
