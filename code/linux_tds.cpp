#include <X11/Xlib.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef uintptr_t umm;
typedef intptr_t smm;

typedef size_t memory_index;

typedef float r32;
typedef double r64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef int32_t b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define internal static
#define global_variable static
#define local_persist static

typedef Display linux_display;
typedef Window linux_window;
typedef GC linux_gc;

#define LINUX_ESCAPE_KEY 66
#define LINUX_D_KEY 40

struct button_state
{
    b32 WasDown;
    b32 IsDown;
};

struct controller
{
    button_state Right;
};

struct game_state
{
    controller Controller;
};

struct linux_state
{
    linux_display *Display;
    linux_window Window;
    linux_gc GraphicsContext;
    s64 EventMask;

    controller Controller;
};

global_variable b32 GlobalRunning;

internal void
LinuxProcessButtonPress(s32 EventType, button_state *Button)
{
    if (EventType == ButtonPress)
    {
        Button->IsDown = true;
    }
    else
    {
        Button->IsDown = false;
    }
}

internal void
LinuxProcessKey(XKeyEvent XKey, controller *Controller)
{
    switch(XKey.keycode)
    {
        case LINUX_ESCAPE_KEY:
        {
            GlobalRunning = false;
        } break;
        case LINUX_D_KEY:
        {
            LinuxProcessButtonPress(XKey.type, &Controller->Right);
        } break;
        default:
        {
            printf("Unhandled Keycode: %u\n", XKey.keycode);
        } break;
    }
}

internal void
LinuxProcessEvents(linux_state *LinuxState)
{
    XWindowAttributes WindowAttributes;
    XGetWindowAttributes(LinuxState->Display, LinuxState->Window, &WindowAttributes);

    XEvent Event;
    while (XCheckWindowEvent(LinuxState->Display, LinuxState->Window, LinuxState->EventMask, &Event))
    {
        switch (Event.type)
        {
            case MapNotify:
            {
            } break;
            case ConfigureNotify:
            {
            /*    XResizeWindow(LinuxState->Display, LinuxState->Window, Event.xconfigure.width, Event.xconfigure.height); */
            } break;
            case DestroyNotify:
            {
                GlobalRunning = false;
            } break;
            case KeyPress:
            case KeyRelease:
            {
                LinuxProcessKey(Event.xkey, &LinuxState->Controller);
            } break;
            default:
            {
            } break;
        }
    }
}

inline void
LinuxWaitForWindowToMap(linux_state *LinuxState)
{
    XEvent Event;
    do XNextEvent(LinuxState->Display, &Event);
    while (Event.type != MapNotify);
}

int
main(int argc, char** argv)
{
    linux_state LinuxState = {};

    u32 BufferWidth = 800;
    u32 BufferHeight = 600;

    char *DefaultDisplay = getenv("DISPLAY");
    if (DefaultDisplay)
    {
        LinuxState.Display = XOpenDisplay(DefaultDisplay);
    }
    else
    {
        LinuxState.Display = XOpenDisplay(NULL);
    }
    if (LinuxState.Display)
    {
        s32 Black = BlackPixel(LinuxState.Display, DefaultScreen(LinuxState.Display));
        s32 White = WhitePixel(LinuxState.Display, DefaultScreen(LinuxState.Display));
        LinuxState.Window = XCreateSimpleWindow(LinuxState.Display, DefaultRootWindow(LinuxState.Display),
                                                0, 0, BufferWidth, BufferHeight, 0, White, Black);

        LinuxState.EventMask = StructureNotifyMask | KeyPressMask;
        XSelectInput(LinuxState.Display, LinuxState.Window, LinuxState.EventMask);

        XMapWindow(LinuxState.Display, LinuxState.Window);
        LinuxWaitForWindowToMap(&LinuxState);

        LinuxState.GraphicsContext = XCreateGC(LinuxState.Display, LinuxState.Window, 0, NULL);

        u8 TimesThrough = 0;
        GlobalRunning = true;
        while(GlobalRunning)
        {
            LinuxProcessEvents(&LinuxState);

            XSetForeground(LinuxState.Display, LinuxState.GraphicsContext, White);
            XDrawLine(LinuxState.Display, LinuxState.Window, LinuxState.GraphicsContext, 0, 0, BufferWidth, BufferHeight);
            XFlush(LinuxState.Display);
        }
    }
}
