#ifndef DEMETER_UTIL_H
#define DEMETER_UTIL_H

#include "../src/parsing.h"
#include <err.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>

#define MALLOC(var, type, size)                                                                                        \
	{                                                                                                              \
		var = (type *)malloc(sizeof(type) * size);                                                             \
		if (var == NULL) {                                                                                     \
			err(1, "%s:%d, malloc", __FILE__, __LINE__);                                                   \
		}                                                                                                      \
	}

#define CALLOC(var, type, size)                                                                                        \
	{                                                                                                              \
		var = (type *)calloc(size, sizeof(type));                                                              \
		if (var == NULL) {                                                                                     \
			err(1, "%s:%d, calloc", __FILE__, __LINE__);                                                   \
		}                                                                                                      \
	}

#define REALLOC(var, type, size)                                                                                       \
	{                                                                                                              \
		var = (type *)realloc(var, sizeof(type) * size);                                                       \
		if (var == NULL) {                                                                                     \
			err(1, "%s:%d, realloc", __FILE__, __LINE__);                                                  \
		}                                                                                                      \
	}

#define MUTEX_INIT(mutex)                                                                                              \
	{ pthread_mutex_init(&mutex, NULL); }

#define LOCK(mutex)                                                                                                    \
	{                                                                                                              \
		if (pthread_mutex_lock(&mutex)) {                                                                      \
			errx(1, "%s:%d, pthread_mutex_lock", __FILE__, __LINE__);                                      \
		}                                                                                                      \
	}

#define TRYLOCK(mutex) pthread_mutex_trylock(&mutex)

#define UNLOCK(mutex)                                                                                                  \
	{                                                                                                              \
		if (pthread_mutex_unlock(&mutex)) {                                                                    \
			errx(1, "%s:%d, pthread_mutex_unlock", __FILE__, __LINE__);                                    \
		}                                                                                                      \
	}

#define SEM_INIT(sem, val)                                                                                             \
	{                                                                                                              \
		if (sem_init(&sem, 0, val)) {                                                                          \
			err(1, "%s:%d, sem_init", __FILE__, __LINE__);                                                 \
		}                                                                                                      \
	}

#define THREAD_START(thread, func, arg)                                                                                \
	{                                                                                                              \
		if (pthread_create(&thread, NULL, func, (void *)arg)) {                                                \
			errx(1, "%s:%d, pthread_create", __FILE__, __LINE__);                                          \
		}                                                                                                      \
	}

#define THREAD_JOIN(thread)                                                                                            \
	{                                                                                                              \
		if (pthread_join(thread, NULL)) {                                                                      \
			errx(1, "%s:%d, pthread_join", __FILE__, __LINE__);                                            \
		}                                                                                                      \
	}

#define MUTEX_DESTROY(mutex)                                                                                           \
	{                                                                                                              \
		if (pthread_mutex_destroy(&mutex)) {                                                                   \
			errx(1, "%s:%d, pthread_mutex_destroy", __FILE__, __LINE__);                                   \
		}                                                                                                      \
	}

#define SEM_DESTROY(sem)                                                                                               \
	{                                                                                                              \
		if (sem_destroy(&sem)) {                                                                               \
			err(1, "%s:%d, sem_destroy", __FILE__, __LINE__);                                              \
		}                                                                                                      \
	}

#define SEM_WAIT(sem)                                                                                                  \
	{                                                                                                              \
		if (sem_wait(&sem)) {                                                                                  \
			err(1, "%s:%d, sem_wait", __FILE__, __LINE__);                                                 \
		}                                                                                                      \
	}

#define SEM_TRYWAIT(sem) sem_trywait(&sem)

#define SEM_POST(sem)                                                                                                  \
	{                                                                                                              \
		if (sem_post(&sem)) {                                                                                  \
			err(1, "%s:%d, sem_post", __FILE__, __LINE__);                                                 \
		}                                                                                                      \
	}

#define FREAD(ptr, type, nmemb, stream)                                                                                \
	{                                                                                                              \
		if (fread(ptr, sizeof(type), nmemb, stream) != nmemb) {                                                \
			errx(1, "%s:%d, fread", __FILE__, __LINE__);                                                   \
		}                                                                                                      \
	}

#endif
