#include "IrrAssimpExport.h"
#include <iostream>

using namespace irr;

IrrAssimpExport::IrrAssimpExport()
{
    //ctor
}

IrrAssimpExport::~IrrAssimpExport()
{
    //dtor
}

aiColor3D IrrToAssimpColor(video::SColor color)
{
    return aiColor3D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f);
}

void IrrAssimpExport::writeFile(irr::scene::IMesh* mesh, irr::core::stringc format, irr::core::stringc filename)
{
    Assimp::Exporter exporter;

    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mNumMeshes = mesh->getMeshBufferCount();
    scene->mRootNode->mMeshes = new unsigned int[mesh->getMeshBufferCount()];
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
        scene->mRootNode->mMeshes[i] = i;

    scene->mNumMeshes = mesh->getMeshBufferCount();
    scene->mMeshes = new aiMesh*[scene->mNumMeshes];

    scene->mNumMaterials = scene->mNumMeshes;
    scene->mMaterials = new aiMaterial*[scene->mNumMeshes];

    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        aiMesh* assMesh = new aiMesh();
        irr::scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
        video::E_VERTEX_TYPE verticesType = buffer->getVertexType();

        assMesh->mNumVertices = buffer->getVertexCount();
        assMesh->mVertices = new aiVector3D[assMesh->mNumVertices];
        assMesh->mNormals = new aiVector3D[assMesh->mNumVertices];

        assMesh->mTextureCoords[0] = new aiVector3D[assMesh->mNumVertices];
        assMesh->mNumUVComponents[0] = 2;

        assMesh->mColors[0] = new aiColor4D[assMesh->mNumVertices];

        if (verticesType == video::EVT_2TCOORDS)
        {
            assMesh->mTextureCoords[1] = new aiVector3D[assMesh->mNumVertices];
            assMesh->mNumUVComponents[1] = 2;
        }
        if (verticesType == video::EVT_TANGENTS)
        {
            assMesh->mTangents = new aiVector3D[assMesh->mNumVertices];
            assMesh->mBitangents = new aiVector3D[assMesh->mNumVertices];
        }

        video::S3DVertex* vertices = (video::S3DVertex*)buffer->getVertices();

        video::S3DVertex2TCoords* tCoordsVertices;
        verticesType == video::EVT_2TCOORDS ? tCoordsVertices = (video::S3DVertex2TCoords*) vertices : tCoordsVertices = 0;

        video::S3DVertexTangents* tangentsVertices;
        verticesType == video::EVT_TANGENTS ? tangentsVertices = (video::S3DVertexTangents*) vertices : tangentsVertices = 0;

        for (unsigned int j = 0; j < buffer->getVertexCount(); ++j)
        {
            video::S3DVertex vertex = vertices[j];

            core::vector3df position = vertex.Pos;
            core::vector3df normal = vertex.Normal;
            core::vector2df uv = vertex.TCoords;
            video::SColor color = vertex.Color;

            assMesh->mVertices[j] = aiVector3D(position.X, position.Y, position.Z);
            assMesh->mNormals[j] = aiVector3D(normal.X, normal.Y, normal.Z);
            assMesh->mTextureCoords[0][j] = aiVector3D(uv.X, uv.Y, 0);
            assMesh->mColors[0][j] = aiColor4D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f, color.getAlpha() / 255.f);

            if (verticesType == video::EVT_2TCOORDS)
            {
                video::S3DVertex2TCoords tCoordsVertex = tCoordsVertices[j];
                core::vector2df uv2 = tCoordsVertex.TCoords2;
                assMesh->mTextureCoords[1][j] = aiVector3D(uv2.X, uv2.Y, 0);
            }
            if (verticesType == video::EVT_TANGENTS)
            {
                video::S3DVertexTangents tangentsVertex = tangentsVertices[j];
                core::vector3df tangent = tangentsVertex.Tangent;
                core::vector3df binormal = tangentsVertex.Binormal;

                assMesh->mTangents[j] = aiVector3D(tangent.X, tangent.Y, tangent.Z);
                assMesh->mBitangents[j] = aiVector3D(binormal.X, binormal.Y, binormal.Z);
            }
        }

        assMesh->mNumFaces = buffer->getIndexCount() / 3;
        assMesh->mFaces = new aiFace[assMesh->mNumFaces];
        for (unsigned int j = 0; j < assMesh->mNumFaces; ++j)
        {
            aiFace face;
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = buffer->getIndices()[3 * j + 0];
            face.mIndices[1] = buffer->getIndices()[3 * j + 1];
            face.mIndices[2] = buffer->getIndices()[3 * j + 2];
            assMesh->mFaces[j] = face;
        }

        scene->mMaterials[i] = new aiMaterial();
        scene->mMaterials[i]->mNumProperties = 0;
        assMesh->mMaterialIndex = i;

        video::SMaterial mat = buffer->getMaterial();

        aiColor3D diffuseColor = IrrToAssimpColor(mat.DiffuseColor);
        aiColor3D ambiantColor = IrrToAssimpColor(mat.AmbientColor);
        aiColor3D emissiveColor = IrrToAssimpColor(mat.EmissiveColor);
        aiColor3D specularColor = IrrToAssimpColor(mat.SpecularColor);
        float shininess = mat.Shininess;

        scene->mMaterials[i]->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
        scene->mMaterials[i]->AddProperty(&ambiantColor, 1, AI_MATKEY_COLOR_AMBIENT);
        scene->mMaterials[i]->AddProperty(&emissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
        scene->mMaterials[i]->AddProperty(&specularColor, 1, AI_MATKEY_COLOR_SPECULAR);
        scene->mMaterials[i]->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);

        if (mat.getTexture(0))
        {
            aiString textureName = aiString(to_char_string(mat.getTexture(0)->getName().getPath()).c_str());
            scene->mMaterials[i]->AddProperty(&textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));
        }
        if (mat.getTexture(1))
        {
            // Normal map
            if (   mat.MaterialType == video::EMT_NORMAL_MAP_SOLID
                || mat.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR
                || mat.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA
                || mat.MaterialType == video::EMT_PARALLAX_MAP_SOLID
                || mat.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR
                || mat.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA)
            {
                aiString textureName = aiString(to_char_string(mat.getTexture(1)->getName().getPath()).c_str());
                scene->mMaterials[i]->AddProperty(&textureName, AI_MATKEY_TEXTURE_NORMALS(0));
            }

        }


        scene->mMeshes[i] = assMesh;
    }

    exporter.Export(scene, format.c_str(), to_char_string(filename).c_str(), aiProcess_FlipUVs);

    delete scene;
}

