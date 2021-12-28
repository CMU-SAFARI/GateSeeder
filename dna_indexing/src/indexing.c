#include "indexing.h"
#include "sort.h"
#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NB_THREADS
#define NB_THREADS 8
#endif
#define MEM_SECTION_SIZE 67108864

pthread_t threads[NB_THREADS];
size_t ms_id = 0;
info_t info[32];

void create_index(int fd, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                  const char *name) {
	min_loc_stra_v p;
	index_t idx;
	// Parse & extract
	parse_extract(fd, w, k, b, &p);
	// Sort p
	sort(&p);

	/*
	// Used for plots
	size_t counter = 0;
	uint32_t buf = 1;
	for (size_t i = 0; i < p.n; i++) {
	        counter++;
	        if(p.a[i].min/100000 > buf) {
	                buf = p.a[i].min/100000;
	                printf("%u\t%lu\n", p.a[i].min - 100000, counter/10000);
	                counter = 0;
	        }
	}
	*/

	// Write the data in the struct & filter out the most frequent minimizers
	info[0] = build_index(p, f, b, &idx, 0, 1);
	char name_buf[200];
	sprintf(name_buf, "%s.bin", name);
	FILE *fp_out = fopen(name_buf, "wb");
	if (fp_out == NULL) {
		err(1, "fopen %s", name_buf);
	}
	fwrite(&(idx.n), sizeof(idx.n), 1, fp_out);
	fwrite(idx.h, sizeof(idx.h[0]), idx.n, fp_out);
	fwrite(idx.loc, sizeof(idx.loc[0]), idx.m, fp_out);
	printf("Info: Binary file `%s` written\n", name_buf);
	// Write the info file
	write_info(name, 1);
	return;
}

static inline void thread_init(min_loc_stra_v p, const char *name, const unsigned int f, const unsigned int b) {
	static thread_index_t params[NB_THREADS];
	size_t thread_id = ms_id % NB_THREADS;
	if (thread_id >= NB_THREADS) {
		pthread_join(threads[thread_id], NULL);
	}
	params[thread_id] = (thread_index_t){p, ms_id, name, f, b};
	pthread_create(&threads[thread_id], NULL, thread_create_index, (void *)&params[thread_id]);
	ms_id++;
}
void create_index_part(int fd, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                       const unsigned int t, const char *name) {
	min_loc_stra_v p;
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size        = statbuf.st_size;
	char *file_buffer = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (file_buffer == MAP_FAILED) {
		err(1, "mmap");
	}

	p.n = 0;
	p.a = (min_loc_stra_t *)malloc(sizeof(min_loc_stra_t) * 5 * size / (2 * (w + 1)));
	if (p.a == NULL) {
		err(1, "malloc");
	}

	char *dna_buffer = (char *)malloc(size);
	if (dna_buffer == NULL) {
		err(1, "malloc");
	}

	size_t i          = 0;
	size_t dna_len    = 0;
	size_t chromo_len = 0;
	size_t initial    = 0;

	while (1) {
		char c = file_buffer[i];
		if (c == '>') {
			if (chromo_len > 0) {
				extract_minimizers(dna_buffer, chromo_len, w, k, b, &p, dna_len);
				if (p.n - initial > MEM_SECTION_SIZE) {
					size_t mem_size = 0;
					for (size_t j = MEM_SECTION_SIZE; j > 0; j--) {
						if (p.a[initial + j].loc - p.a[initial + j - 1].loc > t ||
						    p.a[initial + j - 1].loc < dna_len) {
							mem_size = j;
							break;
						}
					}
					thread_init((min_loc_stra_v){mem_size, &p.a[initial]}, name, f, b);
					initial = initial + mem_size;
				}
				dna_len += chromo_len;
				chromo_len = 0;
			}
			while (c != '\n' && c != 0) {
				i++;
				c = file_buffer[i];
			}
		} else {
			while (c != '\n' && c != 0) {
				dna_buffer[chromo_len] = c;
				chromo_len++;
				i++;
				c = file_buffer[i];
			}
		}

		if (c == 0) {
			break;
		}
		i++;
	}
	munmap(file_buffer, size);
	if (chromo_len > 0) {
		extract_minimizers(dna_buffer, chromo_len, w, k, b, &p, dna_len);
		dna_len += chromo_len;
		if (p.n - initial > MEM_SECTION_SIZE) {
			size_t mem_size = 0;
			for (size_t j = MEM_SECTION_SIZE; j > 0; j--) {
				if (p.a[initial + j].loc - p.a[initial + j - 1].loc > t) {
					mem_size = j;
					break;
				}
			}
			thread_init((min_loc_stra_v){mem_size, &p.a[initial]}, name, f, b);
			initial = initial + mem_size;
		}
		thread_init((min_loc_stra_v){p.n - initial, &p.a[initial]}, name, f, b);
	}
	free(dna_buffer);
	printf("Info: Indexed DNA length: %lu bases\n", dna_len);
	printf("Info: Number of (minimizer, location, strand): %lu\n", p.n);

	if (ms_id >= NB_THREADS) {
		for (size_t i = 0; i < NB_THREADS; i++) {
			pthread_join(threads[i], NULL);
		}
	} else {
		for (size_t i = 0; i < ms_id; i++) {
			pthread_join(threads[i], NULL);
		}
	}
	write_info(name, ms_id);
}

