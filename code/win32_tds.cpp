#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <stdio.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t bool32;

typedef float real32;
typedef double real64;

#define internal static
#define local_persist static
#define global_variable static

#if TDS_SLOW
#define assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define assert(Expression)
#endif

#define InvalidCodePath assert((!"InvalidCodePath"))

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#include <windows.h>
#include <winuser.h>
#include <gl/gl.h>

typedef struct thread_context
{
    int Placeholder;
} thread_context;

struct win32_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct win32_window_dimension
{
    int width;
    int height;
};

struct win32_state
{
    uint64 total_size;
    void* game_memory_block;
};

typedef struct game_memory
{
    bool32 is_initialized;

    uint64  permanent_storage_size;
    void* permanent_storage;

    uint64 transient_storage_size;
    void* transient_storage;
} game_memory;

typedef struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
} game_button_state;

typedef struct game_controller_input
{
    bool32 is_connected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union
    {
        game_button_state buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Back;
            game_button_state Start;

            // NOTE(casey): All buttons must be added above this line

            game_button_state Terminator;
        };
    };
} game_controller_input;

typedef struct game_input
{
    game_button_state MouseButtons[5];
    int32 mousex, mousey, mousez;

    real32 dtForFrame;

    game_controller_input controllers[5];
} game_input;

#pragma pack(push, 1)
struct bmp_header
{
    // header
    uint16 signiture;
    uint32 file_size;
    uint32 unused_;
    uint32 image_data_offset;

    // infoheader
    uint32 info_header_size;
    uint32 pixel_width;
    uint32 pixel_height;
    uint16 n_planes;
    uint16 bits_per_pixel;

    uint32 compressionype;
    uint32 compression_size;

    // 4 bytes horz pixels per meter
    // 4 bytes vert pixels per meter
    // 4 bytes n colors actually used
    // 4 bytes important colors
    uint32 pad_;

    uint32 red_mask;
    uint32 green_mask;
    uint32 blue_mask;
};
#pragma pack(pop)

/***
* IMPORTANT(casey):
* --------------------
*  * These are NOT for doing anything in the shipping game - they are
*  * blocking and the write doesn't protect against lost data!
*/

typedef struct debug_read_file_result
{
    uint32 ContentsSize;
    void *Contents;
} debug_read_file_result;

union v2
{
    struct
    {
        real32 x, y;
    };
    real32 e[2];
};

union v3
{
    struct
    {
        real32 x, y, z;
    };
    real32 e[3];
};


#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, const char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = (uint64)FileSize.QuadPart;//SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(casey): File read successfully
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    // TODO(casey): Logging
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                // TODO(casey): Logging
            }
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}

/***
* GL stuff
*/
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_GEN_BUFFERS(name) void name(GLsizei n, GLuint *buffers)
typedef GL_GEN_BUFFERS(gl_gen_buffers);

#define GL_BIND_BUFFER(name) void name(GLenum target, GLuint buffer)
typedef GL_BIND_BUFFER(gl_bind_buffer);
GL_BIND_BUFFER(glBindBufferStub)
{
    return;
}
global_variable gl_bind_buffer *glBindBuffer_ = glBindBufferStub;
#define glBindBuffer glBindBuffer_

#define GL_GEN_VERTEX_ARRAYS(name) void name(GLsizei n, GLuint *arrays)
typedef GL_GEN_VERTEX_ARRAYS(gl_gen_vertex_arrays);

#define GL_BIND_VERTEX_ARRAY(name) void name(GLuint array)
typedef GL_BIND_VERTEX_ARRAY(gl_bind_vertex_array);
GL_BIND_VERTEX_ARRAY(glBindVertexArrayStub)
{
    return ;
}
global_variable gl_bind_vertex_array *glBindVertexArray_ = glBindVertexArrayStub;
#define glBindVertexArray glBindVertexArray_

#define GL_BUFFER_DATA(name) void name(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
typedef GL_BUFFER_DATA(gl_buffer_data);

