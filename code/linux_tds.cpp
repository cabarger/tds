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

#include <GL/gl.h>
#include <GL/glx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
LinuxLoadGameCode(linux_game_code *GameCode)
{
    ino_t FileID = LinuxFileID("libtds.so");
    if (GameCode->FileID != FileID)
    {
        printf("Loaded ID: %u, New ID: %u\n", GameCode->FileID, FileID);
        LinuxUnloadLibrary(GameCode->LibraryHandle);
        GameCode->LibraryHandle = LinuxLoadLibrary("./libtds.so");
        GameCode->UpdateAndRender = (game_update_and_render *) LinuxLoadFunction(GameCode->LibraryHandle, "GameUpdateAndRender");
        GameCode->FileID = FileID;
    }
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
            printf("Unhandled Keycode: %u\n", XKey.keycode);
        } break;
    }
}

internal void
LinuxProcessEvents(linux_state *LinuxState, controller *Controller)
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
                glViewport(0, 0, (GLfloat)Event.xconfigure.width, (GLfloat)Event.xconfigure.height);
            } break;
            case DestroyNotify:
            {
                GlobalRunning = false;
            } break;
            case KeyRelease:
            case KeyPress:
            {
                LinuxProcessKey(Event.xkey, Controller);
            } break;
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

DEBUG_PLATFORM_LOAD_OBJ(DEBUGPlatformLoadOBJ)
{
    debug_read_file_result ReadResult = ReadEntireFile(Filename);

    // NOTE(cb): ignoring normals for now
    b32 ReadingComment = false;

    // tmp_verticies
    v3 tmp_verticies[256] = {0};
    u16 n_verticies = 0;

    // tmp_uv coords
    v2 tmp_uv_coords[256] = {0};
    u16 n_uv_coords = 0;

    for (u32 CharacterIndex = 0;
         CharacterIndex < ReadResult.ContentsSize;
         ++CharacterIndex)
    {
        char *CurrCharPtr = ReadResult.Contents + CharacterIndex;
        if (!ReadingComment)
        {
            if (*CurrCharPtr == '#' || *CurrCharPtr == 'o' || *CurrCharPtr == 's')
            {
                ReadingComment = true;
            }
            else if (*CurrCharPtr == 'v' && *(CurrCharPtr + 1) == 't')
            {
                sscanf(CurrCharPtr + 2, "%f%f", tmp_uv_coords[n_uv_coords].m_,
                                                tmp_uv_coords[n_uv_coords].m_ + 1);
                n_uv_coords++;
            }
            else if (*CurrCharPtr == 'v' && *(CurrCharPtr + 1) == 'n')
            {
                // TODO(cjb): vector normals
            }
            else if (*CurrCharPtr == 'v')
            {
                sscanf(CurrCharPtr + 1, "%f%f%f", tmp_verticies[n_verticies].m_,
                                                  tmp_verticies[n_verticies].m_ + 1,
                                                  tmp_verticies[n_verticies].m_ + 2);
                n_verticies++;
            }
            else if (*CurrCharPtr == 'f')
            {
                u32 VIndex[3];
                u32 UVIndex[3];
                u32 VNIndex[3]; // unused
                sscanf(CurrCharPtr + 1, "%u/%u/%u", VIndex,  UVIndex,  VNIndex);
                sscanf(CurrCharPtr + 7, "%u/%u/%u", VIndex + 1,  UVIndex + 1,  VNIndex + 1);
                sscanf(CurrCharPtr + 13, "%u/%u/%u", VIndex + 2,  UVIndex + 2,  VNIndex + 2);

                OBJOut->Verticies[OBJOut->VertexCount++] = tmp_verticies[VIndex[0] - 1];
                OBJOut->Verticies[OBJOut->VertexCount++] = tmp_verticies[VIndex[1] - 1];
                OBJOut->Verticies[OBJOut->VertexCount++] = tmp_verticies[VIndex[2] - 1];

                OBJOut->UVCoords[OBJOut->UVCoordCount++] = tmp_uv_coords[UVIndex[0] - 1];
                OBJOut->UVCoords[OBJOut->UVCoordCount++] = tmp_uv_coords[UVIndex[1] - 1];
                OBJOut->UVCoords[OBJOut->UVCoordCount++] = tmp_uv_coords[UVIndex[2] - 1];
            }
        }
        if (*CurrCharPtr == '\n')
        {
            ReadingComment = false;
        }
    }
    DEBUGPlatformFreeFileMemory(ReadResult.Contents);
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

        LinuxState.EventMask = StructureNotifyMask|KeyPressMask|KeyReleaseMask;

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

        // TODO(caleb): gl code fetching/loader
        gl_gen_buffers *glGenBuffers = (gl_gen_buffers *)glXGetProcAddress((GLubyte *)"glGenBuffers");
        gl_bind_buffer *glBindBuffer = (gl_bind_buffer *)glXGetProcAddress((GLubyte *)"glBindBuffer");
        gl_buffer_data *glBufferData = (gl_buffer_data *)glXGetProcAddress((GLubyte *)"glBufferData");
		gl_create_shader *glCreateShader = (gl_create_shader *)glXGetProcAddress((GLubyte *)"glCreateShader");
		gl_shader_source *glShaderSource = (gl_shader_source *)glXGetProcAddress((GLubyte *)"glShaderSource");
        gl_compile_shader *glCompileShader = (gl_compile_shader *)glXGetProcAddress((GLubyte *)"glCompileShader");
        gl_get_shader_iv *glGetShaderiv = (gl_get_shader_iv *)glXGetProcAddress((GLubyte *)"glGetShaderiv");
        gl_get_shader_info_log *glGetShaderInfoLog = (gl_get_shader_info_log *)glXGetProcAddress((GLubyte *)"glGetShaderInfoLog");
        gl_create_program *glCreateProgram = (gl_create_program *)glXGetProcAddress((GLubyte *)"glCreateProgram");
        gl_attach_shader *glAttachShader = (gl_attach_shader *)glXGetProcAddress((GLubyte *)"glAttachShader");
        gl_link_program *glLinkProgram = (gl_link_program *)glXGetProcAddress((GLubyte *)"glLinkProgram");
        gl_get_program_iv *glGetProgramiv = (gl_get_program_iv *)glXGetProcAddress((GLubyte *)"glGetProgramiv");
        gl_get_program_info_log *glGetProgramInfoLog = (gl_get_program_info_log *)glXGetProcAddress((GLubyte *)"glGetProgramInfoLog");
        gl_use_program *glUseProgram = (gl_use_program *)glXGetProcAddress((GLubyte *)"glUseProgram");
        gl_delete_shader *glDeleteShader = (gl_delete_shader *)glXGetProcAddress((GLubyte *)"glDeleteShader");
        gl_vertex_attrib_pointer *glVertexAttribPointer = (gl_vertex_attrib_pointer *)glXGetProcAddress((GLubyte *)"glVertexAttribPointer");
        gl_enable_vertex_attrib_array *glEnableVertexAttribArray = (gl_enable_vertex_attrib_array *)glXGetProcAddress((GLubyte *)"glEnableVertexAttribArray");
        gl_gen_vertex_arrays *glGenVertexArrays = (gl_gen_vertex_arrays *)glXGetProcAddress((GLubyte *)"glGenVertexArrays");
        gl_bind_vertex_array *glBindVertexArray = (gl_bind_vertex_array *)glXGetProcAddress((GLubyte *)"glBindVertexArray");
        gl_get_uniform_location *glGetUniformLocation = (gl_get_uniform_location *)glXGetProcAddress((GLubyte *)"glGetUniformLocation");
        gl_uniform_matrix_4fv *glUniformMatrix4fv = (gl_uniform_matrix_4fv *)glXGetProcAddress((GLubyte *)"glUniformMatrix4fv");

        glEnable(GL_DEPTH_TEST);

        debug_loaded_obj Cube = {};
        DEBUGPlatformLoadOBJ("./assets/cube.obj", &Cube, DEBUGPlatformReadEntireFile);

        u32 VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v3)*Cube.VertexCount, Cube.Verticies, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), (void *)0);
        glEnableVertexAttribArray(0);

        debug_read_file_result VShaderFileResult = DEBUGPlatformReadEntireFile("./code/tds_shader.vs");
		u32 VertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(VertexShader, 1, (const GLchar **)&VShaderFileResult.Contents, (GLint *)&VShaderFileResult.ContentsSize);
		glCompileShader(VertexShader);
        DEBUGPlatformFreeFileMemory(VShaderFileResult.Contents);

        s32 ShaderOK;
        glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &ShaderOK);
        if (!ShaderOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glGetShaderInfoLog(VertexShader, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }

        debug_read_file_result FShaderFileResult = DEBUGPlatformReadEntireFile("./code/tds_shader.fs");
        u32 FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(FragmentShader, 1, (const GLchar **)&FShaderFileResult.Contents, (GLint *)&FShaderFileResult.ContentsSize);
        glCompileShader(FragmentShader);
        DEBUGPlatformFreeFileMemory(FShaderFileResult.Contents);

        glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &ShaderOK);
        if (!ShaderOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glGetShaderInfoLog(FragmentShader, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }

        u32 ShaderProgram = glCreateProgram();
        glAttachShader(ShaderProgram, VertexShader);
        glAttachShader(ShaderProgram, FragmentShader);
        glLinkProgram(ShaderProgram);

		s32 ProgramOK;
        glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &ProgramOK);
        if (!ProgramOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glGetProgramInfoLog(ShaderProgram, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }
        glDeleteShader(VertexShader);
        glDeleteShader(FragmentShader);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        game_memory GameMemory = {};
        controller Controller[2];
        controller *PrevController = Controller;
        controller *CurrController = Controller + 1;

        v3 CameraP = v3_(0.0f, 0.0f, 0.0f);
        r32 RotateAmt = 0.0f;

        GlobalRunning = true;
        while(GlobalRunning)
        {
            LinuxProcessEvents(&LinuxState, CurrController);

            for (u8 ButtonIndex = 0;
                 ButtonIndex < ArrayCount(CurrController->Buttons);
                 ++ButtonIndex)
            {
                if (PrevController->Buttons[ButtonIndex].IsDown)
                {
                    CurrController->Buttons[ButtonIndex].WasDown = true;
                }
                else
                {
                    CurrController->Buttons[ButtonIndex].WasDown = false;
                }
            }

            LinuxLoadGameCode(&LinuxState.GameCode);
            if (LinuxState.GameCode.UpdateAndRender)
            {
                LinuxState.GameCode.UpdateAndRender(&GameMemory, CurrController);
            }

            if (!CurrController->Right.IsDown && CurrController->Right.WasDown)
            {
                CameraP.x += 1.0f;
            }
            if (!CurrController->Left.IsDown && CurrController->Left.WasDown)
            {
                CameraP.x -= 1.0f;
            }
            if (!CurrController->Up.IsDown && CurrController->Up.WasDown)
            {
                CameraP.z += 1.0f;
            }
            if (!CurrController->Down.IsDown && CurrController->Down.WasDown)
            {
                CameraP.z -= 1.0f;
            }

            RotateAmt += 0.01f;
            if (RotateAmt > 360.0f)
            {
                RotateAmt = 0.0f;
            }

            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(ShaderProgram);

            glm::mat4 Model = glm::mat4(1.0f);
            glm::mat4 View = glm::mat4(1.0f);
            glm::mat4 Projection = glm::mat4(1.0f);

//            Model = glm::rotate(Model, glm::radians(RotateAmt), glm::vec3(0.0f, 1.0f, 0.0f));
            View = glm::translate(View, glm::vec3(-CameraP.x, CameraP.y, CameraP.z));
            Projection = glm::perspective(glm::radians(100.0f), (r32)BufferWidth / (r32)BufferHeight, 0.1f, 100.0f);

            s32 ModelLoc = glGetUniformLocation(ShaderProgram, "model");
            glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, glm::value_ptr(Model));

            s32 ViewLoc = glGetUniformLocation(ShaderProgram, "view");
            glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, glm::value_ptr(View));

            s32 ProjectionLoc = glGetUniformLocation(ShaderProgram, "projection");
            glUniformMatrix4fv(ProjectionLoc, 1, GL_FALSE, glm::value_ptr(Projection));

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, Cube.VertexCount);

            glFlush();

            *PrevController = *CurrController;
        }
    }
}
