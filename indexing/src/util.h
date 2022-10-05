#ifndef UTIL_H
#define UTIL_H

#include <err.h>
#include <stdlib.h>

#define MALLOC(var, type, size)                                                                                        \
	{                                                                                                              \
		var = (type *)malloc(sizeof(type) * size);                                                             \
		if (var == NULL) {                                                                                     \
			err(1, "%s:%d, malloc", __FILE__, __LINE__);                                                   \
		}                                                                                                      \
	}

#define REALLOC(var, type, size)                                                                                       \
	{                                                                                                              \
		var = (type *)realloc(var, sizeof(type) * size);                                                       \
		if (var == NULL) {                                                                                     \
			err(1, "%s:%d, realloc", __FILE__, __LINE__);                                                  \
		}                                                                                                      \
	}

#endif
