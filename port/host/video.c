/*
 * The VI's picture (vi.c) to a window (SDL) or, headless, to screenshots
 * and hashes.  With the OpenGL renderer the window has an OpenGL context,
 * the renderer's, and shows the picture the GPU made (vi.c, host_vi_show);
 * else SDL's renderer shows it.  The keyboard and game controllers come in
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
static int tex_w, tex_h;
static SDL_GameController *pads[4];
static SDL_GLContext glctx;             /* the OpenGL renderer's, the window's */
#endif

/* the OpenGL renderer draws with the window's context (gfx_gl.c), and the
   VI's picture is shown from it (vi.c, host_vi_show) */
int host_video_gl(void) {
#ifdef PORT_HAVE_SDL
    return glctx != NULL;
#else
    return 0;
#endif
}

static int quit;

extern uint16_t host_live_buttons[4];
extern int8_t host_live_x[4], host_live_y[4];
extern int host_live_present[4];

int host_quit_requested(void) { return quit; }

/* FNV-1a of the shown picture (--vi-raw: of the framebuffer's bytes, 0
   while there is none) */
uint32_t host_frame_hash(void) {
    uint32_t h = 2166136261u;
    if (host_vi_raw) {
        const uint8_t *fb = host_vi_raw_fb();
        if (!fb)
            return 0;
        host_gfx_sync();
        for (size_t i = 0; i < 320 * 240 * 2; i++)
            h = (h ^ fb[i]) * 16777619u;
        return h;
    }
    int w, ht;
    const uint32_t *px = host_vi_picture(&w, &ht);
    for (size_t i = 0; i < (size_t)w * ht; i++)
        h = (h ^ px[i]) * 16777619u;
    return h;
}

/* the picture as a BMP; the VI's lines twice (a field's lines are two of
   the screen's) */
void host_frame_shot(const char *path) {
    int w, h;
    const uint32_t *px = host_vi_picture(&w, &h);
    int rep = host_vi_raw ? 1 : 2;
    uint32_t size = (uint32_t)(w * 3) * (uint32_t)(h * rep);
    uint8_t hdr[54] = { 'B', 'M' };
    FILE *f = fopen(path, "wb");
    if (!f) {
        host_log("can't write %s\n", path);
        return;
    }
    port_wle32(hdr + 2, 54 + size);
    port_wle32(hdr + 10, 54);
    port_wle32(hdr + 14, 40);
    port_wle32(hdr + 18, (uint32_t)w);
    port_wle32(hdr + 22, (uint32_t)(h * rep));
    port_wle16(hdr + 26, 1);
    port_wle16(hdr + 28, 24);
    port_wle32(hdr + 34, size);
    fwrite(hdr, 1, sizeof hdr, f);
    for (int y = h * rep - 1; y >= 0; y--)
        for (int x = 0; x < w; x++) {
            uint32_t c = px[(y / rep) * w + x];
            uint8_t b[3] = { (uint8_t)c, (uint8_t)(c >> 8), (uint8_t)(c >> 16) };
            fwrite(b, 1, 3, f);
        }
    fclose(f);
}

void host_video_init(void) {
#ifdef PORT_HAVE_SDL
    if (host_headless)
        return;
    /* (main is the program's own, not SDL_main: SDL2 on Windows refuses to
       start without this) */
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        host_log("SDL: %s: running headless\n", SDL_GetError());
        host_headless = 1;
        return;
    }
#ifdef PORT_HAVE_GL
    /* the OpenGL renderer: a window with an OpenGL 3.3 core context, which
       the renderer draws with and the picture is shown from */
    if (host_gfx_gl()) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
        win = SDL_CreateWindow("Fuse64", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        if (win) {
            glctx = SDL_GL_CreateContext(win);
            if (glctx && SDL_GL_MakeCurrent(win, glctx) == 0) {
                SDL_GL_SetSwapInterval(1);
                return;
            }
        }
        /* (else the renderer's own context, offscreen, and the picture read
           back for SDL's software renderer, as without the window's) */
        host_log("SDL: no window with an OpenGL 3.3 core context (%s): the picture is read back\n",
                 SDL_GetError());
        if (glctx)
            SDL_GL_DeleteContext(glctx);
        glctx = NULL;
        if (win)
            SDL_DestroyWindow(win);
        win = NULL;
    }
#endif
    win = SDL_CreateWindow("Fuse64", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 720,
                           SDL_WINDOW_RESIZABLE);
    if (!win) {
        host_log("SDL: %s: running headless\n", SDL_GetError());
        host_headless = 1;
        return;
    }
    /* (without the window's context, the OpenGL renderer's is current on
       this thread behind SDL's back: SDL's renderer then mustn't be
       OpenGL's) */
    if (host_gfx_gl())
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!ren)
        ren = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(ren, 320 * 3, 240 * 3);
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
    host_vi_frame();
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
        else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_F9 && !e.key.repeat)
            host_reset_button();        /* the console's reset button (main.c) */
    }
    keyboard();
    if (glctx) {
        int dw, dh;
        SDL_GL_GetDrawableSize(win, &dw, &dh);
        host_vi_show(dw, dh);
        SDL_GL_SwapWindow(win);
        return;
    }
    /* the picture at 4:3, whatever its pixels */
    int w, h;
    const uint32_t *px = host_vi_picture(&w, &h);
    if (!tex || w != tex_w || h != tex_h) {
        if (tex)
            SDL_DestroyTexture(tex);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        tex_w = w;
        tex_h = h;
    }
    SDL_UpdateTexture(tex, NULL, px, w * 4);
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
#endif
}

void host_video_shutdown(void) {
#ifdef PORT_HAVE_SDL
    if (win) {
        if (glctx)
            SDL_GL_DeleteContext(glctx);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }
#endif
}
