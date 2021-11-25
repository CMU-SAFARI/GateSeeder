#include "indexing.h"
#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define NB_THREADS 8
#define MEM_SECTION_SIZE 67108864

static inline unsigned char compare(min_loc_stra_t left, min_loc_stra_t right) { return (left.min) <= (right.min); }

void create_index(int fd, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                  index_t *idx) {
	min_loc_stra_v p;
	// Parse & extract
	parse_extract(fd, w, k, b, &p);
	// Sort p
	sort(&p);
	puts("Info: Array sorted");
	// Write the data in the struct & filter out the most frequent minimizers
	build_index(p, f, b, idx);
	return;
}

void create_index_part(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int f, const unsigned int b,
                       index_v *idx) {
	min_loc_stra_v p;
	p.n = 0;
	// Parse & extract
	// parse_extract(fp, w, k, b, &p);
	// Partition p
	idx->n = 0;
	min_loc_stra_v p_block[16];
	size_t counter    = 0;
	p_block[idx->n].a = (min_loc_stra_t *)malloc(sizeof(min_loc_stra_t) * MEM_SECTION_SIZE);
	if (p_block[idx->n].a == NULL) {
		err(1, "malloc");
	}
	for (size_t i = 0; i < p.n; i++) {
		if (counter == MEM_SECTION_SIZE) {
			p_block[idx->n].n = MEM_SECTION_SIZE;
			idx->n++;
			p_block[idx->n].a = (min_loc_stra_t *)malloc(sizeof(min_loc_stra_t) * MEM_SECTION_SIZE);
			if (p_block[idx->n].a == NULL) {
				err(1, "malloc");
			}
			counter = 0;
		}
		p_block[idx->n].a[counter] = p.a[i];
		counter++;
	}
	p_block[idx->n].n = counter;
	idx->n++;
	printf("Info: Number of memory blocks: %lu\n", idx->n);
	idx->a = (index_t *)malloc(sizeof(index_t) * idx->n);
	if (idx->a == NULL) {
		err(1, "malloc");
	}

	// Sort & build index p_blocks
	for (size_t i = 0; i < idx->n; i++) {
		printf("\tMEMORY SECTION %lu\n", i);
		sort(&p_block[i]);
		puts("Info: Array sorted");
		build_index(p_block[i], f, b, &idx->a[i]);
	}
}

