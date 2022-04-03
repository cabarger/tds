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
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#define internal static
#define local_persist static
#define global_variable static

#if TDS_SLOW
// TODO(casey): Complete assertion macro - don't worry everyone!
#define assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define assert(Expression)
#endif

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define array_count(Array) (sizeof(Array) / sizeof((Array)[0]))
// TODO(casey): swap, min, max ... macros???

#include <windows.h>

struct win32_offscreen_buffer_t
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
};

struct win32_window_dimension_t
{
    int width;
    int height;
};

struct win32_state_t
{
    uint64 total_size;
    void* game_memory_block;
};

typedef struct game_memory_t
{
    bool32 is_initialized;

    uint64  permanent_storage_size;
    void* permanent_storage;

    uint64 transient_storage_size;
    void* transient_storage;
} game_memory_t;

typedef struct game_button_state_t
{
    int HalfTransitionCount;
    bool32 EndedDown;
} game_button_state_t;

typedef struct game_controller_input_t
{
    bool32 is_connected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union
    {
        game_button_state_t buttons[12];
        struct
        {
            game_button_state_t MoveUp;
            game_button_state_t MoveDown;
            game_button_state_t MoveLeft;
            game_button_state_t MoveRight;

            game_button_state_t ActionUp;
            game_button_state_t ActionDown;
            game_button_state_t ActionLeft;
            game_button_state_t ActionRight;

            game_button_state_t LeftShoulder;
            game_button_state_t RightShoulder;

            game_button_state_t Back;
            game_button_state_t Start;

            // NOTE(casey): All buttons must be added above this line

            game_button_state_t Terminator;
        };
    };
} game_controller_input_t;

typedef struct game_input_t
{
    game_button_state_t MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;

    real32 dtForFrame;

    game_controller_input_t controllers[5];
} game_input_t;


global_variable bool32 g_running;
global_variable win32_offscreen_buffer_t g_backbuffer;
global_variable int64 GlobalPerfCountFrequency;

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                     (real32)GlobalPerfCountFrequency);
    return(Result);
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