void *thread_create_index(void *arg) {
	thread_index_t *param = (thread_index_t *)arg;
	min_loc_stra_v p      = param->p;
	uint32_t offset       = p.a[0].loc;
	merge_sort(p.a, 0, p.n - 1);
	index_t idx;
	info[param->id] = build_index(p, param->f, param->b, &idx, offset, 0);
	char name_buf[200];
	sprintf(name_buf, "%s_%lu.bin", param->name, param->id);
	FILE *fp_out = fopen(name_buf, "wb");
	if (fp_out == NULL) {
		err(1, "fopen %s", name_buf);
	}
	fwrite(&(idx.n), sizeof(idx.n), 1, fp_out);
	fwrite(idx.h, sizeof(idx.h[0]), idx.n, fp_out);
	fwrite(idx.loc, sizeof(idx.loc[0]), idx.m, fp_out);
	printf("Info: Binary file `%s` written\n", name_buf);
	return NULL;
}

info_t build_index(min_loc_stra_v p, const unsigned int f, const unsigned int b, index_t *idx, const uint32_t offset,
                   char comp) {
	info_t info = {0, 0, 0, 0, 0, offset};
	idx->h      = (uint32_t *)malloc(sizeof(uint32_t) * (1 << b));
	idx->loc    = (uint32_t *)malloc(sizeof(uint32_t) * p.n);
	if (idx->h == NULL || idx->loc == NULL) {
		err(1, "malloc");
	}
	uint32_t index            = p.a[0].min;
	unsigned int freq_counter = 0;
	size_t pos                = 0;
	uint32_t l                = 0;

	for (size_t i = 0; i < index; i++) {
		idx->h[i] = 0;
	}

	for (size_t i = 1; i < p.n; i++) {
		if (index == p.a[i].min) {
			freq_counter++;
		} else {
			if (freq_counter < f) {
				info.nempty_counter++;
				for (size_t j = pos; j < i; j++) {
					idx->loc[l] =
					    (comp ? (p.a[j].loc & (UINT32_MAX - 1)) : ((p.a[j].loc - offset) << 1)) |
					    p.a[j].stra;
					l++;
				}
			} else {
				info.filter_counter += freq_counter;
			}
			pos          = i;
			freq_counter = 0;
			while (index != p.a[i].min) {
				idx->h[index] = l;
				index++;
			}
		}
	}
	if (freq_counter < f) {
		info.nempty_counter++;
		for (size_t j = pos; j < p.n; j++) {
			idx->loc[l] =
			    (comp ? (p.a[j].loc & (UINT32_MAX - 1)) : ((p.a[j].loc - offset) << 1)) | p.a[j].stra;
			l++;
		}
	} else {
		info.filter_counter += freq_counter;
	}
	for (size_t i = index; i < (1 << b); i++) {
		idx->h[i] = l;
	}

	idx->n = 1 << b;
	idx->m = l;
	info.n = idx->n;
	info.m = idx->m;

	float average            = (float)idx->m / idx->n;
	unsigned long sd_counter = (idx->h[0] - average) * (idx->h[0] - average);
	for (size_t i = 1; i < idx->n; i++) {
		sd_counter += (idx->h[i] - idx->h[i - 1] - average) * (idx->h[i] - idx->h[i - 1] - average);
	}
	info.sd = sqrtf((float)sd_counter / idx->n);
	return info;
}

