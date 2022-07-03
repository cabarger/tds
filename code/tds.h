#ifndef TDS_H
#include <stdint.h>
#include <stdlib.h> // size_t
#include "tds_math.h"
#include "tds_types.h"

#define internal static
#define global_variable static
#define local_persist static

#define Assert(Expression) if (!(Expression)) { puts("Assertion Fail"); exit(EXIT_FAILURE); }
#define InvalidCodePath Assert(0)

#define ArrayCount(Array) (sizeof(Array) / sizeof(*Array))


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

struct memory_arena
{
    umm Base;
    memory_index Used;
};

struct game_memory
{
    memory_arena Permanent;
    memory_arena Transient;
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

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *GameMemory, controller *Controller)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(const char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_LOAD_OBJ(name) void name(const char *Filename, debug_loaded_obj *OBJOut, debug_platform_read_entire_file *ReadEntireFile)
typedef DEBUG_PLATFORM_LOAD_OBJ(debug_platform_load_obj);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define TDS_H
#endif
