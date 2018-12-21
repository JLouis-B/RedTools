#include "IO_MeshLoader_CEF.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IWriteFile.h"
#include "Settings.h"

#include <iostream>

#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"
#include "Utils_Halffloat.h"

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
    VertexComponent vComponent;
    vComponent._type = VERTEX_ERROR;
    core::stringc component = readStringFixedSize(file, 16);
    if (component == "POSITION")
        vComponent._type = VERTEX_POSITION;
    else if (component == "BLENDWEIGHT")
        vComponent._type = VERTEX_BLENDWEIGHT;
    else if (component == "BLENDINDICES")
        vComponent._type = VERTEX_BLENDINDICE;
    else if (component == "NORMAL")
        vComponent._type = VERTEX_NORMAL;
    else if (component == "TANGENT")
        vComponent._type = VERTEX_TANGENT;
    else if (component == "TEXCOORD")
        vComponent._type = VERTEX_TEXCOORD;
    else if (component == "COLOR")
        vComponent._type = VERTEX_COLOR;
    else if (component == "INDICE")
        vComponent._type = VERTEX_INDICE;
    else
    {
        file->seek(-16, true);
        return vComponent;
    }

    file->seek(20, true);
    u32 unk = readU32(file);
    if (vComponent._type == VERTEX_TEXCOORD)
    {
        vComponent._nbUV = unk - 114;
    }

    file->seek(4, true); // 1

    return vComponent;
}

u32 getComponentSize(VertexComponent component)
{
    switch (component._type) {
    case VERTEX_ERROR:
        return 0;
    case VERTEX_POSITION:
        return 16;
    case VERTEX_BLENDINDICE:
        return 4;
    case VERTEX_BLENDWEIGHT:
        return 8;
    case VERTEX_TEXCOORD:
        return component._nbUV * 8;
    case VERTEX_NORMAL:
        return 8;
    case VERTEX_TANGENT:
        return 8;
    case VERTEX_COLOR:
        return 4;
    case VERTEX_INDICE:
        return 6;
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
        u32 nbVertexComponentTypes = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        core::array<VertexComponent> components;
        for (u32 j = 0; j < nbVertexComponentTypes; ++j)
        {
            VertexComponent component = readVertexComponent(file);
            components.push_back(component);
        }

        u32 chunkSize = readU32(file);
        file->seek(10, true);
        u32 nbSet = readU32(file);

        std::cout << "ADRESS BUFFER = " << file->getPos() << std::endl;
        core::array<CEF_Weight> weights;
        weights.set_used(nbVertices * 4);


        scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
        buffer->Vertices_Standard.set_used(nbVertices);
        buffer->Indices.set_used(nbTriangles * 3);
        for (u32 j = 0; j < nbVertices; ++j)
        {
            buffer->Vertices_Standard[j].Color = video::SColor(255.f, 255.f, 255.f, 255.f);            
            for (u32 h = 0; h < components.size(); ++h)
            {
                const VertexComponent c = components[h];
                if (c._type == VERTEX_POSITION)
                {
                    f32 x = readF32(file);
                    f32 y = readF32(file);
                    f32 z = readF32(file);
                    buffer->Vertices_Standard[j].Pos = core::vector3df(x, y, z) + bufferOffset;
                    file->seek(4, true);
                }
                else if (c._type == VERTEX_BLENDINDICE)
                {
                    core::array<u8> blendIndices = readDataArray<u8>(file, 4);
                    for (u32 k = 0; k < 4; ++k)
                    {
                        weights[j*4 + 3-k]._boneId = blendIndices[k];
                        //std::cout << k << " : " << (int)blendIndices[k] << std::endl;
                    }
                }
                else if (c._type == VERTEX_BLENDWEIGHT)
                {
                    core::array<u16> blendWeight = readDataArray<u16>(file, 4);
                    for (u32 k = 0; k < 4; ++k)
                    {
                        weights[j*4 + k]._weight = halfToFloat(blendWeight[k]);
                        weights[j*4 + k]._vertex = j;
                        //std::cout << "BLEND WEIGHT " << k << " : " << halfToFloat(blendWeight[k]) << std::endl;
                    }
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
        nbVertexComponentTypes = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        bool hasNormals = false;
        components.clear();
        for (u32 j = 0; j < nbVertexComponentTypes; ++j)
        {
            VertexComponent component = readVertexComponent(file);
            components.push_back(component);
            if (component._type == VERTEX_NORMAL)
                hasNormals = true;
        }


        chunkSize = readU32(file);
        file->seek(10, true);
        nbSet = readU32(file);

        for (u32 j = 0; j < nbVertices; ++j)
        {
            for (u32 h = 0; h < components.size(); ++h)
            {
                const VertexComponent c = components[h];
                if (c._type == VERTEX_TEXCOORD)
                {
                    f32 u = readF32(file);
                    f32 v = readF32(file);
                    buffer->Vertices_Standard[j].TCoords = core::vector2df(u, v);

                    // Only 1 UV channel at the moment
                    file->seek((c._nbUV - 1) * 8, true);
                    /*
                    if (c._nbUV > 1)
                    {
                        f32 u2 = readF32(file);
                        f32 v2 = readF32(file);
                        std::cout << "U2=" << u2 << ", V2=" << v2 << std::endl;
                    }
                    */
                }
                else if (c._type == VERTEX_NORMAL)
                {
                    f32 nX = halfToFloat(readU16(file));
                    f32 nY = halfToFloat(readU16(file));
                    f32 nZ = halfToFloat(readU16(file));
                    f32 nW = halfToFloat(readU16(file));
                    //std::cout << "X=" << nX << ", Y=" << nY << ", Z=" << nZ << ", W=" << nW << std::endl;

                    buffer->Vertices_Standard[j].Normal = core::vector3df(nX, nY, nZ);
                }
                else if (c._type == VERTEX_TANGENT)
                {
                    f32 tX = halfToFloat(readU16(file));
                    f32 tY = halfToFloat(readU16(file));
                    f32 tZ = halfToFloat(readU16(file));
                    f32 tW = halfToFloat(readU16(file));
                    //std::cout << "X=" << tX << ", Y=" << tY << ", Z=" << tZ << ", W=" << tW << std::endl;
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
        nbVertexComponentTypes = readU32(file);
        file->seek(8, true); // uint32 + 00 00 CD AB

        components.clear();
        for (u32 j = 0; j < nbVertexComponentTypes; ++j)
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
                if (c._type == VERTEX_INDICE)
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

                joint->Animatedposition = joint->LocalMatrix.getTranslation();
                joint->Animatedrotation = core::quaternion(joint->LocalMatrix.getRotationDegrees()).makeInverse();
                joint->Animatedscale = joint->LocalMatrix.getScale();
            }
            file->seek(36, true); // 00000000000000...


            // Add weights
            for (u32 j = 0; j < weights.size(); ++j)
            {
                const CEF_Weight cw = weights[j];
                if (cw._weight > 0.f)
                {
                    scene::ISkinnedMesh::SWeight* w = AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[cw._boneId]);
                    w->buffer_id = i;
                    w->strength = cw._weight;
                    w->vertex_id = cw._vertex;
                }
            }
        }

        u32 unknown2 = readU32(file);
        core::stringc effectName =readStringFixedSize(file, 256);
        std::cout << "Effect = " << effectName.c_str() << std::endl;

        if (!hasNormals)
            SceneManager->getMeshManipulator()->recalculateNormals(buffer);
    }

    return true;
}