#define GL_ENABLE_VERTEX_ATTRIB_ARRAY(name) void name(GLuint index)
typedef GL_ENABLE_VERTEX_ATTRIB_ARRAY(gl_enable_vertex_attrib_array);
GL_ENABLE_VERTEX_ATTRIB_ARRAY(glEnableVertexAttribArrayStub)
{
    return;
}
global_variable gl_enable_vertex_attrib_array *glEnableVertexAttribArray_ = glEnableVertexAttribArrayStub;
#define glEnableVertexAttribArray glEnableVertexAttribArray_

#define GL_VERTEX_ATTRIB_POINTER(name) void name(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
typedef GL_VERTEX_ATTRIB_POINTER(gl_vertex_attrib_pointer);
GL_VERTEX_ATTRIB_POINTER(glVertexAttribPointerStub)
{
    return;
}
global_variable gl_vertex_attrib_pointer *glVertexAttribPointer_ = glVertexAttribPointerStub;
#define glVertexAttribPointer glVertexAttribPointer_

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
////

struct  loaded_bmp
{
    uint32 pixel_width;
    uint32 pixel_height;
    uint32 bytes_per_pixel;

    uint8* image_data;
};

internal void
load_bmp(const char* path, loaded_bmp* bmp, thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile)
{
    FILE* bmp_hand = fopen(path, "r");
    bmp_header header = {0};
    fread(&header, sizeof(bmp_header), 1, bmp_hand);

    /* debug_read_file_result ReadResult = ReadEntireFile(Thread, path); */

    bmp->pixel_width = header.pixel_width;
    bmp->pixel_height = header.pixel_height;

    assert(header.bits_per_pixel > 7);
    bmp->bytes_per_pixel = header.bits_per_pixel / 8;

    fseek(bmp_hand, header.image_data_offset, SEEK_SET);
    bmp->image_data = (uint8 *) calloc(bmp->pixel_width*bmp->pixel_height, bmp->bytes_per_pixel);
    fread(bmp->image_data, bmp->bytes_per_pixel, bmp->pixel_width*bmp->pixel_height, bmp_hand);

    fclose(bmp_hand);

#if 0
    char buf[30] = {0};
    sprintf(buf, "FILE SIZE: %i\n", header.file_size);
    OutputDebugString(buf);

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "PIXEL WIDTH: %i\n", header.pixel_width);
    OutputDebugString(buf);

#endif
}

global_variable bool32 g_running;
global_variable win32_offscreen_buffer g_backbuffer;
global_variable int64 g_perfcount_freq;

inline real32
win32_get_seconds_elapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                     (real32)g_perfcount_freq);
    return(Result);
}

inline LARGE_INTEGER
win32_get_wall_clock(void)
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return(result);
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
win32_process_pending_messages(win32_state *state, game_controller_input *keyboard_controller)
{
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                g_running = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)message.wParam;

                // NOTE(casey): Since we are comparing WasDown to IsDown,
                // we MUST use == and != to convert these bit tests to actual
                // 0 or 1 values.
                bool32 WasDown = ((message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->Back, IsDown);
                        g_running = false;
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&keyboard_controller->Start, IsDown);
                    }
                }
            }
            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        }
    }
}

struct loaded_obj
{
    v3 Verticies[256];
    uint16 nVerticies;

    v3 UVCoords[256];
    uint16 nUVCoords;
    /*
        v3 Normals_[256];
        uint16 nNormals;
    */
};

global_variable loaded_obj GlobalCubeOBJ = {};
global_variable GLuint GlobalVAO = 0;
global_variable GLuint GlobalVBO = 0;
global_variable bool32 GlobalLoadedGLFuncs = false;

internal void
win32_resize_dib_section(win32_offscreen_buffer* buffer, int width, int height)
{
    if (buffer->memory)
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    buffer->width = width;
    buffer->height = height;

    int bytes_per_pixel = 4;
    buffer->bytes_per_pixel = bytes_per_pixel;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    int bitmap_memory_size = (buffer->width*buffer->height)*bytes_per_pixel;
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width*bytes_per_pixel;
}

