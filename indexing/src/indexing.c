#include "extraction.h"
#include "indexing.h"
#include "types.h"
#include "util.h"
#include <err.h>
#include <pthread.h>
#include <string.h>

extern unsigned NB_THREADS;
#define IDX_MAGIC "ALOHA"

// TODO: Proper multithread, each thread takes one sequence, it extracts and sorts the seeds, in the end all the
// sequences are merged

typedef struct {
	key_v minimizers;
	unsigned i;
} thread_sort_t;

static inline void merge(keym_t *const keys, const uint32_t l, const uint32_t m, const uint32_t r) {
	uint32_t i, j;
	uint32_t n1 = m - l + 1;
	uint32_t n2 = r - m;

	keym_t *L = (keym_t *)malloc(n1 * sizeof(keym_t));
	keym_t *R = (keym_t *)malloc(n2 * sizeof(keym_t));
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
		memcpy(&keys[k], &L[i], len * sizeof(keym_t));
	} else {
		const unsigned len = n2 - j;
		memcpy(&keys[k], &R[j], len * sizeof(keym_t));
	}
	free(L);
	free(R);
}

static void merge_sort(keym_t *keys, uint32_t l, uint32_t r) {
	if (l < r) {
		uint32_t m = l + (r - l) / 2;
		merge_sort(keys, l, m);
		merge_sort(keys, m + 1, r);
		merge(keys, l, m, r);
	}
}

