#ifndef TDS_H
#include <stdint.h>
#include <stdlib.h> // size_t
#include "tds_math.h"
#include "tds_types.h"
#include "tds_gl.h"

#define internal static
#define global_variable static
#define local_persist static

#define Assert(Expression) if (!(Expression)) { void *ptr = NULL; *(int *)ptr; }
#define InvalidCodePath Assert(0)

#define KB(Count) (Count*1024)
#define MB(Count) (KB(Count)*1024)
#define GB(Count) (MB(Count)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof(*Array))

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))

struct memory_arena
{
    umm Base;
    memory_index Size;
    memory_index Used;
    u32 TempCount;
};

void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
	Arena->Size = Size;
	Arena->Base = (umm)Base;
	Arena->Used = 0;
    Arena->TempCount = 0;
}

umm
PushSize_(memory_arena *Arena, memory_index Size)
{
	Assert((Arena->Used + Size) <= Arena->Size);
	umm Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return Result;
}


struct button_state
{
    b32 WasDown;
    b32 IsDown;
};

union controller
{
    struct
    {
        button_state Right;
        button_state Left;
        button_state Up;
        button_state Down;
    };
    button_state Buttons[4];
};

struct debug_read_file_result
{
    u32 ContentsSize;
    char *Contents;
};

struct debug_loaded_obj
{
    v3 Verticies[256];
    u16 VertexCount;

    v2 UVCoords[256];
    u16 UVCoordCount;

    /*
        v3 Normals_[256];
        uint16 nNormals;
    */
};


#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

struct game_memory
{
    b32 IsInitialized;

    memory_arena Permanent;
    memory_arena Transient;

    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    gl_api glAPI;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, controller *Controller)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

struct game_state
{
    u32 VAO, VBO;
    u32 ShaderProgram;

    v3 CameraP;

    debug_loaded_obj PlayerOBJ;
};

#define TDS_H
#endif