internal win32_window_dimension
win32_get_window_dimension(HWND window)
{
    win32_window_dimension result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return(result);
}

internal void
win32_display_buffer_in_window(win32_offscreen_buffer *Buffer,
                               HDC DeviceContext, int WindowWidth, int WindowHeight)
{
#if 1
    if((WindowWidth >= Buffer->width*2) &&
       (WindowHeight >= Buffer->height*2))
    {
        StretchDIBits(DeviceContext,
                      0, 0, 5*Buffer->width, 5*Buffer->height,
                      0, 0, Buffer->width, Buffer->height,
                      Buffer->memory,
                      &Buffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int OffsetX = 10;
        int OffsetY = 10;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, WHITENESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->height, WindowWidth, WindowHeight, WHITENESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, WHITENESS);
        PatBlt(DeviceContext, OffsetX + Buffer->width, 0, WindowWidth, WindowHeight, WHITENESS);

        // NOTE(casey): For prototyping purposes, we're going to always blit
        // 1-to-1 pixels to make sure we don't introduce artifacts with
        // stretching while we are learning to code the renderer!
        StretchDIBits(DeviceContext,
                      OffsetX, OffsetY, Buffer->width, Buffer->height,
                      0, 0, Buffer->width, Buffer->height,
                      Buffer->memory,
                      &Buffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
#else
    if (!GlobalLoadedGLFuncs)
        return;

	glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glTranslatef(1.5f, 0.0f, -7.0f);

    glBegin(GL_QUADS);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, GlobalVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);
    glDrawArrays(GL_TRIANGLES, 0, GlobalCubeOBJ.nVerticies);

    SwapBuffers(DeviceContext);
#endif
}

internal LRESULT CALLBACK
win32_main_window_callback(HWND Window,
                           UINT Message,
                           WPARAM WParam,
                           LPARAM LParam)
{
    LRESULT result = 0;
    switch(Message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            // TODO(casey): Handle this with a message to the user?
            g_running = false;
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(Window, &paint);
            win32_window_dimension dimension = win32_get_window_dimension(Window);
            win32_display_buffer_in_window(&g_backbuffer, device_context,
                                           dimension.width, dimension.height);
            EndPaint(Window, &paint);
        } break;
        default:
        {
            result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    return result;
}

inline game_controller_input *get_controller(game_input *input, int unsigned controller_index)
{
    assert(controller_index < ArrayCount(input->controllers));

    game_controller_input *result = &input->controllers[controller_index];
    return(result);
}

inline int32
round_real32_to_int32(real32 r32)
{
    if (r32 == 0.0f)
        return 0;
    int32 result = (int32)roundf(r32);
    return result;
}

inline uint32
roundReal32ToUint32(real32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    return(Result);
}

internal void
draw_bmp(win32_offscreen_buffer* buffer, v2 top_left, loaded_bmp* bmp)
{
    int32 minx = round_real32_to_int32(top_left.x);
    int32 miny = round_real32_to_int32(top_left.y);
    int32 maxx = (int32)bmp->pixel_width;
    int32 maxy = (int32)bmp->pixel_height;

    if (minx < 0)
        minx = 0;
    if (miny < 0)
        miny = 0;
    if (maxx > buffer->width)
        maxx = buffer->width;
    if (maxy > buffer->height)
        maxy = buffer->height;

    uint8 *Row = ((uint8 *)buffer->memory + miny*buffer->pitch + minx*buffer->bytes_per_pixel);
    for(int y = miny;
        y < maxy;
        ++y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int x = minx;
            x < maxx;
            ++x)
        {
            uint8* bmp_pixel = (uint8 *)bmp->image_data + (maxy - y) * bmp->pixel_width*bmp->bytes_per_pixel
                + x*bmp->bytes_per_pixel;

            // BGR => RGB
            uint32 Color = ((*(bmp_pixel + 2) << 16) | // R
                            (*(bmp_pixel + 1)) << 8  | // G
                            (*bmp_pixel) << 0);        // B
            *Pixel++ = Color;
        }

        Row += buffer->pitch;
    }
}



internal uint8
DEBUGLoadOBJ(const char* path, loaded_obj* OBJOut, thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile)
{
    debug_read_file_result ReadResult = ReadEntireFile(Thread, path);

    //NOTE(caleb): ignoring normals for now
    uint8 reading_nothing = 0;
    uint8 reading_v =        1;
    uint8 reading_ut =       2;
    uint8 reading_vn =       4;
    uint8 reading_f =        8;
    uint8 reading_comment = 16;

    // tmp_verticies
    v3 tmp_verticies[256] = {0};
    uint16 n_verticies = 0;

    // tmp_uv coords
    v3 tmp_uv_coords[256] = {0};
    uint16 n_uv_coords = 0;

    const real32 SillyReal32Init = -95.771542;
    real32 tmp_a = SillyReal32Init;
    real32 tmp_b = SillyReal32Init;
    real32 tmp_c = SillyReal32Init;

    const uint16 IndexInit = ArrayCount(OBJOut->Verticies) + 1;
    uint16 TmpVertexIndexPlusOne = IndexInit;
    uint16 TmpUVCoordIndexPlusOne = IndexInit;

    uint8 reading_wat = reading_nothing;
    for (char* cptr=(char*)ReadResult.Contents; *cptr; cptr++)
    {
        if (*cptr == '\n')
        {
            if (reading_wat == reading_v)
            {
                if ((tmp_a != SillyReal32Init) &&
                    (tmp_b != SillyReal32Init) &&
                    (tmp_c != SillyReal32Init))
                {
                    tmp_verticies[n_verticies].x = tmp_a;
                    tmp_verticies[n_verticies].y = tmp_b;
                    tmp_verticies[n_verticies].z = tmp_c;
                    n_verticies++;

                    tmp_a = SillyReal32Init;
                    tmp_b = SillyReal32Init;
                    tmp_c = SillyReal32Init;
                }
            }
            else if (reading_wat == reading_ut)
            {
                if ((tmp_a != SillyReal32Init) && (tmp_b != SillyReal32Init))
                {
                    tmp_uv_coords[n_uv_coords].x = tmp_a;
                    tmp_uv_coords[n_uv_coords].y = tmp_b;
                    n_uv_coords++;

                    tmp_a = SillyReal32Init;
                    tmp_b = SillyReal32Init;
                }
            }
            else if (reading_wat == reading_f)
            {
                if ((TmpVertexIndexPlusOne != IndexInit) && (TmpUVCoordIndexPlusOne != IndexInit))
                {
                    OBJOut->Verticies[OBJOut->nVerticies++] = tmp_verticies[TmpVertexIndexPlusOne - 1];
                    OBJOut->UVCoords[OBJOut->nUVCoords++] = tmp_uv_coords[TmpUVCoordIndexPlusOne - 1];

                    TmpVertexIndexPlusOne = IndexInit;
                    TmpUVCoordIndexPlusOne = IndexInit;
                }
            }
            reading_wat = reading_nothing;
        }

        if (reading_wat == reading_v)
        {
            if (*cptr != ' ')
            {
                if (tmp_a == SillyReal32Init)
                    sscanf(cptr, "%f", &tmp_a);
                else if (tmp_b == SillyReal32Init)
                    sscanf(cptr, "%f", &tmp_b);
                else if (tmp_c == SillyReal32Init)
                    sscanf(cptr, "%f", &tmp_c);
                else
                    InvalidCodePath;

                if (*cptr == '-')
                    cptr += 8;
                else
                    cptr += 7;
            }
        }
        else if (reading_wat == reading_ut)
        {
            if (*cptr != ' ')
            {
                if (tmp_a == SillyReal32Init)
                    sscanf(cptr, "%f", &tmp_a);
                else if (tmp_b == SillyReal32Init)
                    sscanf(cptr, "%f", &tmp_b);
                else
                    InvalidCodePath;
                cptr += 7;
            }

        }
        else if (reading_wat == reading_vn)
        {
            // TODO(caleb): handle me
        }
        else if (reading_wat == reading_f)
        {
            if (*cptr != ' ')
            {
                if (TmpVertexIndexPlusOne == IndexInit)
                {
                    sscanf(cptr, "%hu", &TmpVertexIndexPlusOne);
                    cptr++;
                }
                else if (TmpUVCoordIndexPlusOne == IndexInit)
                {
                    sscanf(cptr, "%hu", &TmpUVCoordIndexPlusOne);
                    cptr += 2; // adv past VN index
                }
            }
        }

        if (reading_wat == reading_nothing)
        {
            if (*cptr == 'v' && *(cptr + 1) == ' ') // reading v
            {
                reading_wat = reading_v;
                cptr++;
            }
            else if (*cptr == 'v' && *(cptr + 1) == 't') // reading texture coord
            {
                reading_wat = reading_ut;
                cptr += 2;
            }
            else if (*cptr == 'f' && *(cptr + 1) == ' ')
                reading_wat = reading_f;
            else if (*cptr == '#')
                reading_wat = reading_comment;
        }
    }

    return 0;
}

internal void
draw_line(win32_offscreen_buffer* buffer, v2 start, v2 end, real32 r, real32 g, real32 b)
{
    int32 startx = round_real32_to_int32(start.x);
    int32 starty = round_real32_to_int32(start.y);
    int32 endx = round_real32_to_int32(end.x);
    int32 endy = round_real32_to_int32(end.y);

    if (startx >= buffer->width)
        startx = buffer->width;
    if (endx >= buffer->width)
        endx = buffer->width;

    if (startx < 0)
        startx = 0;
    if (endx < 0)
        endx = 0;

    if (starty >= buffer->height)
        starty = buffer->height;
    if (endy >= buffer->height)
        endy = buffer->height;

    if (starty < 0)
        starty = 0;
    if (endy < 0)
        endy = 0;

    int32 length = round_real32_to_int32(sqrt(((endx - startx)*(endx - startx)) +
                                              ((endy - starty)*(endy - starty))));
    real32 stepx = (endx - start.x) / length;
    real32 stepy = (endy - start.y) / length;
    real32 currx = start.x;
    real32 curry = start.y;

    uint32 color = ((roundReal32ToUint32(r * 255.0f) << 16) |
                    (roundReal32ToUint32(g * 255.0f) << 8)  |
                    (roundReal32ToUint32(b * 255.0f) << 0));
    for (int i=0; i < length; ++i)
    {
        uint8* pixel_start = ((uint8 *)buffer->memory +
                              round_real32_to_int32(currx)*buffer->bytes_per_pixel +
                              round_real32_to_int32(curry)*buffer->pitch);
        *(uint32*)pixel_start = color;
        currx += stepx;
        curry += stepy;
    }
}

internal void
draw_rectangle(win32_offscreen_buffer* buffer, v2 vec_min, v2 vec_max, real32 r, real32 g, real32 b)
{
    int32 Minx = round_real32_to_int32(vec_min.x);
    int32 Miny = round_real32_to_int32(vec_min.y);
    int32 Maxx = round_real32_to_int32(vec_max.x);
    int32 Maxy = round_real32_to_int32(vec_max.y);

    if (Minx < 0)
        Minx = 0;
    if (Miny < 0)
        Miny = 0;
    if (Maxx > buffer->width)
        Maxx = buffer->width;
    if (Maxy > buffer->height)
        Maxy = buffer->height;

    uint32 Color = ((roundReal32ToUint32(r * 255.0f) << 16) |
                    (roundReal32ToUint32(g * 255.0f) << 8)  |
                    (roundReal32ToUint32(b * 255.0f) << 0));

    uint8 *Row = ((uint8 *)buffer->memory +
                  Minx*buffer->bytes_per_pixel +
                  Miny*buffer->pitch);
    for(int y = Miny;
        y < Maxy;
        ++y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int x = Minx;
            x < Maxx;
            ++x)
        {
            *Pixel++ = Color;
        }

        Row += buffer->pitch;
    }
}

