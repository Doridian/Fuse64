/*
 * The VI's picture: the framebuffer the game shows (16-bit RGBA5551 in the
 * N64's byte order, as the RDP writes it), to a window (SDL) or, headless,
 * to screenshots and hashes.  The keyboard and game controllers come in
 * through the window's events.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

#ifdef PORT_HAVE_SDL
#include <SDL.h>
static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_GameController *pads[4];
#endif

static const uint8_t *vi_fb;
static int vi_width = 320;
static int quit;

extern uint16_t host_live_buttons[4];
extern int8_t host_live_x[4], host_live_y[4];
extern int host_live_present[4];

void host_vi_set_framebuffer(const void *fb, int width) {
    vi_fb = fb;
    vi_width = width > 0 && width <= 640 ? width : 320;
}

int host_quit_requested(void) { return quit; }

/* the shown picture as 32-bit XRGB, 320x240 */
static void to_rgb(uint32_t *out) {
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++) {
            uint32_t c = 0;
            if (vi_fb) {
                uint16_t p = port_be16(vi_fb + ((size_t)y * vi_width + x) * 2);
                uint32_t r = (p >> 11) & 31, g = (p >> 6) & 31, b = (p >> 1) & 31;
                c = (r << 19 | (r >> 2) << 16) | (g << 11 | (g >> 2) << 8) | (b << 3 | b >> 2);
            }
            out[y * 320 + x] = c;
        }
}

uint32_t host_frame_hash(void) {
    uint32_t h = 2166136261u;
    if (!vi_fb)
        return 0;
    for (size_t i = 0; i < (size_t)vi_width * 240 * 2; i++)
        h = (h ^ vi_fb[i]) * 16777619u;
    return h;
}

void host_frame_shot(const char *path) {
    static uint32_t px[320 * 240];
    uint8_t hdr[54] = { 'B', 'M' };
    FILE *f = fopen(path, "wb");
    if (!f) {
        host_log("can't write %s\n", path);
        return;
    }
    to_rgb(px);
    port_wle32(hdr + 2, 54 + 320 * 240 * 3);
    port_wle32(hdr + 10, 54);
    port_wle32(hdr + 14, 40);
    port_wle32(hdr + 18, 320);
    port_wle32(hdr + 22, 240);
    port_wle16(hdr + 26, 1);
    port_wle16(hdr + 28, 24);
    port_wle32(hdr + 34, 320 * 240 * 3);
    fwrite(hdr, 1, sizeof hdr, f);
    for (int y = 239; y >= 0; y--)
        for (int x = 0; x < 320; x++) {
            uint32_t c = px[y * 320 + x];
            uint8_t b[3] = { (uint8_t)c, (uint8_t)(c >> 8), (uint8_t)(c >> 16) };
            fwrite(b, 1, 3, f);
        }
    fclose(f);
}

void host_video_init(void) {
#ifdef PORT_HAVE_SDL
    if (host_headless)
        return;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        host_log("SDL: %s: running headless\n", SDL_GetError());
        host_headless = 1;
        return;
    }
    win = SDL_CreateWindow("Fuse64", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
                           SDL_WINDOW_RESIZABLE);
    if (!win) {
        host_log("SDL: %s: running headless\n", SDL_GetError());
        host_headless = 1;
        return;
    }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(ren, 320 * 3, 240 * 3);
    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
#endif
}

