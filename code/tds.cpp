#include "tds.h"
#include "tds_gl.h"
#include <stdio.h>
#include <cstring>

void
LoadOBJ(const char *Filename, debug_loaded_obj *OBJOut,
        debug_platform_read_entire_file *ReadEntireFile, debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory)
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
    DEBUGPlatformFreeFileMemory(ReadResult.Contents);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)Memory->Permanent.Base;
    gl_api *glAPI = &Memory->glAPI;
    if (!Memory->IsInitialized)
    {
        glAPI->Enable(GL_DEPTH_TEST);

        memset(&GameState->PlayerOBJ, 0, sizeof(game_state));
        LoadOBJ("./assets/triangle.obj", &GameState->PlayerOBJ, Memory->DEBUGPlatformReadEntireFile, Memory->DEBUGPlatformFreeFileMemory);

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

        GameState->CameraP = Vec3(0, 0, -3);
        Memory->IsInitialized = true;
    }

    controller Controller = Input->Controller;
    if (!Controller.Right.IsDown && Controller.Right.WasDown)
    {
        GameState->CameraP.X -= 1.0f;
    }
    if (!Controller.Left.IsDown && Controller.Left.WasDown)
    {
        GameState->CameraP.X += 1.0f;
    }
    if (!Controller.Up.IsDown && Controller.Up.WasDown)
    {
        GameState->CameraP.Z += 1.0f;
    }
    if (!Controller.Down.IsDown && Controller.Down.WasDown)
    {
        GameState->CameraP.Z -= 1.0f;
    }

    r32 UpdateFreq = 1.0f;
    static u32 CounterCounter = 1;
    static r32 UpdateCounter = 0.0f;
    static v3 PlayerPos = {0.0f, -1.0f, 0.0f};

    glAPI->ClearColor(0, 0, 0, 1);
    glAPI->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glAPI->UseProgram(GameState->ShaderProgram);

    v3 CameraTarget = Vec3(0.0f, 0.0f, 0.0f);
    v3 Up = Vec3(0.0f, 1.0f, 0.0f);

    m3 RotationMat3 = Rotate(10, Vec3(0.0f, 1.0f, 0.0f));

	m4 Model = Mat4(1.0f);
    Model = Translate(Model, PlayerPos);
	for (u8 row=0; row < 3; ++row)
	{
        for (u8 col=0; col < 3; ++col)
        {
            Model.Elements[col][row] *= RotationMat3.Elements[col][row];
        }
	}
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

    glAPI->Flush();

    return;
}