internal void
Win32ProcessKeyboardMessage(game_button_state_t *NewState, bool32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
win32_process_pending_messages(win32_state_t *state, game_controller_input_t *keyboard_controller)
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

internal void
win32_resize_dib_section(win32_offscreen_buffer_t* buffer, int width, int height)
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

internal win32_window_dimension_t
win32_get_window_dimension(HWND window)
{
    win32_window_dimension_t result;

    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;

    return(result);
}

internal void
win32_display_buffer_in_window(win32_offscreen_buffer_t *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // TODO(casey): Centering / black bars?

    if((WindowWidth >= Buffer->width*2) &&
       (WindowHeight >= Buffer->height*2))
    {
        StretchDIBits(DeviceContext,
                      0, 0, 2*Buffer->width, 2*Buffer->height,
                      0, 0, Buffer->width, Buffer->height,
                      Buffer->memory,
                      &Buffer->info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int OffsetX = 10;
        int OffsetY = 10;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->width, 0, WindowWidth, WindowHeight, BLACKNESS);

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
            win32_window_dimension_t dimension = win32_get_window_dimension(Window);
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

inline game_controller_input_t *get_controller(game_input_t *input, int unsigned controller_index)
{
    assert(controller_index < array_count(input->controllers));

    game_controller_input_t *result = &input->controllers[controller_index];
    return(result);
}

union v2_t
{
    struct
    {
        real32 x, y;
    };
    real32 e[2];
};

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)roundf(Real32);
    return(Result);
}

inline uint32
RoundReal32ToUInt32(real32 Real32)
{
    uint32 Result = (uint32)roundf(Real32);
    return(Result);
}

internal void
draw_rectangle(win32_offscreen_buffer_t* buffer, v2_t vec_min, v2_t vec_max, real32 r, real32 g, real32 b)
{
    int32 Minx = RoundReal32ToInt32(vec_min.x);
    int32 Miny = RoundReal32ToInt32(vec_min.y);
    int32 Maxx = RoundReal32ToInt32(vec_max.x);
    int32 Maxy = RoundReal32ToInt32(vec_max.y);

    if(Minx < 0)
    {
        Minx = 0;
    }

    if(Miny < 0)
    {
        Miny = 0;
    }

    if(Maxx > buffer->width)
    {
        Maxx = buffer->width;
    }

    if(Maxy > buffer->height)
    {
        Maxy = buffer->height;
    }

    uint32 Color = ((RoundReal32ToUInt32(r * 255.0f) << 16) |
                    (RoundReal32ToUInt32(g * 255.0f) << 8) |
                    (RoundReal32ToUInt32(b * 255.0f) << 0));

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

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    win32_state_t win32_state = {};

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // NOTE(casey): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular.
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    win32_resize_dib_section(&g_backbuffer, 960, 540);

    WNDCLASSA window_class = {};
    window_class.style = CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = win32_main_window_callback;
    window_class.hInstance = Instance;
    window_class.hCursor = LoadCursor(0, IDC_ARROW);
//    WindowClass.hIcon;
    window_class.lpszClassName = "TDSWindowClass";

    if (RegisterClassA(&window_class))
    {
        HWND window = CreateWindowExA(
                0,
                window_class.lpszClassName,
                "Top Down Shooter",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if (window)
        {
            int MonitorRefreshHz = 60;
            real32 GameUpdateHz = (MonitorRefreshHz / 1.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

            g_running = true;
            LPVOID base_address = 0;

            game_memory_t game_memory = {};
            game_memory.permanent_storage_size = megabytes(64);
            game_memory.transient_storage_size = gigabytes(1);

            win32_state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            win32_state.game_memory_block = VirtualAlloc(base_address, (size_t)win32_state.total_size,
                                                         MEM_RESERVE|MEM_COMMIT,
                                                         PAGE_READWRITE);
            game_memory.permanent_storage = win32_state.game_memory_block;
            game_memory.transient_storage = ((uint8*)win32_state.game_memory_block + game_memory.permanent_storage_size);

            if (game_memory.permanent_storage && game_memory.transient_storage)
            {
                game_input_t input[2] = {};
                game_input_t* new_input = &input[0];
                game_input_t* old_input = &input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                uint64 LastCycleCount = __rdtsc();

                v2_t player_pos = {(real32)g_backbuffer.width/2, (real32)g_backbuffer.height/2};
                int32 player_width = 40;
                real32 player_move_speed = 200.0f;

                while(g_running)
                {
                    new_input->dtForFrame = TargetSecondsPerFrame;

                    game_controller_input_t* old_keyboard_controller = get_controller(old_input,  0);
                    game_controller_input_t* new_keyboard_controller = get_controller(new_input,  0);

                    *new_keyboard_controller = {};
                    new_keyboard_controller->is_connected = true;
                    for(int button_index = 0;
                        button_index < array_count(new_keyboard_controller->buttons);
                        ++button_index)
                    {
                        new_keyboard_controller->buttons[button_index].EndedDown =
                            old_keyboard_controller->buttons[button_index].EndedDown;
                    }

                    win32_process_pending_messages(&win32_state, new_keyboard_controller);

                    // game update and render
                    v2_t zero_vec = {0.0f, 0.0f};
                    v2_t wh_vec = {(real32)g_backbuffer.width, (real32)g_backbuffer.height};
                    draw_rectangle(&g_backbuffer, zero_vec, wh_vec, 0.0f, 0.0f, 0.0f);

                    if (new_keyboard_controller->MoveRight.EndedDown)
                        player_pos.x+=player_move_speed*new_input->dtForFrame;;
                    if (new_keyboard_controller->MoveLeft.EndedDown)
                        player_pos.x-=player_move_speed*new_input->dtForFrame;
                    if (new_keyboard_controller->MoveUp.EndedDown)
                        player_pos.y-=player_move_speed*new_input->dtForFrame;
                    if (new_keyboard_controller->MoveDown.EndedDown)
                        player_pos.y+=player_move_speed*new_input->dtForFrame;

                    v2_t min = {player_pos.x - (player_width*0.5f), player_pos.y - (player_width*0.5f)};
                    v2_t max = {player_pos.x + (player_width*0.5f), player_pos.y + (player_width*0.5f)};
/*
                    min.x = (min.x*cosf(1.0f)) - (min.y*sinf(1.0f));
                    min.y = (min.x*sinf(1.0f)) + (min.y*cosf(1.0f));
                    max.x = (max.x*cosf(1.0f)) - (max.y*sinf(1.0f));
                    max.y = (max.x*sinf(1.0f)) + (max.y*cosf(1.0f));
*/

                    real32 gray = 0.5f;
                    draw_rectangle(&g_backbuffer, min, max, gray, gray, gray);
                    /////

                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                               SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }

                        real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                   Win32GetWallClock());
                        if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            // TODO(casey): LOG MISSED SLEEP HERE
                        }

                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                            Win32GetWallClock());
                        }
                    }
                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    real32 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter);
                    LastCounter = EndCounter;

                    FlipWallClock = Win32GetWallClock();

                    win32_window_dimension_t dimension = win32_get_window_dimension(window);
                    HDC device_context = GetDC(window);
                    win32_display_buffer_in_window(&g_backbuffer, device_context,
                                               dimension.width, dimension.height);
                    ReleaseDC(window, device_context);

                    game_input_t *temp = new_input;
                    new_input = old_input;
                    old_input = temp;
                }
            }
        }
    }
    return 0;
}

