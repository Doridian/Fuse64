/*
 * PORT_WARP: a stage reached directly (docs/PORT.md, "Reaching a stage").
 *
 * The game changes scene through 54E40.c's func_8023AE6C(scene, sub,
 * flags): it records the request (D_802AC5D4, D_802AC5D8) and the main
 * loop (func_8023DB0C) loads that scene's overlay and sets the scene up
 * from its number (func_8023D0F8: the world and the stage from
 * D_802A04C8's ranges, the battle stages 0x90-0x99).  With PORT_WARP=N
 * (decimal, or 0x hex) the first request for a story stage (scenes
 * 0x28-0x8F, the world select's) or a battle stage (0x90-0x99) is
 * changed to scene N, so that the game's own code loads it with the
 * state the menus left: a story scene from the story's way in
 * (tools/inputs/menus.txt), a battle stage from battle's
 * (tools/inputs/battle.<v>.txt), every round's.  `N:S` gives the
 * sub-mode too.
 * PORT_SCENE_LOG=1 writes every request to stderr.
 */
#include <stdio.h>
#include <stdlib.h>

extern unsigned host_vi_count;

void port_warp(int *scene, int *sub, int *flags) {
    static int init, target = -1, target_sub = -1, done;
    if (!init) {
        const char *e = getenv("PORT_WARP");
        init = 1;
        if (e && *e) {
            char *end;
            target = (int) strtol(e, &end, 0);
            if (*end == ':')
                target_sub = (int) strtol(end + 1, NULL, 0);
        }
        e = getenv("PORT_SCENE_LOG");
        if (e && *e && *e != '0')
            init = 2;
    }
    /* a story scene: the first request for one (the stage's own scenes
       follow from it); a battle stage: every round's */
    if (target >= 0x90 && target < 0x9A ? *scene >= 0x90 && *scene < 0x9A
                                         : target >= 0 && !done && *scene >= 0x28 && *scene < 0x9A) {
        done = 1;
        if (init == 2)
            fprintf(stderr, "warp: scene %d -> %d\n", *scene, target);
        *scene = target;
        if (target_sub >= 0)
            *sub = target_sub;
    }
    if (init == 2)
        fprintf(stderr, "scene %u: %d sub %d flags %d\n", host_vi_count, *scene, *sub, *flags);
}