static void final_merge(keym_t *keys, uint32_t n, uint32_t l, uint32_t r) {
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
	key_v minimizers     = param->minimizers;
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

static void sort(const key_v minimizers) {
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
	uint64_t nb_bases = 0;
	key_v minimizers  = {.capacity = 1 << 10, .len = 0};
	MALLOC(minimizers.keys, keym_t, minimizers.capacity);

	for (unsigned i = 0; i < target.nb_sequences; i++) {
		extract_seeds(target.seq[i], target.len[i], i, &minimizers, w, k, b);
		nb_bases += target.len[i];
	}
	fprintf(stderr, "[INFO] %u extrcated minimizers (%u sequences, %lu bases)\n", minimizers.len,
	        target.nb_sequences, nb_bases);

	sort(minimizers);
	fputs("[INFO] minimizers sorted\n", stderr);

	index_t index = {.map_len = 1 << b};
	MALLOC(index.map, uint32_t, index.map_len);
	MALLOC(index.key, keym_t, minimizers.len);

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
	return index;
}

void write_index(FILE *fp, const index_MS_t index, const target_t target, const unsigned w, const unsigned k,
                 const unsigned b, const unsigned max_occ, const size_t MS_size) {
	fwrite(IDX_MAGIC, sizeof(char), 5, fp);
	fwrite(&w, sizeof(unsigned), 1, fp);
	fwrite(&k, sizeof(unsigned), 1, fp);
	fwrite(&b, sizeof(unsigned), 1, fp);
	fwrite(&max_occ, sizeof(unsigned), 1, fp);
	fwrite(&index.nb_MS, sizeof(unsigned), 1, fp);
	fwrite(index.map, sizeof(uint32_t), 1 << b, fp);
	for (unsigned i = 0; i < index.nb_MS; i++) {
		fwrite(index.key[i], sizeof(uint64_t), MS_size >> 2, fp);
	}
	fwrite(&target.nb_sequences, sizeof(unsigned), 1, fp);
	for (unsigned i = 0; i < target.nb_sequences; i++) {
		uint8_t len = strlen(target.name[i]);
		fwrite(&len, sizeof(uint8_t), 1, fp);
		fwrite(target.name[i], sizeof(char), len, fp);
		fwrite(&target.len[i], sizeof(uint32_t), 1, fp);
		fwrite(target.seq[i], sizeof(uint8_t), target.len[i], fp);
	}
}

// INDEX_MS MAP
// |-- MS_ID --|-- POSITION IN THE MS --|
//  31          24                     0
#define MS_POS_SIZE 25
#define MS_POS_MASK ((1 << MS_POS_SIZE) - 1))

// KEY
// loc -> 32 bits  |
// CH_ID -> 10 bits | 42 bits
#define LOC_SHIFT 42
// STR -> 1 bits
// SEED_ID -> 21 bits
index_MS_t partion_index(const index_t index, const size_t MS_size, const unsigned max_nb_MS) {
	if ((MS_size >> 2) < index.map_len) {
		errx(1, "[ERROR] size of the map greater than the size of one memory section");
	}

	index_MS_t index_MS = {.nb_MS = 0};
	MALLOC(index_MS.key, uint64_t *, max_nb_MS);
	MALLOC(index_MS.map, uint32_t, index.map_len);

	const size_t MS_SIZE_64 = MS_size >> 3;
	size_t size_counter     = 0;
	uint32_t start_i        = 0;
	for (uint32_t i = 0; i < index.map_len; i++) {
		const uint32_t start = i ? index.map[i - 1] : 0;
		const uint32_t end   = index.map[i];
		size_counter += end - start;
		if (size_counter > MS_SIZE_64) {

			if (i == 0) {
				errx(1, "[ERROR] MS size too small");
			}

			// Create the key array for the new MS
			MALLOC(index_MS.key[index_MS.nb_MS], uint64_t, MS_SIZE_64);

			// Write the key array
			const uint32_t start = start_i ? index.map[start_i - 1] : 0;
			const uint32_t end   = index.map[i - 1];

			/*
			printf("start_i: %u map[]: %u\n", start_i - 1, start);
			printf("end_i: %u map[]: %u\n", i - 1, end);
			printf("next_i: %u map[]: %u\n", i, index.map[i]);
			printf("MS: %u\n", index_MS.nb_MS);
			*/

			for (uint32_t j = 0; j < end - start; j++) {
				index_MS.key[index_MS.nb_MS][j] =
				    index.key[start + j].loc | ((uint64_t)index.key[start + j].chrom_id << 32) |
				    ((uint64_t)index.key[start + j].str << LOC_SHIFT) |
				    ((uint64_t)index.key[start + j].seed_id << (LOC_SHIFT + 1));
			}

			// Write the map array
			// map[start_i - 1] -> first pos for the first seed
			// map[i - 1] -> (last pos for the last seed) + 1

			// map[start_i] -> (last pos for the first seed) + 1
			// need to be shifted based on (map[start_i - 1])

			uint32_t base_pos  = (1 << MS_POS_SIZE) * index_MS.nb_MS;
			uint32_t MS_offset = base_pos - start;
			// printf("MS_offset: %u\n", MS_offset);
			for (uint32_t j = start_i; j < i; j++) {
				index_MS.map[j] = index.map[j] + MS_offset;
			}

			index_MS.nb_MS++;
			if (index_MS.nb_MS == max_nb_MS) {
				errx(1, "[ERROR] maximum number of MS too small");
			}
			start_i                  = i;
			const uint32_t new_start = i ? index.map[i - 1] : 0;
			const uint32_t new_end   = index.map[i];
			size_counter             = new_end - new_start;
			if (size_counter > MS_SIZE_64) {
				errx(1, "[ERROR] MS size too small");
			}
		}
	}

	// Create the key array for the new MS
	MALLOC(index_MS.key[index_MS.nb_MS], uint64_t, MS_SIZE_64);

	// Write the key array
	const uint32_t start = start_i ? index.map[start_i - 1] : 0;
	const uint32_t end   = index.map[index.map_len - 1];

	for (uint32_t j = 0; j < end - start; j++) {
		index_MS.key[index_MS.nb_MS][j] = index.key[start + j].loc |
		                                  ((uint64_t)index.key[start + j].chrom_id << 32) |
		                                  ((uint64_t)index.key[start + j].str << LOC_SHIFT) |
		                                  ((uint64_t)index.key[start + j].seed_id << (LOC_SHIFT + 1));
	}

	// Write the map array
	uint32_t base_pos  = (1 << MS_POS_SIZE) * index_MS.nb_MS;
	uint32_t MS_offset = base_pos - start;
	// printf("new_start: %10x\n", start_i);
	for (uint32_t j = start_i; j < index.map_len; j++) {
		index_MS.map[j] = index.map[j] + MS_offset;
	}
	index_MS.nb_MS++;

	/*
	// DEBUG
	printf("MAP\n");
	uint32_t prev = 0;
	for (uint32_t i = 0; i < index.map_len; i++) {
	        if (index_MS.map[i] != prev) {
	                printf("%x: %u\n", i, index_MS.map[i]);
	                prev = index_MS.map[i];
	        }
	}
	printf("KEY\n");
	for (unsigned i = 0; i < index_MS.nb_MS; i++) {
	        printf("MS %u:\n", i);
	        for (unsigned j = 0; j < MS_SIZE_64; j++) {
	                printf("%lx\n", index_MS.key[i][j]);
	        }
	}
	*/
	printf("[INFO] Number of used MS %u\n", index_MS.nb_MS + 1);
	return index_MS;
}

void index_MS_destroy(const index_MS_t index) {
	free(index.map);
	for (unsigned i = 0; i < index.nb_MS; i++) {
		free(index.key[i]);
	}
	free(index.key);
}

void write_gold_index(FILE *fp, const index_t index, const target_t target, const unsigned w, const unsigned k,
                      const unsigned b, const unsigned max_occ) {
	fwrite(IDX_MAGIC, sizeof(char), 5, fp);
	fwrite(&w, sizeof(unsigned), 1, fp);
	fwrite(&k, sizeof(unsigned), 1, fp);
	fwrite(&b, sizeof(unsigned), 1, fp);
	fwrite(&max_occ, sizeof(unsigned), 1, fp);
	fwrite(&index.key_len, sizeof(uint32_t), 1, fp);
	fwrite(index.map, sizeof(uint32_t), 1 << b, fp);
	// Write the key array
	uint64_t *key;
	MALLOC(key, uint64_t, index.key_len);
	for (unsigned i = 0; i < index.key_len; i++) {
		key[i] = index.key[i].loc | ((uint64_t)index.key[i].chrom_id << 32) |
		         ((uint64_t)index.key[i].str << LOC_SHIFT) |
		         ((uint64_t)index.key[i].seed_id << (LOC_SHIFT + 1));
	}
	fwrite(key, sizeof(uint64_t), index.key_len, fp);
	fwrite(&target.nb_sequences, sizeof(unsigned), 1, fp);
	for (unsigned i = 0; i < target.nb_sequences; i++) {
		uint8_t len = strlen(target.name[i]);
		fwrite(&len, sizeof(uint8_t), 1, fp);
		fwrite(target.name[i], sizeof(char), len, fp);
		fwrite(&target.len[i], sizeof(uint32_t), 1, fp);
		fwrite(target.seq[i], sizeof(uint8_t), target.len[i], fp);
	}
}
