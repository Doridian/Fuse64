/*
 * -DPORT_HOST_MALLOC=ON (a debugging build): the game's allocator (GNU
 * malloc in the sbrk arena, src/libmalloc, pc_names.h's bm64_ names) is
 * the host's instead, so that AddressSanitizer sees every block the game
 * allocates.  Not for play: the heap's addresses then aren't the
 * game's own layout (docs/NETPLAY.md wants that).
 */
#ifdef PORT_HOST_MALLOC
#include <stddef.h>

void *malloc(size_t n);
void free(void *p);
void *realloc(void *p, size_t n);
void *calloc(size_t n, size_t m);

int bm64___malloc_initialized;

void *bm64_malloc(size_t n) { return malloc(n ? n : 1); }
void bm64_free(void *p) { free(p); }
void *bm64_realloc(void *p, size_t n) { return realloc(p, n); }
void *bm64_calloc(size_t n, size_t m) { return calloc(n, m); }
#endif
