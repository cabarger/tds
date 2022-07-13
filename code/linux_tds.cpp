#include "tds.h"
#include "linux_tds.h"
#include "tds_math.h"
#include "tds_types.h"
#include "tds_gl.h"

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h> // sleep
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // getenv
#include <sys/stat.h>
#include <limits.h>
#include <time.h>

#include <GL/gl.h>
#include <GL/glx.h>

global_variable b32 GlobalRunning;

inline void *
LinuxLoadLibrary(const char *LibPath)
{
    void *LibHandle;
    LibHandle = dlopen(LibPath, RTLD_LAZY);
    if (!LibHandle)
    {
        fprintf(stderr, "%s\n", dlerror());
        InvalidCodePath;
    }
    dlerror();
    return LibHandle;
}

inline void
LinuxUnloadLibrary(void *LibraryHandle)
{
    if (LibraryHandle)
    {
        puts("closed lib");
        dlclose(LibraryHandle);
    }
    LibraryHandle = NULL;
}

inline void *
LinuxLoadFunction(void *LibraryHandle, const char *Sym)
{
    void *Result;
    Result = dlsym(LibraryHandle, Sym);
    return Result;
}

inline ino_t
LinuxFileID(const char *FilePath)
{
    ino_t Result;

    struct stat StatBuf;
    stat(FilePath, &StatBuf);
    Result = StatBuf.st_ino;

    return Result;
}

internal void
LinuxLoadGameCode(linux_game_code *GameCode, ino_t NewFileID)
{
    LinuxUnloadLibrary(GameCode->LibraryHandle);
    GameCode->LibraryHandle = LinuxLoadLibrary("./libtds.so");
    GameCode->UpdateAndRender = (game_update_and_render *) LinuxLoadFunction(GameCode->LibraryHandle, "GameUpdateAndRender");
    GameCode->FileID = NewFileID;
}

internal void
LinuxProcessButtonPress(XKeyEvent XKey, button_state *Button)
{
    if (XKey.type == KeyPress)
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
        case KEYCODE_ESCAPE:
        {
            GlobalRunning = false;
        } break;
        case KEYCODE_D:
        {
            LinuxProcessButtonPress(XKey, &Controller->Right);
        } break;
        case KEYCODE_A:
        {
            LinuxProcessButtonPress(XKey, &Controller->Left);
        } break;
        case KEYCODE_W:
        {
            LinuxProcessButtonPress(XKey, &Controller->Up);
        } break;
        case KEYCODE_S:
        {
            LinuxProcessButtonPress(XKey, &Controller->Down);
        } break;
        default:
        {
#if 0
            printf("Unhandled Keycode: %u\n", XKey.keycode);
#endif
        } break;
    }
}

internal void
LinuxProcessEvents(linux_state *LinuxState, game_input *Input)
{
    XWindowAttributes WindowAttributes;
    XGetWindowAttributes(LinuxState->Display, LinuxState->Window, &WindowAttributes);

    XEvent Event;
    while (XCheckWindowEvent(LinuxState->Display, LinuxState->Window, LinuxState->EventMask, &Event))
    {
        switch (Event.type)
        {
            case ConfigureNotify:
            {
                glViewport(0, 0, (GLfloat)Event.xconfigure.width, (GLfloat)Event.xconfigure.height);
            } break;
            case DestroyNotify:
            {
                GlobalRunning = false;
            } break;
            case KeyRelease:
            case KeyPress:
            {
                LinuxProcessKey(Event.xkey, &Input->Controller);
            } break;
            case MotionNotify:
            {
                Input->MouseX =  Event.xmotion.x;
                Input->MouseY = Event.xmotion.y;
            }
            default:
            {
            } break;
        }
    }
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    free(Memory);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};

    u32 fd = open(Filename, O_RDONLY, (mode_t)0600);
    struct stat Statbuf;
    fstat(fd, &Statbuf);
    u32 FileSize = (u32)Statbuf.st_size;

    Result.Contents = (char *)malloc(FileSize);
    u32 BytesRead = read(fd, Result.Contents, FileSize);

    close(fd);
    if (BytesRead != FileSize)
    {
        free(Result.Contents);
        Result.Contents = 0;
    }
    Result.ContentsSize = BytesRead;
    return(Result);
}

