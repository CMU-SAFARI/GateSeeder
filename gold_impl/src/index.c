#include "index.h"
#include "kvec.h"
#include "mmpriv.h"
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#define BUFFER_SIZE 4294967296
#define NB_THREADS 6

static inline unsigned char compare(mm72_t left, mm72_t right) { return (left.minimizer) <= (right.minimizer); }

void create_cindex(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int filter_threshold,
                   const unsigned int b, cindex_t *idx) {
	mm72_v *p = (mm72_v *)calloc(1, sizeof(mm72_v));

	// Parse & sketch
	parse_sketch(fp, w, k, b, p);

	// Sort p
	sort(p);
	printf("Info: Array sorted\n");

	// Write the data in the struct & filter out the most frequent minimizers
	idx->h        = (uint32_t *)malloc(sizeof(uint32_t) * (1 << b));
	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * p->n);
	if (idx->h == NULL || idx->location == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	unsigned int diff_counter   = 0;
	uint32_t index              = p->a[0].minimizer;
	unsigned int freq_counter   = 0;
	unsigned int filter_counter = 0;
	size_t pos                  = 0;
	uint32_t l                  = 0;

	for (size_t i = 0; i < index; i++) {
		idx->h[i] = 0;
	}

	for (size_t i = 1; i < p->n; i++) {
		if (index == p->a[i].minimizer) {
			freq_counter++;
		} else {
			if (freq_counter < filter_threshold) {
				diff_counter++;
				for (size_t j = pos; j < i; j++) {
					idx->location[l] = (p->a[j].location & (UINT32_MAX - 1)) | p->a[j].strand;
					l++;
				}
			} else {
				filter_counter++;
			}
			pos          = i;
			freq_counter = 0;
			while (index != p->a[i].minimizer) {
				idx->h[index] = l;
				index++;
			}
		}
	}
	if (freq_counter < filter_threshold) {
		diff_counter++;
		for (size_t j = pos; j < p->n; j++) {
			idx->location[l] = (p->a[j].location & (UINT32_MAX - 1)) | p->a[j].strand;
			l++;
		}
	} else {
		filter_counter++;
	}

	for (size_t i = index; i < (1 << b); i++) {
		idx->h[i] = l;
	}

	kv_destroy(*p);
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
	return;
}

void create_index(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int filter_threshold,
                  const unsigned int b, index_t *idx) {
	mm72_v *p = (mm72_v *)calloc(1, sizeof(mm72_v));

	// Parse & sketch
	parse_sketch(fp, w, k, b, p);

	// Sort p
	sort(p);
	printf("Info: Array sorted\n");

	// Write the data in the struct & filter out the most frequent minimizers
	idx->h        = (uint32_t *)malloc(sizeof(uint32_t) * (1 << b));
	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * p->n);
	idx->strand   = (uint8_t *)malloc(sizeof(uint8_t) * p->n);
	if (idx->h == NULL || idx->location == NULL || idx->strand == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	unsigned int diff_counter   = 0;
	uint32_t index              = p->a[0].minimizer;
	unsigned int freq_counter   = 0;
	unsigned int filter_counter = 0;
	size_t pos                  = 0;
	uint32_t l                  = 0;

	for (size_t i = 0; i < index; i++) {
		idx->h[i] = 0;
	}

	for (size_t i = 1; i < p->n; i++) {
		if (index == p->a[i].minimizer) {
			freq_counter++;
		} else {
			if (freq_counter < filter_threshold) {
				diff_counter++;
				for (size_t j = pos; j < i; j++) {
					idx->location[l] = p->a[j].location;
					idx->strand[l]   = p->a[j].strand;
					l++;
				}
			} else {
				filter_counter++;
			}
			pos          = i;
			freq_counter = 0;
			while (index != p->a[i].minimizer) {
				idx->h[index] = l;
				index++;
			}
		}
	}
	if (freq_counter < filter_threshold) {
		diff_counter++;
		for (size_t j = pos; j < p->n; j++) {
			idx->location[l] = p->a[j].location;
			idx->strand[l]   = p->a[j].strand;
			l++;
		}
	} else {
		filter_counter++;
	}

	for (size_t i = index; i < (1 << b); i++) {
		idx->h[i] = l;
	}

	kv_destroy(*p);
	printf("Info: Number of ignored minimizers: %u\n", filter_counter);
	printf("Info: Number of distinct minimizers: %u\n", diff_counter);
	idx->n              = 1 << b;
	idx->m              = l;
	float strand_size   = (float)idx->m / (1 << 30);
	float location_size = strand_size * 4;
	float hash_size     = (float)idx->n / (1 << 28);
	float average       = (float)idx->m / idx->n;

	printf("Info: Number of (location, strand): %u\n", idx->m);
	printf("Info: Size of the strand array: %fGB\n", strand_size);
	printf("Info: Size of the location array: %fGB\n", location_size);
	printf("Info: Size of the strand array: %fGB\n", strand_size);
	printf("Info: Size of the minimizer array: %fGB\n", hash_size);
	printf("Info: Total size: %fGB\n", location_size + strand_size + hash_size);
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
	return;
}

