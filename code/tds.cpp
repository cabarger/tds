#include "tds.h"
#include "tds_gl.h"
#include <stdio.h>
#include <cstring>

// Png spec
// http://www.libpng.org/pub/png/spec/1.2/

struct png_chunk
{
	u32 Length;
    u32 Type;
    u8 *Data;
    u32 CRC;
};

struct png_data_ihdr
{
    u32 Width;
    u32 Height;
    u8 BitDepth;
    u8 ColorType;
    u8 CompressionMethod;
    u8 FilterMethod;
    u8 InterlaceMethod;
}; //__attribute__((packed));

struct debug_loaded_png
{
    int Delme;
};

inline bool
RunningLittleEnidean()
{
    bool Result = false;

    u8 TestBytes[4] = {};
    *TestBytes = 1;
    if (*(u32 *)TestBytes == 1)
    {
        Result = true;
    }

    return (Result);
}

void
NetOrderU32(u32 *U32Out)
{
    if (RunningLittleEnidean())
    {
        u8 *Result = (u8 *)U32Out;

        u8 Tmp = Result[0];
        Result[0] = Result[3];
        Result[3] = Tmp;

        Tmp = Result[1];
        Result[1] = Result[2];
        Result[2] = Tmp;
    }
}

void
DEBUGLoadPng(const char *Filename, debug_loaded_png *PngOut,
             debug_platform_read_entire_file *ReadEntireFile, debug_platform_free_file_memory *FreeFileMemory)
{
    debug_read_file_result ReadResult = ReadEntireFile(Filename);
/***
* ----------------------------------
* | PNG file signature ( 8 bytes ) |
* ----------------------------------
* 1 byte - Has the high bit set to detect transmission systems that do not support 8-bit data and to reduce the chance that a text file is mistakenly interpreted as a PNG, or vice versa.
* 3 bytes - In ASCII, the letters PNG, allowing a person to identify the format easily if it is viewed in a text editor.
* 2 bytes - A DOS-style line ending (CRLF) to detect DOS-Unix line ending conversion of the data.
* 1 byte -  Stops display of the file under DOS when the command type has been used—the end-of-file character.
* 1 byte - A Unix-style line ending (LF) to detect Unix-DOS line ending conversion.
*/
    // NOTE(kroney): Png is stored in network byte order ( big enidean ).
    bool IsLittleEnidean = RunningLittleEnidean();

    png_chunk Chunk;
    u8 ChunkDataBuffer[256];
    Chunk.Data = ChunkDataBuffer;

    u32 FileOffset = 0;
    memcpy(&Chunk.Length, ReadResult.Contents + FileOffset, sizeof(u32));
    FileOffset += sizeof(u64);

    memcpy(&Chunk.Type, ReadResult.Contents + FileOffset, sizeof(u32));
    FileOffset += sizeof(u32);

    NetOrderU32(&Chunk.Length);
    memcpy(Chunk.Data, ReadResult.Contents + FileOffset  + sizeof(u32), Chunk.Length);
    FileOffset += sizeof(u32);

    memcpy(&Chunk.CRC, ReadResult.Contents + FileOffset + Chunk.Length, sizeof(u32));
    FileOffset += Chunk.Length;

    NetOrderU32(&Chunk.Type);
    if (Chunk.Type == 0x49484452) // first chunk is allways ihdr
    {
        png_data_ihdr *IHDRChunk = (png_data_ihdr *)Chunk.Data;

        NetOrderU32(&IHDRChunk->Width);
        u32 ImageWidth = IHDRChunk->Width;

        NetOrderU32(&IHDRChunk->Height);
        u32 ImageHeight = IHDRChunk->Height;

        u8 BitDepth = IHDRChunk->BitDepth;
        u8 ColorType = IHDRChunk->ColorType;
/*
   Color    Allowed    Interpretation
   Type    Bit Depths

   0       1,2,4,8,16  Each pixel is a grayscale sample.

   2       8,16        Each pixel is an R,G,B triple.

   3       1,2,4,8     Each pixel is a palette index;
                       a PLTE chunk must appear.

   4       8,16        Each pixel is a grayscale sample,
                       followed by an alpha sample.

   6       8,16        Each pixel is an R,G,B triple,
                       followed by an alpha sample.
*/
        if ((BitDepth == 8) &&
            (ColorType == 6))
        {
            // Read idat *hopefully*
            u8 BytesPerPixel = 4;
            u8 ImageDataSize = BytesPerPixel*ImageWidth*ImageHeight;

            memcpy(&Chunk.Length, ReadResult.Contents + FileOffset + sizeof(u34), sizeof(u32));
            FileOffset += sizeof(u32);

            memcpy(&Chunk.Type, ReadResult.Contents + FileOffset + sizeof(u32), sizeof(u32));
            FileOffset += sizeof(u32);
/*
            NetOrderU32(&Chunk.Length);
            memcpy(Chunk.Data

            NetOrderU32(&Chunk.Type);
*/
        }
    }

    FreeFileMemory(ReadResult.Contents);
}