void
LinuxLoadGLAPI(gl_api *glAPI)
{
    glAPI->GenBuffers = (gl_gen_buffers *)glXGetProcAddress((GLubyte *)"glGenBuffers");
    glAPI->BindBuffer = (gl_bind_buffer *)glXGetProcAddress((GLubyte *)"glBindBuffer");
    glAPI->BufferData = (gl_buffer_data *)glXGetProcAddress((GLubyte *)"glBufferData");
    glAPI->CreateShader = (gl_create_shader *)glXGetProcAddress((GLubyte *)"glCreateShader");
    glAPI->ShaderSource = (gl_shader_source *)glXGetProcAddress((GLubyte *)"glShaderSource");
    glAPI->CompileShader = (gl_compile_shader *)glXGetProcAddress((GLubyte *)"glCompileShader");
    glAPI->GetShaderiv = (gl_get_shader_iv *)glXGetProcAddress((GLubyte *)"glGetShaderiv");
    glAPI->GetShaderInfoLog = (gl_get_shader_info_log *)glXGetProcAddress((GLubyte *)"glGetShaderInfoLog");
    glAPI->CreateProgram = (gl_create_program *)glXGetProcAddress((GLubyte *)"glCreateProgram");
    glAPI->AttachShader = (gl_attach_shader *)glXGetProcAddress((GLubyte *)"glAttachShader");
    glAPI->LinkProgram = (gl_link_program *)glXGetProcAddress((GLubyte *)"glLinkProgram");
    glAPI->GetProgramiv = (gl_get_program_iv *)glXGetProcAddress((GLubyte *)"glGetProgramiv");
    glAPI->GetProgramInfoLog = (gl_get_program_info_log *)glXGetProcAddress((GLubyte *)"glGetProgramInfoLog");
    glAPI->UseProgram = (gl_use_program *)glXGetProcAddress((GLubyte *)"glUseProgram");
    glAPI->DeleteShader = (gl_delete_shader *)glXGetProcAddress((GLubyte *)"glDeleteShader");
    glAPI->VertexAttribPointer = (gl_vertex_attrib_pointer *)glXGetProcAddress((GLubyte *)"glVertexAttribPointer");
    glAPI->EnableVertexAttribArray = (gl_enable_vertex_attrib_array *)glXGetProcAddress((GLubyte *)"glEnableVertexAttribArray");
    glAPI->GenVertexArrays = (gl_gen_vertex_arrays *)glXGetProcAddress((GLubyte *)"glGenVertexArrays");
    glAPI->BindVertexArray = (gl_bind_vertex_array *)glXGetProcAddress((GLubyte *)"glBindVertexArray");
    glAPI->GetUniformLocation = (gl_get_uniform_location *)glXGetProcAddress((GLubyte *)"glGetUniformLocation");
    glAPI->UniformMatrix4fv = (gl_uniform_matrix_4fv *)glXGetProcAddress((GLubyte *)"glUniformMatrix4fv");
    glAPI->Enable = (gl_enable *)glXGetProcAddress((GLubyte *)"glEnable");
    glAPI->ClearColor = (gl_clear_color *)glXGetProcAddress((GLubyte *)"glClearColor");
    glAPI->Clear = (gl_clear *)glXGetProcAddress((GLubyte *)"glClear");
    glAPI->DrawArrays = (gl_draw_arrays *)glXGetProcAddress((GLubyte *)"glDrawArrays");
    glAPI->Flush = (gl_flush *)glXGetProcAddress((GLubyte *)"glFlush");
//    glAPI->PolygonMode = (gl_polygon_mode *)glXGetProcAddress((GLubyte *)"glPolygonMode");
}

inline r32
LinuxGetTimeMS(void)
{
    r32 Result;
    Result = ((r32)clock() / CLOCKS_PER_SEC)*1000;
    return(Result);
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

    u32 BufferWidth = 640;
    u32 BufferHeight = 480;

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
        int AttributeList[] = { GLX_RGBA, None };
        XVisualInfo *VisualInfo = glXChooseVisual(LinuxState.Display, DefaultScreen(LinuxState.Display), AttributeList);
        LinuxState.GC = glXCreateContext(LinuxState.Display, VisualInfo, 0, GL_TRUE);
        if (LinuxState.GC == 0)
        {
            fprintf(stderr, "Failed to create glX context\n");
            InvalidCodePath;
        }

        LinuxState.EventMask = StructureNotifyMask|KeyPressMask|KeyReleaseMask|PointerMotionMask;

        XSetWindowAttributes SWA;
        SWA.colormap = XCreateColormap(LinuxState.Display, RootWindow(LinuxState.Display, VisualInfo->screen), VisualInfo->visual, AllocNone);
        SWA.border_pixel = 0;
        SWA.event_mask = LinuxState.EventMask;

        LinuxState.Window = XCreateWindow(LinuxState.Display, RootWindow(LinuxState.Display, VisualInfo->screen),
                                          0, 0, BufferWidth, BufferHeight, 0, VisualInfo->depth, InputOutput,
                                          VisualInfo->visual, CWBorderPixel|CWColormap|CWEventMask, &SWA);

        XMapWindow(LinuxState.Display, LinuxState.Window);
        LinuxWaitForWindowToMap(&LinuxState);
        glXMakeCurrent(LinuxState.Display, LinuxState.Window, LinuxState.GC);


        game_memory GameMemory = {};
        void *PermanentBase = calloc(1, MB(5));
		InitializeArena(&GameMemory.Permanent, MB(5), PermanentBase);
        PushStruct(&GameMemory.Permanent, game_state);
        GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
        GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
        LinuxLoadGLAPI(&GameMemory.glAPI);

        game_input Input = {0};
        controller OldController = {0};

        r32 LastTimeMS = LinuxGetTimeMS();

        GlobalRunning = true;
        while(GlobalRunning)
        {
            r32 TimeNowMS = LinuxGetTimeMS();
            r32 TimeSinceLastFrameMS = TimeNowMS - LastTimeMS;
            LastTimeMS = TimeNowMS;

            ino_t NewFileID = LinuxFileID("libtds.so");
            if (LinuxState.GameCode.FileID != NewFileID)
            {
                LinuxLoadGameCode(&LinuxState.GameCode, NewFileID);
            }

            LinuxProcessEvents(&LinuxState, &Input);
            for (u8 ButtonIndex = 0;
                 ButtonIndex < ArrayCount(Input.Controller.Buttons);
                 ++ButtonIndex)
            {
                if (OldController.Buttons[ButtonIndex].IsDown)
                {
                    Input.Controller.Buttons[ButtonIndex].WasDown = true;
                }
                else
                {
                    Input.Controller.Buttons[ButtonIndex].WasDown = false;
                }
            }
            Input.dTimeMS = TimeSinceLastFrameMS;
            if (LinuxState.GameCode.UpdateAndRender)
            {
                LinuxState.GameCode.UpdateAndRender(&GameMemory, &Input);
            }

            OldController = Input.Controller;
        }
    }
}
