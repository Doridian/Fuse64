/*
 * The game's malloc and realloc (src/libmalloc/pc_names.h).  The game's C
 * declares them with a 32-bit size (`void *malloc(u32);`); gmalloc.c's take
 * a size_t.  Called through the game's declaration, a 64-bit host's callee
 * would read the whole register, whose high half the caller needn't have
 * set (port/tools/proto_check.py, docs/PORT.md "Where pointers are 8
 * bytes"): these take the 32-bit size and widen it.
 */
#include <stddef.h>

void *bm64_malloc(size_t n);
void *bm64_realloc(void *p, size_t n);

void *bm64_malloc32(unsigned int n) {
    return bm64_malloc(n);
}

void *bm64_realloc32(void *p, unsigned int n) {
    return bm64_realloc(p, n);
}
