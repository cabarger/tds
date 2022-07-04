#ifndef LINUX_TDS_H

#include "tds_gl.h"
#include <X11/Xlib.h>
#include <GL/glx.h>

typedef Display linux_display;
typedef Window linux_window;
typedef GLXContext linux_glx_context;

struct linux_game_code
{
    void *LibraryHandle;
    ino_t FileID;
    game_update_and_render *UpdateAndRender;
};

struct linux_state
{
    linux_display *Display;
    linux_window Window;
    linux_glx_context GC;
    s64 EventMask;

    linux_game_code GameCode;
};

#define KEYCODE_W           25
#define KEYCODE_A           38
#define KEYCODE_S           39
#define KEYCODE_D           40
#define KEYCODE_Q           24
#define KEYCODE_E           26
#define KEYCODE_UP          111
#define KEYCODE_DOWN        116
#define KEYCODE_LEFT        113
#define KEYCODE_RIGHT       114
#define KEYCODE_ESCAPE      66
#define KEYCODE_ENTER       36
#define KEYCODE_SPACE       65
#define KEYCODE_P           33
#define KEYCODE_L           46

#define LINUX_TDS_H
#endif