#ifdef PORT_HAVE_SDL
static void keyboard(void) {
    const uint8_t *k = SDL_GetKeyboardState(NULL);
    uint16_t b = 0;
    int x = 0, y = 0;
    if (k[SDL_SCANCODE_X] || k[SDL_SCANCODE_SPACE]) b |= B_A;
    if (k[SDL_SCANCODE_C] || k[SDL_SCANCODE_LCTRL]) b |= B_B;
    if (k[SDL_SCANCODE_Z]) b |= B_Z;
    if (k[SDL_SCANCODE_RETURN]) b |= B_START;
    if (k[SDL_SCANCODE_A]) b |= B_L;
    if (k[SDL_SCANCODE_S]) b |= B_R;
    if (k[SDL_SCANCODE_I]) b |= B_CU;
    if (k[SDL_SCANCODE_K]) b |= B_CD;
    if (k[SDL_SCANCODE_J]) b |= B_CL;
    if (k[SDL_SCANCODE_L]) b |= B_CR;
    if (k[SDL_SCANCODE_T]) b |= B_DU;
    if (k[SDL_SCANCODE_G]) b |= B_DD;
    if (k[SDL_SCANCODE_F]) b |= B_DL;
    if (k[SDL_SCANCODE_H]) b |= B_DR;
    if (k[SDL_SCANCODE_LEFT]) x -= 80;
    if (k[SDL_SCANCODE_RIGHT]) x += 80;
    if (k[SDL_SCANCODE_UP]) y += 80;
    if (k[SDL_SCANCODE_DOWN]) y -= 80;
    /* pad 0: the keyboard, or the first game controller */
    for (int i = 0; i < 4; i++) {
        if (!pads[i])
            continue;
        SDL_GameController *c = pads[i];
        uint16_t pb = 0;
        static const struct { int s; uint16_t n; } map[] = {
            { SDL_CONTROLLER_BUTTON_A, B_A }, { SDL_CONTROLLER_BUTTON_B, B_B },
            { SDL_CONTROLLER_BUTTON_X, B_B }, { SDL_CONTROLLER_BUTTON_START, B_START },
            { SDL_CONTROLLER_BUTTON_LEFTSHOULDER, B_L }, { SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, B_R },
            { SDL_CONTROLLER_BUTTON_DPAD_UP, B_DU }, { SDL_CONTROLLER_BUTTON_DPAD_DOWN, B_DD },
            { SDL_CONTROLLER_BUTTON_DPAD_LEFT, B_DL }, { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, B_DR },
        };
        for (unsigned j = 0; j < sizeof map / sizeof map[0]; j++)
            if (SDL_GameControllerGetButton(c, map[j].s))
                pb |= map[j].n;
        if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000)
            pb |= B_Z;
        int rx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX);
        int ry = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY);
        if (rx > 16000) pb |= B_CR;
        if (rx < -16000) pb |= B_CL;
        if (ry > 16000) pb |= B_CD;
        if (ry < -16000) pb |= B_CU;
        int ax = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX) * 80 / 32767;
        int ay = -SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY) * 80 / 32767;
        host_live_present[i] = 1;
        if (i == 0) {
            b |= pb;
            if (ax * ax + ay * ay > x * x + y * y) {
                x = ax;
                y = ay;
            }
        } else {
            host_live_buttons[i] = pb;
            host_live_x[i] = (int8_t)ax;
            host_live_y[i] = (int8_t)ay;
        }
    }
    host_live_buttons[0] = b;
    host_live_x[0] = (int8_t)x;
    host_live_y[0] = (int8_t)y;
}
#endif

void host_video_frame(void) {
    if (host_screenshot_prefix && host_screenshot_every && host_vi_count % host_screenshot_every == 0) {
        char path[1024];
        snprintf(path, sizeof path, "%s_%05u.bmp", host_screenshot_prefix, host_vi_count);
        host_frame_shot(path);
    }
#ifdef PORT_HAVE_SDL
    if (!win)
        return;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            quit = 1;
        else if (e.type == SDL_CONTROLLERDEVICEADDED) {
            for (int i = 0; i < 4; i++)
                if (!pads[i]) {
                    pads[i] = SDL_GameControllerOpen(e.cdevice.which);
                    break;
                }
        } else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            quit = 1;
    }
    keyboard();
    static uint32_t px[320 * 240];
    to_rgb(px);
    SDL_UpdateTexture(tex, NULL, px, 320 * 4);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
#endif
}

void host_video_shutdown(void) {
#ifdef PORT_HAVE_SDL
    if (win) {
        SDL_DestroyWindow(win);
        SDL_Quit();
    }
#endif
}
