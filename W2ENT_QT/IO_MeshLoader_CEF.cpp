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
    else if (component == "INDICE")
        vComponent = VERTEX_INDICE;
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
    case VERTEX_INDICE:
        return 2*3;
    }
}

// a starting point : http://forum.xentax.com/viewtopic.php?f=16&t=13367
bool IO_MeshLoader_CEF::load(io::IReadFile* file)
{
    scene::ISkinnedMesh::SJoint* rootJoint = AnimatedMesh->addJoint();

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
        //file->seek(120, true);
        file->seek(4, true);
        core::array<f32> unkFloats = readDataArray<f32>(file, 2);
        std::cout << "unkown float 1 = " << unkFloats[0] << std::endl;
        std::cout << "unkown float 2 = " << unkFloats[1] << std::endl; // 1.0 ?

        file->seek(28, true);
        u32 nbVertices = readU32(file);
        file->seek(4, true);
        u32 nbTriangles = readU32(file);
        file->seek(44, true);

        file->seek(12, true); // uint32 + uint32 + 00 00 CD AB
        u32 nbVertexComponents = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        core::array<VertexComponent> components;
        for (u32 j = 0; j < nbVertexComponents; ++j)
        {
            VertexComponent component = readVertexComponent(file);
            components.push_back(component);
        }

        u32 chunkSize = readU32(file);
        file->seek(10, true);
        u32 nbSet = readU32(file);

        scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
        buffer->Vertices_Standard.set_used(nbVertices);
        buffer->Indices.set_used(nbTriangles * 3);
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

        file->seek(12, true); // uint32 + uint32 + 00 00 CD AB
        nbVertexComponents = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        components.clear();
        for (u32 j = 0; j < nbVertexComponents; ++j)
        {
            VertexComponent component = readVertexComponent(file);
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
        file->seek(12, true); // uint32 + uint32 + 00 00 CD AB
        nbVertexComponents = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        components.clear();
        for (u32 j = 0; j < nbVertexComponents; ++j)
        {
            VertexComponent component = readVertexComponent(file);
            components.push_back(component);
        }

        chunkSize = readU32(file);
        file->seek(10, true);
        nbSet = readU32(file);

        std::cout << "ADRESS = " << file->getPos() << std::endl;


        for (u32 j = 0; j < nbTriangles; ++j)
        {
            for (u32 h = 0; h < components.size(); ++h)
            {
                const VertexComponent c = components[h];
                if (c == VERTEX_INDICE)
                {
                    buffer->Indices[j * 3 + 0] = readU16(file);
                    buffer->Indices[j * 3 + 1] = readU16(file);
                    buffer->Indices[j * 3 + 2] = readU16(file);
                }
            }
        }

        std::cout << "ADRESS = " << file->getPos() << std::endl;

        file->seek(37, true);
        u32 nbBones = readU32(file);
        u32 nbUnknown = readU32(file);
        file->seek(4, true);

        if (nbBones > 0)
        {
            core::array<scene::ISkinnedMesh::SJoint*> joints;

            // bones
            for (u32 j = 0; j < nbBones; ++j)
            {
                core::stringc name = readStringUntilNull(file);
                scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint(rootJoint);
                joint->Name = name;
                joints.push_back(joint);
            }

            // bones transform
            for (u32 j = 0; j < nbBones; ++j)
            {
                scene::ISkinnedMesh::SJoint* joint = joints[j];

                // quaternion + translaction + scale ?
                core::array<f32> unkFloats = readDataArray<f32>(file, 8);
                //for  (u32 h = 0; h < 8; ++h)
                //    std::cout << h << " : " << unkFloats[h] << std::endl;

                // rotation
                core::quaternion q(unkFloats[0], unkFloats[1], unkFloats[2], unkFloats[3]);
                core::vector3df rotation;
                q.toEuler(rotation);

                // position
                core::vector3df position(unkFloats[4], unkFloats[5], unkFloats[6]);

                // scale
                core::vector3df scale(unkFloats[7], unkFloats[7], unkFloats[7]);



                core::matrix4 positionMatrix;
                positionMatrix.setTranslation(-position);
                core::matrix4 scaleMatrix;
                scaleMatrix.setScale(scale);
                core::matrix4 rotationMatrix;
                rotationMatrix.setRotationRadians(rotation);
                core::matrix4 invRotationMatrix;
                rotationMatrix.getInverse(invRotationMatrix);

                joint->GlobalMatrix = scaleMatrix * invRotationMatrix * positionMatrix;
                joint->LocalMatrix = joint->GlobalMatrix;
            }
            file->seek(36, true); // 00000000000000...
        }

        u32 unknown2 = readU32(file);
        core::stringc effectName =readStringFixedSize(file, 256);
        std::cout << "Effect = " << effectName.c_str() << std::endl;
    }

    return true;
}