void
DEBUGLoadObj(const char *Filename, debug_loaded_obj *OBJOut,
        debug_platform_read_entire_file *ReadEntireFile, debug_platform_free_file_memory *FreeFileMemory)
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
                sscanf(CurrCharPtr + 2, "%f%f", tmp_uv_coords[n_uv_coords].Elements,
                                                tmp_uv_coords[n_uv_coords].Elements + 1);
                n_uv_coords++;
            }
            else if (*CurrCharPtr == 'v' && *(CurrCharPtr + 1) == 'n')
            {
                // TODO(cjb): vector normals
            }
            else if (*CurrCharPtr == 'v')
            {
                sscanf(CurrCharPtr + 1, "%f%f%f", tmp_verticies[n_verticies].Elements,
                                                  tmp_verticies[n_verticies].Elements + 1,
                                                  tmp_verticies[n_verticies].Elements + 2);
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
    FreeFileMemory(ReadResult.Contents);
}


/***
* Draw a grid on the xz plane
*/
void
DrawGrid(r32 Size, r32 Step)
{
    // disable lighting
    glDisable(GL_LIGHTING);

    glBegin(GL_LINES);

    for(r32 LineIndex = Step;
        LineIndex <= Size;
        LineIndex += Step)
    {
        glVertex3f(-Size, 0,  LineIndex);   // lines parallel to X-axis
        glVertex3f( Size, 0,  LineIndex);
        glVertex3f(-Size, 0, -LineIndex);   // lines parallel to X-axis
        glVertex3f( Size, 0, -LineIndex);

        glVertex3f( LineIndex, 0, -Size);   // lines parallel to Z-axis
        glVertex3f( LineIndex, 0,  Size);
        glVertex3f(-LineIndex, 0, -Size);   // lines parallel to Z-axis
        glVertex3f(-LineIndex, 0,  Size);
    }

    // x-axis
    glVertex3f(-Size, 0, 0);
    glVertex3f( Size, 0, 0);

    // z-axis
    glVertex3f(0, 0, -Size);
    glVertex3f(0, 0,  Size);

    glEnd();

    // enable lighting back
    glEnable(GL_LIGHTING);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)Memory->Permanent.Base;
    gl_api *glAPI = &Memory->glAPI;
    if (!Memory->IsInitialized)
    {
        glAPI->Enable(GL_DEPTH_TEST);

        memset(&GameState->PlayerOBJ, 0, sizeof(game_state));
        DEBUGLoadObj("./assets/triangle.obj", &GameState->PlayerOBJ, Memory->DEBUGPlatformReadEntireFile, Memory->DEBUGPlatformFreeFileMemory);

        debug_loaded_png LoadedPng;
        DEBUGLoadPng("./assets/pro.png", &LoadedPng, Memory->DEBUGPlatformReadEntireFile, Memory->DEBUGPlatformFreeFileMemory);

        glAPI->GenVertexArrays(1, &GameState->VAO);
        glAPI->GenBuffers(1, &GameState->VBO);

        glAPI->BindVertexArray(GameState->VAO);

        glAPI->BindBuffer(GL_ARRAY_BUFFER, GameState->VBO);
        glAPI->BufferData(GL_ARRAY_BUFFER, sizeof(v3)*GameState->PlayerOBJ.VertexCount, GameState->PlayerOBJ.Verticies, GL_STATIC_DRAW);

        glAPI->VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(v3), (void *)0);
        glAPI->EnableVertexAttribArray(0);

        debug_read_file_result VShaderFileResult = Memory->DEBUGPlatformReadEntireFile("./code/tds_shader.vs");
		u32 VertexShader = glAPI->CreateShader(GL_VERTEX_SHADER);
		glAPI->ShaderSource(VertexShader, 1, (const GLchar **)&VShaderFileResult.Contents, (GLint *)&VShaderFileResult.ContentsSize);
		glAPI->CompileShader(VertexShader);
        Memory->DEBUGPlatformFreeFileMemory(VShaderFileResult.Contents);

        s32 ShaderOK;
        glAPI->GetShaderiv(VertexShader, GL_COMPILE_STATUS, &ShaderOK);
        if (!ShaderOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glAPI->GetShaderInfoLog(VertexShader, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }

        debug_read_file_result FShaderFileResult = Memory->DEBUGPlatformReadEntireFile("./code/tds_shader.fs");
        u32 FragmentShader = glAPI->CreateShader(GL_FRAGMENT_SHADER);
        glAPI->ShaderSource(FragmentShader, 1, (const GLchar **)&FShaderFileResult.Contents, (GLint *)&FShaderFileResult.ContentsSize);
        glAPI->CompileShader(FragmentShader);
        Memory->DEBUGPlatformFreeFileMemory(FShaderFileResult.Contents);

        glAPI->GetShaderiv(FragmentShader, GL_COMPILE_STATUS, &ShaderOK);
        if (!ShaderOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glAPI->GetShaderInfoLog(FragmentShader, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }

        GameState->ShaderProgram = glAPI->CreateProgram();
        glAPI->AttachShader(GameState->ShaderProgram, VertexShader);
        glAPI->AttachShader(GameState->ShaderProgram, FragmentShader);
        glAPI->LinkProgram(GameState->ShaderProgram);

		s32 ProgramOK;
        glAPI->GetProgramiv(GameState->ShaderProgram, GL_LINK_STATUS, &ProgramOK);
        if (!ProgramOK)
        {
            char InfoLog[512];
            u32 ResultLength;
            glAPI->GetProgramInfoLog(GameState->ShaderProgram, 512, (GLsizei *)&ResultLength, InfoLog);
            for(u32 CharacterIndex=0;
                CharacterIndex < ResultLength;
                ++CharacterIndex)
            {
                putc(InfoLog[CharacterIndex], stdout);
            }
            InvalidCodePath;
        }
        glAPI->DeleteShader(VertexShader);
        glAPI->DeleteShader(FragmentShader);

        GameState->CameraP = Vec3(0.0f, 5.0f, 1.0f);
        Memory->IsInitialized = true;
    }

    static v3 PlayerP = {0.0f, 0.0f, 0.0f};

    controller Controller = Input->Controller;
    if (!Controller.Right.IsDown && Controller.Right.WasDown)
    {
       // GameState->CameraP.X += 1.0f;
        PlayerP.X += 1.0f;
    }
    if (!Controller.Left.IsDown && Controller.Left.WasDown)
    {
     //   GameState->CameraP.X -= 1.0f;
       PlayerP.X -= 1.0f;
    }
    if (!Controller.Up.IsDown && Controller.Up.WasDown)
    {
      //  GameState->CameraP.Z -= 1.0f;
        PlayerP.Z -= 1.0f;
    }
    if (!Controller.Down.IsDown && Controller.Down.WasDown)
    {
       // GameState->CameraP.Z += 1.0f;
        PlayerP.Z += 1.0f;
    }


    r32 UpdateFreq = 1.0f;
    static u32 CounterCounter = 1;
    static r32 UpdateCounter = 0.0f;

    glAPI->ClearColor(0, 0, 0, 1);
    glAPI->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glAPI->UseProgram(GameState->ShaderProgram);

    v3 CameraTarget = Vec3(0.0f, 0.0f, 0.0f);
//    v3 CameraTarget = PlayerP;
    v3 Up = Vec3(0.0f, 1.0f, 0.0f);

	m4 Model = Mat4(1.0f);
    Model = Translate(Model, PlayerP);

    GameState->CameraP.Y = PlayerP.Y + 7.f;
    GameState->CameraP.Z = PlayerP.Z + 1.0f;
    GameState->CameraP.X = PlayerP.X;

    //printf("%f, %f, %f\n", PlayerP.X, PlayerP.Y, PlayerP.Z);
    //printf("%f, %f, %f\n", GameState->CameraP.X, GameState->CameraP.Y, GameState->CameraP.Z);

    m4 View = LookAt(GameState->CameraP, CameraTarget, Up);
    m4 Projection = Perspective(ToRadians(90.0f), (r32)640.0f / (r32)480.0f, 0.1f, 100.0f);

    s32 ModelLoc = glAPI->GetUniformLocation(GameState->ShaderProgram, "model");
    glAPI->UniformMatrix4fv(ModelLoc, 1, GL_FALSE, &Model.Elements[0][0]);

    s32 ViewLoc = glAPI->GetUniformLocation(GameState->ShaderProgram, "view");
    glAPI->UniformMatrix4fv(ViewLoc, 1, GL_FALSE, &View.Elements[0][0]);

    s32 ProjectionLoc = glAPI->GetUniformLocation(GameState->ShaderProgram, "projection");
    glAPI->UniformMatrix4fv(ProjectionLoc, 1, GL_FALSE, &Projection.Elements[0][0]);

    glAPI->BindVertexArray(GameState->VAO);
    glAPI->DrawArrays(GL_TRIANGLES, 0, GameState->PlayerOBJ.VertexCount);

    glAPI->UniformMatrix4fv(ModelLoc, 1, GL_FALSE, &Model.Elements[0][0]);
    DrawGrid(16, 2);

    glAPI->Flush();

    return;
}
