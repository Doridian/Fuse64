/*
 * The ROM when none is given on the command line (a package started from
 * its icon, docs/PORT.md "Packages"): ROM_DEFAULT in the working directory,
 * as always; else, with a window to come, the one the player picked before,
 * or the system's file dialog asks for it (Windows' open dialog,
 * osascript's `choose file` on macOS, zenity or kdialog on Linux), or,
 * where there is none of those, a window of the port's own takes it
 * dropped on it.  A ROM is taken only if it is this build's version and
 * gives the build its data (host_rom_fill_check: the bytes the strip took
 * out); another is refused and it asks again.  The choice is kept in the
 * player's data directory (SDL_GetPrefPath: ~/.local/share/Fuse64,
 * %APPDATA%\Fuse64, ~/Library/Application Support/Fuse64) as
 * rom-path.<v>.txt, and the save goes there too unless --save-dir says
 * otherwise.  Nothing else is looked at.
 *
 * From DozerEncore's port/host/romfind.c (the same author's, AGPL-3.0),
 * without its resource packs and its own font: the drop window says what
 * it wants in its title.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"
#include "host.h"

#if defined(VERSION_JP)
#define ROM_NAME "Baku Bomberman (Japan)"
#define ROM_VER "jp"
#elif defined(VERSION_EU)
#define ROM_NAME "Bomberman 64 (Europe)"
#define ROM_VER "eu"
#else
#define ROM_NAME "Bomberman 64 (USA)"
#define ROM_VER "us"
#endif

static int readable(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fclose(f);
    return 1;
}

#if defined(PORT_HAVE_SDL) && !defined(__EMSCRIPTEN__)

#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#else
#include <sys/wait.h>
#endif

void host_error_box(const char *msg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fuse64", msg, NULL);
}

/* this build's ROM (in any byte order): its game code, and the data the
   strip took out */
static int ours(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *rom = n >= 0x1000 && n <= 64 << 20 ? malloc((size_t)n) : NULL;
    int ok = rom && fread(rom, 1, (size_t)n, f) == (size_t)n && host_rom_ours(rom, (uint32_t)n, NULL, 0);
    fclose(f);
    free(rom);
    return ok;
}

static void chomp(char *s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
        s[--n] = 0;
}

#define PROMPT "Choose your " ROM_NAME " ROM (.z64, .v64 or .n64)"

/* the system's file dialog; the path picked (malloc'd), or NULL (*none:
   there is no dialog to ask with) */
static char *ask(int *none) {
    *none = 0;
#if defined(_WIN32)
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.lpstrFilter = "N64 ROMs (*.z64;*.v64;*.n64)\0*.z64;*.v64;*.n64\0All files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof path;
    ofn.lpstrTitle = PROMPT;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    return GetOpenFileNameA(&ofn) ? strdup(path) : NULL;
#else
    static const char *const cmds[] = {
#ifdef __APPLE__
        "osascript -e 'POSIX path of (choose file with prompt \"" PROMPT "\")' 2>/dev/null",
#else
        "zenity --file-selection --title='" PROMPT "' "
        "--file-filter='N64 ROMs | *.z64 *.v64 *.n64' --file-filter='All files | *' 2>/dev/null",
        "kdialog --getopenfilename . 'N64 ROMs (*.z64 *.v64 *.n64)' --title '" PROMPT "' 2>/dev/null",
#endif
    };
    for (size_t i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        FILE *p = popen(cmds[i], "r");
        if (!p)
            continue;
        char path[4096] = "";
        int got = fgets(path, sizeof path, p) != NULL;
        int rc = pclose(p);
        chomp(path);
        if (got && rc == 0 && *path)
            return strdup(path);
        if (rc != -1 && WIFEXITED(rc) && WEXITSTATUS(rc) != 127)
            return NULL;            /* (the dialog ran, and was cancelled) */
    }
    *none = 1;
    return NULL;
#endif
}

/* Where there is no dialog (no zenity or kdialog, as in a sandbox): a
   window that takes the ROM dropped on it, what it wants in its title (SDL
   2's message box needs zenity or the X server's core fonts too).  The
   file dropped (malloc'd); NULL: no window (closing it quits). */
static char *drop(const char *err) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        host_log("no file dialog, and no window either: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    char title[512];
    snprintf(title, sizeof title, "Fuse64: %s%sdrop your %s ROM on this window", err ? err : "",
             err ? " " : "", ROM_NAME);
    SDL_Window *win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 300, 0);
    char *got = NULL;
    if (win) {
        SDL_Surface *s = SDL_GetWindowSurface(win);
        if (s) {
            SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 0x20, 0x20, 0x24));
            SDL_UpdateWindowSurface(win);
        }
        SDL_Event e;
        while (!got && SDL_WaitEvent(&e)) {
            if (e.type == SDL_QUIT) {
                host_log("no ROM: start fuse64 with your %s ROM as its argument, or drop the ROM on its window\n",
                         ROM_NAME);
                exit(1);            /* (the player closed it: no box) */
            }
            if (e.type == SDL_DROPFILE) {
                got = strdup(e.drop.file);
                SDL_free(e.drop.file);
            } else if (e.type == SDL_WINDOWEVENT && s && e.window.event == SDL_WINDOWEVENT_EXPOSED) {
                SDL_UpdateWindowSurface(win);
            }
        }
        SDL_DestroyWindow(win);
    } else {
        host_log("no file dialog, and no window either: %s\n", SDL_GetError());
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    return got;
}

const char *host_find_rom(const char *def, const char **save_dir) {
    if (readable(def))
        return def;
    char *dir = SDL_GetPrefPath("", "Fuse64");
    if (!dir)
        return def;
    size_t n = strlen(dir) + 32;
    char *kept = malloc(n), *sv = malloc(n);
    snprintf(kept, n, "%srom-path." ROM_VER ".txt", dir);
    snprintf(sv, n, "%s", dir);
    SDL_free(dir);
    if (*sv && (sv[strlen(sv) - 1] == '/' || sv[strlen(sv) - 1] == '\\'))
        sv[strlen(sv) - 1] = 0;
    if (save_dir)
        *save_dir = sv;
    char path[4096] = "";
    FILE *f = fopen(kept, "r");
    if (f) {
        if (!fgets(path, sizeof path, f))
            *path = 0;
        fclose(f);
        chomp(path);
        if (*path && ours(path))
            return strdup(path);
    }
    char *picked = NULL;
    int none = 0;
    char err[256] = "";
    for (;;) {
        if (!none)
            picked = ask(&none);
        if (none)
            picked = drop(*err ? err : NULL);
        if (!picked)
            host_fatal("no ROM: start fuse64 with your %s ROM as its argument, or pick the ROM when it asks",
                       ROM_NAME);
        if (ours(picked))
            break;
        if (none) {
            const char *base = strrchr(picked, '/');
            snprintf(err, sizeof err, "%s isn't the %s ROM.", base ? base + 1 : picked, ROM_NAME);
        } else {
            host_error_box("That isn't the " ROM_NAME " ROM this build is for. Choose another.");
        }
        free(picked);
    }
    if ((f = fopen(kept, "w"))) {
        fprintf(f, "%s\n", picked);
        fclose(f);
    }
    return picked;
}

#else

void host_error_box(const char *msg) {
    (void)msg;
}

const char *host_find_rom(const char *def, const char **save_dir) {
    (void)save_dir;
    return def;
}

#endif