void parse_extract(int fd, const unsigned int w, const unsigned int k, const unsigned int b, min_loc_stra_v *p) {
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		err(1, "fstat");
	}
	off_t size        = statbuf.st_size;
	char *file_buffer = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_POPULATE, fd, 0);
	if (file_buffer == MAP_FAILED) {
		err(1, "mmap");
	}

	p->n = 0;
	p->a = (min_loc_stra_t *)malloc(sizeof(min_loc_stra_t) * 5 * size / (2 * (w + 1)));
	if (p->a == NULL) {
		err(1, "malloc");
	}

	char *dna_buffer = (char *)malloc(size);
	if (dna_buffer == NULL) {
		err(1, "malloc");
	}

	size_t dna_len    = 0;
	size_t chromo_len = 0;

	for (size_t i = 0; i < size; i++) {
		char c = file_buffer[i];
		if (c == '>') {
			/*
			char name[20] = {0};
			for (size_t j = 0; file_buffer[i + 1 + j] != ' '; j++) {
			        name[j] = file_buffer[i + 1 + j];
			}
			printf("(\"%s\", %lu),\n", name, dna_len + chromo_len);
			*/
			if (chromo_len > 0) {
				extract_minimizers(dna_buffer, chromo_len, w, k, b, p, dna_len);
				dna_len += chromo_len;
				chromo_len = 0;
			}
			while (c != '\n' && c != 0) {
				i++;
				c = file_buffer[i];
			}
		} else {
			while (c != '\n' && c != 0) {
				dna_buffer[chromo_len] = c;
				chromo_len++;
				i++;
				c = file_buffer[i];
			}
		}
	}

	munmap(file_buffer, size);
	if (chromo_len > 0) {
		extract_minimizers(dna_buffer, chromo_len, w, k, b, p, dna_len);
		dna_len += chromo_len;
	}
	free(dna_buffer);
	printf("Info: Indexed DNA length: %lu bases\n", dna_len);
	printf("Info: Number of (minimizer, location, strand): %lu\n", p->n);
}

void write_info(const char *name, const size_t nb) {
	char name_buf[200];
	sprintf(name_buf, "%s_info.csv", name);
	FILE *fp_info = fopen(name_buf, "w");
	if (fp_info == NULL) {
		err(1, "fopen %s", name_buf);
	}
	fputs("\"size of the index\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %lu", info[i].n);
	}
	fputs("\n\"number of locations\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %lu", info[i].m);
	}
	fputs("\n\"number of ignored locations\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %lu", info[i].filter_counter);
	}
	fputs("\n\"number of non-empty buckets\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %lu", info[i].nempty_counter);
	}
	fputs("\n\"standard deviation of the number of locations per minimizers\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %f", info[i].sd);
	}
	fputs("\n\"offset(s)\"", fp_info);
	for (size_t i = 0; i < nb; i++) {
		fprintf(fp_info, ", %lu", info[i].offset);
	}
	fputs("\n", fp_info);
	printf("Info: Info file `%s` written\n", name_buf);
}
