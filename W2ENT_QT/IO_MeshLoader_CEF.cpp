#include "IO_MeshLoader_CEF.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IWriteFile.h"
#include "Settings.h"

#include <iostream>

#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"

IO_MeshLoader_CEF::IO_MeshLoader_CEF(scene::ISceneManager* smgr, io::IFileSystem* fs)
    : SceneManager(smgr), FileSystem(fs)
{

}

bool IO_MeshLoader_CEF::isALoadableFileExtension(const io::path& filename) const
{
    return core::hasFileExtension ( filename, "cef" );
}

scene::IAnimatedMesh* IO_MeshLoader_CEF::createMesh(io::IReadFile* file)
{
    if (!file)
        return nullptr;

    _log = Log::Instance();
    _log->addLine("");
    _log->addLine(formatString("-> File : %s", file->getFileName().c_str()));
    _log->add("_________________________________________________________\n\n\n");
    _log->addLineAndFlush("Start loading");


    AnimatedMesh = SceneManager->createSkinnedMesh();

    if (load(file))
    {
        SceneManager->getMeshManipulator()->recalculateNormals(AnimatedMesh);
        AnimatedMesh->finalize();
    }
    else
    {
        AnimatedMesh->drop();
        AnimatedMesh = nullptr;
    }
    _log->addLineAndFlush("Loading finished");


    return AnimatedMesh;
}

VertexComponent readVertexComponent(io::IReadFile* file)
{
    VertexComponent vComponent = VERTEX_ERROR;
    core::stringc component = readStringFixedSize(file, 16);
    if (component == "POSITION")
        vComponent = VERTEX_POSITION;
    else if (component == "BLENDWEIGHT")
        vComponent = VERTEX_BLENDWEIGHT;
    else if (component == "BLENDINDICES")
        vComponent = VERTEX_BLENDINDICE;
    else if (component == "NORMAL")
        vComponent = VERTEX_NORMAL;
    else if (component == "TANGENT")
        vComponent = VERTEX_TANGENT;
    else if (component == "TEXCOORD")
        vComponent = VERTEX_TEXCOORD;
    else
    {
        file->seek(-16, true);
        return VERTEX_ERROR;
    }

    file->seek(28, true);
    return vComponent;
}

u32 getComponentSize(VertexComponent component)
{
    switch (component) {
    case VERTEX_ERROR:
        return 0;
    case VERTEX_POSITION:
        return 16;
    case VERTEX_BLENDINDICE:
        return 6;
    case VERTEX_BLENDWEIGHT:
        return 6;
    case VERTEX_TEXCOORD:
        return 8;
    case VERTEX_NORMAL:
        return 8;
    case VERTEX_TANGENT:
        return 8;

    }
}

// a starting point : http://forum.xentax.com/viewtopic.php?f=16&t=13367
bool IO_MeshLoader_CEF::load(io::IReadFile* file)
{
    // 7 bytes always the same + timestanp of the file (4 bytes)
    file->seek(11);
    u32 nbBuffer = readU32(file);

    for (u32 i = 0; i < nbBuffer; ++i)
    {
        std::cout << "ADRESS = " << file->getPos() << std::endl;
        file->seek(8, true);
        f32 offsetX = readF32(file);
        f32 offsetY = readF32(file);
        f32 offsetZ = readF32(file);
        core::vector3df bufferOffset(offsetX, offsetY, offsetZ);

        file->seek(32, true);

        core::array<f32> boundingBox = readDataArray<f32>(file, 6);

        file->seek(4, true);
        core::stringc modelName = readStringUntilNull(file);
        bufferNames.push_back(modelName);
        std::cout << "modelName=" << modelName.c_str() << std::endl;
        file->seek(120, true);

        u32 vertexSize = 0;
        core::array<VertexComponent> components;
        VertexComponent component = VERTEX_POSITION;
        while (component != VERTEX_ERROR)
        {
            component = readVertexComponent(file);
            if (component == VERTEX_ERROR)
                break;

            vertexSize += getComponentSize(component);
            components.push_back(component);
        }

        u32 chunkSize = readU32(file);
        file->seek(10, true);
        u32 nbSet = readU32(file);

        u32 nbVertices = chunkSize / vertexSize;

        scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
        buffer->Vertices_Standard.set_used(nbVertices);
        for (u32 j = 0; j < nbVertices; ++j)
        {
            buffer->Vertices_Standard[j].Color = video::SColor(255.f, 255.f, 255.f, 255.f);            
            for (u32 h = 0; h < components.size(); ++h)
            {
                const VertexComponent c = components[h];
                if (c == VERTEX_POSITION)
                {
                    f32 x = readF32(file);
                    f32 y = readF32(file);
                    f32 z = readF32(file);
                    buffer->Vertices_Standard[j].Pos = core::vector3df(x, y, z) + bufferOffset;
                    file->seek(4, true);
                }
                else
                {
                    file->seek(getComponentSize(c), true);
                }
            }
        }
        std::cout << "ADRESS = " << file->getPos() << std::endl;
        std::cout << "NB VERTEX = " << nbVertices << std::endl;
        buffer->recalculateBoundingBox();

        file->seek(24, true);

        components.clear();
        component = VERTEX_POSITION;
        while (component != VERTEX_ERROR)
        {
            component = readVertexComponent(file);
            if (component == VERTEX_ERROR)
                break;

            components.push_back(component);
        }


        chunkSize = readU32(file);
        file->seek(10, true);
        nbSet = readU32(file);

        for (u32 j = 0; j < nbVertices; ++j)
        {
            for (u32 h = 0; h < components.size(); ++h)
            {
                const VertexComponent c = components[h];
                if (c == VERTEX_TEXCOORD)
                {
                    f32 u = readF32(file);
                    f32 v = readF32(file);
                    buffer->Vertices_Standard[j].TCoords = core::vector2df(u, v);
                }
                else
                {
                    file->seek(getComponentSize(c), true);
                }
            }
        }

        // TODO !
        std::cout << "nbSet = " << nbSet << std::endl;
        std::cout << "chunkSize = " << chunkSize << std::endl;
        //file->seek(chunkSize, true);
        file->seek(24, true);

        file->seek(44, true);
        chunkSize = readU32(file);
        file->seek(10, true);
        nbSet = readU32(file);

        std::cout << "ADRESS = " << file->getPos() << std::endl;
        u32 nbTriangles = chunkSize / 6;
        buffer->Indices.set_used(chunkSize / 2);
        for (u32 j = 0; j < nbTriangles; ++j)
        {
            buffer->Indices[j * 3 + 0] = readU16(file);
            buffer->Indices[j * 3 + 1] = readU16(file);
            buffer->Indices[j * 3 + 2] = readU16(file);

        }


        std::cout << "ADRESS = " << file->getPos() << std::endl;

        file->seek(37, true);
        u32 nbBones = readU32(file);
        u32 nbUnknown = readU32(file);
        file->seek(4, true);

        if (nbBones > 0)
        {
            // bones
            for (u32 j = 0; j < nbBones; ++j)
            {
                core::stringc name = readStringUntilNull(file);
                scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint();
                joint->Name = name;
            }

            // bones transform ?
            for (u32 j = 0; j < nbBones; ++j)
            {
                file->seek(32, true);
            }
            file->seek(36, true);
        }

        u32 unknown2 = readU32(file);
        core::stringc effectName =readStringFixedSize(file, 256);
    }

    return true;
}