void create_raw_index(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int filter_threshold,
                      const unsigned int b, mm72_v *p) {
	p->n = 0;
	p->m = 0;
	// Parse & sketch
	parse_sketch(fp, w, k, b, p);

	// Sort p
	sort(p);
	printf("Info: Array sorted\n");
	printf("Info: Total size: %fGB\n", (float)p->n * 9 / (1 << 30));
	return;
}

void read_cindex(FILE *fp, cindex_t *idx) {
	fread(&idx->n, sizeof(uint32_t), 1, fp);
	fprintf(stderr, "Info: Number of minimizers: %u\n", idx->n);

	idx->h = (uint32_t *)malloc(sizeof(uint32_t) * idx->n);
	if (idx->h == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->h, sizeof(uint32_t), idx->n, fp);
	idx->m = idx->h[idx->n - 1];
	fprintf(stderr, "Info: Size of the location & strand arrays: %u\n", idx->m);

	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * idx->m);
	if (idx->location == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->location, sizeof(uint32_t), idx->m, fp);

	// Check if we reached the EOF
	uint8_t eof;
	fread(&eof, sizeof(uint8_t), 1, fp);
	if (!feof(fp)) {
		fputs("Reading error: wrong file format\n", stderr);
		fclose(fp);
		exit(3);
	}

	fclose(fp);
	return;
}

void read_index(FILE *fp, index_t *idx) {
	fread(&idx->n, sizeof(uint32_t), 1, fp);
	fprintf(stderr, "Info: Number of minimizers: %u\n", idx->n);

	idx->h = (uint32_t *)malloc(sizeof(uint32_t) * idx->n);
	if (idx->h == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->h, sizeof(uint32_t), idx->n, fp);
	idx->m = idx->h[idx->n - 1];
	fprintf(stderr, "Info: Size of the location & strand arrays: %u\n", idx->m);

	idx->location = (uint32_t *)malloc(sizeof(uint32_t) * idx->m);
	if (idx->location == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->location, sizeof(uint32_t), idx->m, fp);

	idx->strand = (uint8_t *)malloc(sizeof(uint8_t) * idx->m);
	if (idx->strand == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}
	fread(idx->strand, sizeof(uint8_t), idx->m, fp);

	// Check if we reached the EOF
	uint8_t eof;
	fread(&eof, sizeof(uint8_t), 1, fp);
	if (!feof(fp)) {
		fputs("Reading error: wrong file format\n", stderr);
		fclose(fp);
		exit(3);
	}

	fclose(fp);
	return;
}

void parse_sketch(FILE *fp, const unsigned int w, const unsigned int k, const unsigned int b, mm72_v *p) {
	char *read_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	if (read_buffer == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	char *dna_buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
	if (dna_buffer == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
	}

	fread(read_buffer, sizeof(char), BUFFER_SIZE, fp);
	if (!feof(fp)) {
		fputs("Reading error: buffer too small\n", stderr);
		fclose(fp);
		exit(3);
	}

	size_t i          = 0;
	size_t dna_len    = 0;
	size_t chromo_len = 0;

	while (1) {
		char c = read_buffer[i];

		if (c == '>') {
			if (chromo_len > 0) {
				mm_sketch(0, dna_buffer, chromo_len, w, k, b, p, dna_len);
				dna_len += chromo_len;
				chromo_len = 0;
			}
			while (c != '\n' && c != 0) {
				i++;
				c = read_buffer[i];
			}
		} else {
			while (c != '\n' && c != 0) {
				dna_buffer[chromo_len] = c;
				chromo_len++;
				i++;
				c = read_buffer[i];
			}
		}

		if (c == 0) {
			break;
		}
		i++;
	}

	free(read_buffer);
	fclose(fp);
	if (chromo_len > 0) {
		mm_sketch(0, dna_buffer, chromo_len, w, k, b, p, dna_len);
		dna_len += chromo_len;
	}

	printf("Info: Indexed DNA length: %lu bases\n", dna_len);
	printf("Info: Number of (minimizer, location, strand): %lu\n", p->n);
	free(dna_buffer);
}

void sort(mm72_v *p) {
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

void merge_sort(mm72_t *a, size_t l, size_t r) {
	if (l < r) {
		size_t m = l + (r - l) / 2;
		merge_sort(a, l, m);
		merge_sort(a, m + 1, r);
		merge(a, l, m, r);
	}
}

void final_merge(mm72_t *a, size_t n, size_t l, size_t r) {
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

void merge(mm72_t *a, size_t l, size_t m, size_t r) {
	size_t i, j;
	size_t n1 = m - l + 1;
	size_t n2 = r - m;

	mm72_t *L = (mm72_t *)malloc(n1 * sizeof(mm72_t));
	mm72_t *R = (mm72_t *)malloc(n2 * sizeof(mm72_t));
	if (L == NULL || R == NULL) {
		fputs("Memory error\n", stderr);
		exit(2);
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