void build_index(min_loc_stra_v p, const unsigned int f, const unsigned int b, index_t *idx) {
	idx->h   = (uint32_t *)malloc(sizeof(uint32_t) * (1 << b));
	idx->loc = (uint32_t *)malloc(sizeof(uint32_t) * p.n);
	if (idx->h == NULL || idx->loc == NULL) {
		err(1, "malloc");
	}

	unsigned int diff_counter   = 0;
	uint32_t index              = p.a[0].min;
	unsigned int freq_counter   = 0;
	unsigned int filter_counter = 0;
	size_t pos                  = 0;
	uint32_t l                  = 0;

	for (size_t i = 0; i < index; i++) {
		idx->h[i] = 0;
	}

	for (size_t i = 1; i < p.n; i++) {
		if (index == p.a[i].min) {
			freq_counter++;
		} else {
			if (freq_counter < f) {
				diff_counter++;
				for (size_t j = pos; j < i; j++) {
					idx->loc[l] = (p.a[j].loc & (UINT32_MAX - 1)) | p.a[j].stra;
					l++;
				}
			} else {
				filter_counter++;
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
		diff_counter++;
		for (size_t j = pos; j < p.n; j++) {
			idx->loc[l] = (p.a[j].loc & (UINT32_MAX - 1)) | p.a[j].stra;
			l++;
		}
	} else {
		filter_counter++;
	}

	for (size_t i = index; i < (1 << b); i++) {
		idx->h[i] = l;
	}

	free(p.a);
	printf("Info: Number of ignored minimizers: %u\n", filter_counter);
	printf("Info: Number of distinct minimizers: %u\n", diff_counter);
	idx->n              = 1 << b;
	idx->m              = l;
	float location_size = (float)idx->m / (1 << 28);
	float hash_size     = (float)idx->n / (1 << 28);
	float average       = (float)idx->m / idx->n;

	printf("Info: Number of locations: %u\n", idx->m);
	printf("Info: Size of the location array: %fGB\n", location_size);
	printf("Info: Size of the minimizer array: %fGB\n", hash_size);
	printf("Info: Total size: %fGB\n", location_size + hash_size);
	printf("Info: Average locations per minimizers: %f\n", average);

	unsigned int empty_counter = 0;
	uint32_t j                 = 0;
	unsigned long sd_counter   = (idx->h[0] - average) * (idx->h[0] - average);
	for (size_t i = 0; i < idx->n; i++) {
		if (i > 0) {
			sd_counter += (idx->h[i] - idx->h[i - 1] - average) * (idx->h[i] - idx->h[i - 1] - average);
		}
		if (idx->h[i] == j) {
			empty_counter++;
		} else {
			j = idx->h[i];
		}
	}

	float sd = sqrtf((float)sd_counter / idx->n);
	printf("Info: Standard deviation of the number of locations per "
	       "minimizers: %f\n",
	       sd);
	printf("Info: Number of empty entries in the hash-table: %u (%f%%)\n", empty_counter,
	       (float)empty_counter / idx->n * 100);
}

void parse_extract(int fd, const unsigned int w, const unsigned int k, const unsigned int b, min_loc_stra_v *p) {

	off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
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

	size_t i          = 0;
	size_t dna_len    = 0;
	size_t chromo_len = 0;

	while (1) {
		char c = file_buffer[i];

		if (c == '>') {
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

		if (c == 0) {
			break;
		}
		i++;
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

void sort(min_loc_stra_v *p) {
	pthread_t threads[NB_THREADS];
	thread_param_t params[NB_THREADS];
	for (size_t i = 0; i < NB_THREADS; i++) {
		params[i].p = p;
		params[i].i = i;
		pthread_create(&threads[i], NULL, thread_merge_sort, (void *)&params[i]);
	}

	for (size_t i = 0; i < NB_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	final_merge(p->a, p->n, 0, NB_THREADS);
}

void *thread_merge_sort(void *arg) {
	thread_param_t *param = (thread_param_t *)arg;
	size_t l              = param->i * param->p->n / NB_THREADS;
	size_t r              = (param->i + 1) * param->p->n / NB_THREADS - 1;
	if (l < r) {
		size_t m = l + (r - l) / 2;
		merge_sort(param->p->a, l, m);
		merge_sort(param->p->a, m + 1, r);
		merge(param->p->a, l, m, r);
	}
	return (void *)NULL;
}

void merge_sort(min_loc_stra_t *a, size_t l, size_t r) {
	if (l < r) {
		size_t m = l + (r - l) / 2;
		merge_sort(a, l, m);
		merge_sort(a, m + 1, r);
		merge(a, l, m, r);
	}
}

void final_merge(min_loc_stra_t *a, size_t n, size_t l, size_t r) {
	if (r == l + 2) {
		merge(a, l * n / NB_THREADS, (l + 1) * n / NB_THREADS - 1, r * n / NB_THREADS - 1);
	}

	else if (r > l + 2) {
		size_t m = (r + l) / 2;
		final_merge(a, n, l, m);
		final_merge(a, n, m, r);
		merge(a, l * n / NB_THREADS, m * n / NB_THREADS - 1, r * n / NB_THREADS - 1);
	}
}

void merge(min_loc_stra_t *a, size_t l, size_t m, size_t r) {
	size_t i, j;
	size_t n1 = m - l + 1;
	size_t n2 = r - m;

	min_loc_stra_t *L = (min_loc_stra_t *)malloc(n1 * sizeof(min_loc_stra_t));
	min_loc_stra_t *R = (min_loc_stra_t *)malloc(n2 * sizeof(min_loc_stra_t));
	if (L == NULL || R == NULL) {
		err(1, "malloc");
	}

	for (i = 0; i < n1; i++) {
		L[i] = a[l + i];
	}
	for (j = 0; j < n2; j++) {
		R[j] = a[m + 1 + j];
	}

	i        = 0;
	j        = 0;
	size_t k = l;

	while (i < n1 && j < n2) {
		if (compare(L[i], R[j])) {
			a[k] = L[i];
			i++;
		} else {
			a[k] = R[j];
			j++;
		}
		k++;
	}

	while (i < n1) {
		a[k] = L[i];
		i++;
		k++;
	}

	while (j < n2) {
		a[k] = R[j];
		j++;
		k++;
	}

	free(L);
	free(R);
}
