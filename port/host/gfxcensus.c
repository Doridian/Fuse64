/*
 * PORT_GFX_CENSUS=FILE: a census of what the graphics tasks use, for
 * port/tools/gbi_census.py (docs/PORT.md, "Graphics", "The GBI census").
 * gfx.c names each thing it meets as a short key ("dl BF", "prim tri.ST
 * cyc=1 ...", "load block fmt=0 siz=2 w=32", ...) and this counts them; at
 * the exit FILE gets one line per key: the count, the first retrace it was
 * seen at, the key.  It reads nothing the game sees and changes nothing it
 * draws (off, it costs a test of host_census_on).
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

int host_census_on;

static const char *census_path;

typedef struct {
    char *key;
    uint64_t n;
    unsigned first;
} Entry;

static Entry *tab;
static size_t cap, used;

static uint32_t hash(const char *s) {
    uint32_t h = 2166136261u;
    for (; *s; s++)
        h = (h ^ (uint8_t)*s) * 16777619u;
    return h;
}

static void put(char *key, uint64_t n, unsigned first) {
    size_t i = hash(key) & (cap - 1);
    while (tab[i].key)
        i = (i + 1) & (cap - 1);
    tab[i].key = key;
    tab[i].n = n;
    tab[i].first = first;
}

static void grow(void) {
    Entry *old = tab;
    size_t oc = cap;
    cap = cap ? cap * 2 : 4096;
    tab = calloc(cap, sizeof *tab);
    if (!tab)
        host_fatal("PORT_GFX_CENSUS: no memory");
    for (size_t i = 0; i < oc; i++)
        if (old[i].key)
            put(old[i].key, old[i].n, old[i].first);
    free(old);
}

void host_census(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if ((used + 1) * 2 > cap)
        grow();
    size_t i = hash(buf) & (cap - 1);
    while (tab[i].key) {
        if (!strcmp(tab[i].key, buf)) {
            tab[i].n++;
            return;
        }
        i = (i + 1) & (cap - 1);
    }
    tab[i].key = strdup(buf);
    tab[i].n = 1;
    tab[i].first = host_vi_count;
    used++;
}

static void finish(void) {
    FILE *f = fopen(census_path, "w");
    if (!f) {
        host_log("PORT_GFX_CENSUS: can't write %s\n", census_path);
        return;
    }
    for (size_t i = 0; i < cap; i++)
        if (tab[i].key)
            fprintf(f, "%llu\t%u\t%s\n", (unsigned long long)tab[i].n, tab[i].first, tab[i].key);
    fclose(f);
}

void host_census_init(void) {
    const char *e = getenv("PORT_GFX_CENSUS");
    if (!e || !*e)
        return;
    census_path = e;
    host_census_on = 1;
    grow();
    atexit(finish);
}
