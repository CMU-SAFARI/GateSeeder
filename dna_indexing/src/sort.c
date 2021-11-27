#include "sort.h"
#include "extraction.h"
#include <err.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef NB_THREADS
#define NB_THREADS 8
#endif

static inline unsigned char compare(min_loc_stra_t left, min_loc_stra_t right) { return left.min <= right.min; }

void sort(min_loc_stra_v *p) {
	pthread_t threads[NB_THREADS];
	thread_sort_t params[NB_THREADS];
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
	thread_sort_t *param = (thread_sort_t *)arg;
	size_t l             = param->i * param->p->n / NB_THREADS;
	size_t r             = (param->i + 1) * param->p->n / NB_THREADS - 1;
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
