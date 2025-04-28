//
// Created by Matt on 4/27/2025.
//

#include <assert.h>
#include <stdlib.h>

extern void * r_malloc(size_t size) {
	void * result = malloc(size);
	assert(result != NULL);
	return result;
}

extern void * r_calloc(size_t n, size_t size) {
	void * result = calloc(n, size);
	assert(result != NULL);
	return result;
}

extern void * r_recalloc(void * ptr, size_t n, size_t size) {
	void * result = realloc(ptr, n * size);
	assert(result != NULL);
	return result;
}

extern void * r_realloc(void * ptr, size_t new_size) {
	void * result = realloc(ptr, new_size);
	assert(result != NULL);
	return result;
}

extern void * r_free(void * ptr) {
	if (ptr != NULL) {
		free(ptr);
	}
	return NULL;
}
