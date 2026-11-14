/*
 * PORT_HEAPCHECK=1 (debugging): GNU malloc's free fragment lists checked
 * at every retrace (main.c), to find when the game's heap goes bad.
 */
#include "port.h"

struct port_list {
    struct port_list *next, *prev;
};


extern struct port_list bm64__fraghead[] __attribute__((weak));


int port_heap_check(void) {

    if (!bm64__fraghead)
        return 0;
    for (int log = 0; log < 12; log++) {
        struct port_list *p = &bm64__fraghead[log];
        int n = 0;
        for (struct port_list *q = p->next; q != NULL; q = q->next) {
            if ((uint8_t *)q < port_rdram + 0x300000 || (uint8_t *)q >= port_rdram + 0x400000) {
                host_log("heap: fragment list %d: node %d at %p, outside the arena\n", log, n, (void *)q);
                return 1;
            }
            if (q->prev == NULL || q->prev->next != q) {
                host_log("heap: fragment list %d: node %d at %p: prev %p doesn't point back\n", log, n, (void *)q,
                         (void *)q->prev);
                return 1;
            }
            if (++n > 100000)
                break;
        }
    }

    return 0;
}