internal v2
rotate(v2 p, v2 origin, real32 rad_angle)
{
    v2 out = {};

    real32 cos = cosf(rad_angle);
    real32 sin = sinf(rad_angle);
    real32 temp;

    temp = ((p.x - origin.x)*cos - (p.y - origin.y)*sin) + origin.x;
    out.y = ((p.x - origin.x)*sin + (p.y - origin.y)*cos) + origin.y;
    out.x = temp;

    return out;
}

internal real32
rad(real32 in)
{
    real32 out;

    out = in * (3.14 / 180);
    return out;
}

internal real32
deg(real32 in)
{
    real32 out;
    out = in * (180 / 3.14);

    return out;
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    win32_state win32_state = {};

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    g_perfcount_freq = PerfCountFrequencyResult.QuadPart;

    // NOTE(casey): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular.
    UINT DesiredSchedulerMS = 1;
    bool32 sleep_is_granular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    win32_resize_dib_section(&g_backbuffer, 640, 480);

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = Instance;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
    //  window_class.hIcon;
    window_class.lpszClassName = "TDSWindowClass";

    if (RegisterClassA(&window_class))
    {
        HWND window = CreateWindowExA(
                                      0,
                                      window_class.lpszClassName,
                                      "Top Down Shooter",
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      640,
                                      480,
                                      0,
                                      0,
                                      Instance,
                                      0);
        if (window)
        {
            PIXELFORMATDESCRIPTOR requested_pfd =
            {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
                PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
                32,                   // Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24,                   // Number of bits for the depthbuffer
                8,                    // Number of bits for the stencilbuffer
                0,                    // Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0
            };

            HDC device_context = GetDC(window);

            int pixel_format_idx = ChoosePixelFormat(device_context, &requested_pfd);

            PIXELFORMATDESCRIPTOR suggested_pfd;
            DescribePixelFormat(device_context, pixel_format_idx, sizeof(suggested_pfd), &suggested_pfd);
            SetPixelFormat(device_context, pixel_format_idx, &suggested_pfd);

            HGLRC glr_context = wglCreateContext(device_context);
            wglMakeCurrent(device_context, glr_context);

            ReleaseDC(window, device_context);

            thread_context FakeThread;

            DEBUGLoadOBJ("assets/cube.obj", &GlobalCubeOBJ, &FakeThread, DEBUGPlatformReadEntireFile);

            // bind ogl functions
            gl_gen_buffers* glGenBuffers = (gl_gen_buffers *)wglGetProcAddress("glGenBuffers");
            glBindBuffer = (gl_bind_buffer *)wglGetProcAddress("glBindBuffer");

            glEnableVertexAttribArray = (gl_enable_vertex_attrib_array *)wglGetProcAddress("glEnableVertexAttribArray");
            glVertexAttribPointer = (gl_vertex_attrib_pointer  *)wglGetProcAddress("glVertexAttribPointer");

            gl_buffer_data *glBufferData = (gl_buffer_data *)wglGetProcAddress("glBufferData");
            gl_gen_vertex_arrays *glGenVertexArrays = (gl_gen_vertex_arrays *)wglGetProcAddress("glGenVertexArrays");
            gl_bind_vertex_array *glBindVertexArray = (gl_bind_vertex_array *)wglGetProcAddress("glBindVertexArray");

            GlobalLoadedGLFuncs = true;
// DEBUG code opengl cube prep
            glGenVertexArrays(1, &GlobalVAO);
            glBindVertexArray(GlobalVAO);

            glGenBuffers(1, &GlobalVBO);
            glBindBuffer(GL_ARRAY_BUFFER, GlobalVBO);

            glBufferData(GL_ARRAY_BUFFER, sizeof(v3)*GlobalCubeOBJ.nVerticies, GlobalCubeOBJ.Verticies, GL_STATIC_DRAW);
            glBindVertexArray(0);
////
            int MonitorRefreshHz = 60;
            real32 GameUpdateHz = (MonitorRefreshHz / 1.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

            g_running = true;
            LPVOID base_address = 0;

            game_memory game_memory = {};
            game_memory.permanent_storage_size = megabytes(64);
            game_memory.transient_storage_size = gigabytes(1);

            win32_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            win32_state.game_memory_block = VirtualAlloc(base_address, (int32)win32_state.total_size,
                                                         MEM_RESERVE|MEM_COMMIT,
                                                         PAGE_READWRITE);
            game_memory.permanent_storage = win32_state.game_memory_block;
            game_memory.transient_storage = ((uint8*)win32_state.game_memory_block + game_memory.permanent_storage_size);

            if (game_memory.permanent_storage && game_memory.transient_storage)
            {
                game_input input[2] = {};
                game_input* new_input = &input[0];
                game_input* old_input = &input[1];

                LARGE_INTEGER last_counter = win32_get_wall_clock();
                LARGE_INTEGER flip_wall_clock = win32_get_wall_clock();

                uint64 LastCycleCount = __rdtsc();

                real32 player_move_speed = 200.0f;
                int32 player_width = 25;
                int32 player_height = 14;
                int32 player_startx = g_backbuffer.width / 2;
                int32 player_starty = g_backbuffer.height / 2;

                v2 player_left = {(real32)(player_startx-player_width/2),
                    (real32)(player_height + (player_starty + player_height / 2))};

                v2 player_right = {(real32)(player_startx + player_width/2),
                    (real32)(player_height + (player_starty + player_height / 2))};

                v2 playerop = {(real32)(player_left.x + ((player_right.x - player_left.x) / 2)),
                    (real32)(player_starty - player_height / 2)};
#if 0

                loaded_bmp TestBMP = {};
                load_bmp("assets/floor_tile.bmp", &TestBMP);
#endif

                while(g_running)
                {
                    new_input->dtForFrame = TargetSecondsPerFrame;

                    game_controller_input* old_keyboard_controller = get_controller(old_input,  0);
                    game_controller_input* new_keyboard_controller = get_controller(new_input,  0);

                    *new_keyboard_controller = {};
                    new_keyboard_controller->is_connected = true;
                    for(int button_index = 0;
                        button_index < ArrayCount(new_keyboard_controller->buttons);
                        ++button_index)
                    {
                        new_keyboard_controller->buttons[button_index].EndedDown =
                            old_keyboard_controller->buttons[button_index].EndedDown;
                    }
                    win32_process_pending_messages(&win32_state, new_keyboard_controller);

                    tagPOINT mousep = {};
                    GetCursorPos(&mousep);
                    ScreenToClient(window, &mousep);
                    new_input->mousex = mousep.x;
                    new_input->mousey = mousep.y;
                    new_input->mousez = 0;

/////////////////////// Game update and render

                    //TODO(caleb): collision

                    // Clear screen
                    v2 zero_vec = {0.0f, 0.0f};
                    v2 wh_vec = {(real32)g_backbuffer.width, (real32)g_backbuffer.height};
                    draw_rectangle(&g_backbuffer, zero_vec, wh_vec, 0.0f, 0.0f, 0.0f);

                    //draw_bmp(&g_backbuffer, {0.0f, 10.0f}, &TestBMP);

                    // Translate player verticies
                    if (new_keyboard_controller->MoveRight.EndedDown)
                    {
                        playerop.x += player_move_speed*new_input->dtForFrame;
                        player_right.x += player_move_speed*new_input->dtForFrame;
                        player_left.x += player_move_speed*new_input->dtForFrame;
                    }
                    if (new_keyboard_controller->MoveLeft.EndedDown)
                    {
                        playerop.x -= player_move_speed*new_input->dtForFrame;
                        player_right.x -= player_move_speed*new_input->dtForFrame;
                        player_left.x -= player_move_speed*new_input->dtForFrame;
                    }
                    if (new_keyboard_controller->MoveUp.EndedDown)
                    {
                        playerop.y -= player_move_speed*new_input->dtForFrame;
                        player_right.y -= player_move_speed*new_input->dtForFrame;
                        player_left.y -= player_move_speed*new_input->dtForFrame;
                    }
                    if (new_keyboard_controller->MoveDown.EndedDown)
                    {
                        playerop.y += player_move_speed*new_input->dtForFrame;
                        player_right.y += player_move_speed*new_input->dtForFrame;
                        player_left.y += player_move_speed*new_input->dtForFrame;
                    }

                    // Rotate player to face mosue
                    v2 mouse_pos = {(real32)new_input->mousex, (real32)new_input->mousey};
                    v2 player_mid = {playerop.x, playerop.y + player_height};

                    v2 veca = {-(player_mid.x - playerop.x), -(player_mid.y - playerop.y)};
                    real32 veca_len = sqrtf(veca.x * veca.x + veca.y * veca.y);

                    v2 vecb = {mouse_pos.x - player_mid.x, mouse_pos.y - player_mid.y};
                    real32 vecb_len = sqrtf(vecb.x * vecb.x + vecb.y * vecb.y);

                    real32 angle_ab = acosf((veca.x*vecb.x + veca.y*vecb.y) / (veca_len*vecb_len));

                    if (mouse_pos.x < player_mid.x)
                        angle_ab *= -1.0f;

                    // Translate player points
                    v2 player_left_p = rotate(player_left, player_mid, angle_ab);
                    v2 player_right_p = rotate(player_right, player_mid, angle_ab);
                    v2 playerop_p = rotate(playerop, player_mid, angle_ab);

                    // Draw Player
                    draw_line(&g_backbuffer, player_left_p, playerop_p, 1.0f, 1.0f, 1.0f);
                    draw_line(&g_backbuffer, player_right_p, playerop_p, 1.0f, 1.0f, 1.0f);
                    draw_line(&g_backbuffer, player_left_p, player_right_p, 1.0f, 1.0f, 1.0f);

                    // Player mid mouse
                    if (new_keyboard_controller->Start.EndedDown)
                        draw_line(&g_backbuffer, player_mid, mouse_pos, 1.0f, 1.0f, 0.0f);
                    else
                        draw_line(&g_backbuffer, player_mid, mouse_pos, 1.0f, 0.0f, 0.0f);

/////////////////////// Game update and render

                    LARGE_INTEGER WorkCounter = win32_get_wall_clock();
                    real32 WorkSecondsElapsed = win32_get_seconds_elapsed(last_counter, WorkCounter);

                    real32 seconds_elapsed_for_frame = WorkSecondsElapsed;
                    if(seconds_elapsed_for_frame < TargetSecondsPerFrame)
                    {
                        if(sleep_is_granular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                               seconds_elapsed_for_frame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }

                        real32 Testseconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter,
                                                                                         win32_get_wall_clock());
                        if(Testseconds_elapsed_for_frame < TargetSecondsPerFrame)
                        {
                            // TODO(casey): LOG MISSED SLEEP HERE
                        }

                        while(seconds_elapsed_for_frame < TargetSecondsPerFrame)
                        {
                            seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter,
                                                                                  win32_get_wall_clock());
                        }
                    }
                    LARGE_INTEGER end_counter = win32_get_wall_clock();
                    real32 MSPerFrame = 1000.0f*win32_get_seconds_elapsed(last_counter, end_counter);
                    last_counter = end_counter;

                    flip_wall_clock = win32_get_wall_clock();

                    win32_window_dimension dimension = win32_get_window_dimension(window);
                    device_context = GetDC(window);
                    win32_display_buffer_in_window(&g_backbuffer, device_context,
                                                   dimension.width, dimension.height);
                    ReleaseDC(window, device_context);

                    game_input *temp = new_input;
                    new_input = old_input;
                        old_input = temp;
                }
            }
        }
    }
    return 0;
}
