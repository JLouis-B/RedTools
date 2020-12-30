#include "IrrAssimpExport.h"
#include <iostream>

#include <ITexture.h>

using namespace irr;

IrrAssimpExport::IrrAssimpExport() : m_assimpScene(0)
{
    //ctor
}

IrrAssimpExport::~IrrAssimpExport()
{
    //dtor
}

aiString irrToAssimpString(const core::stringc& str)
{
    return aiString(str.c_str());
}

aiVector3D irrToAssimpVector3(const core::vector2df& vect)
{
    return aiVector3D(vect.X, vect.Y, 0.f);
}

aiVector3D irrToAssimpVector3(const core::vector3df& vect)
{
    return aiVector3D(vect.X, vect.Y, vect.Z);
}

aiQuaternion irrToAssimpQuaternion(const core::quaternion& quat)
{
    return aiQuaternion(-quat.W, quat.X, quat.Y, quat.Z);
}

aiMatrix4x4 irrToAssimpMatrix(const core::matrix4& irrMatrix)
{
    aiMatrix4x4 assimpMatrix;

    assimpMatrix.a1 = irrMatrix[0];
    assimpMatrix.b1 = irrMatrix[1];
    assimpMatrix.c1 = irrMatrix[2];
    assimpMatrix.d1 = irrMatrix[3];

    assimpMatrix.a2 = irrMatrix[4];
    assimpMatrix.b2 = irrMatrix[5];
    assimpMatrix.c2 = irrMatrix[6];
    assimpMatrix.d2 = irrMatrix[7];

    assimpMatrix.a3 = irrMatrix[8];
    assimpMatrix.b3 = irrMatrix[9];
    assimpMatrix.c3 = irrMatrix[10];
    assimpMatrix.d3 = irrMatrix[11];

    assimpMatrix.a4 = irrMatrix[12];
    assimpMatrix.b4 = irrMatrix[13];
    assimpMatrix.c4 = irrMatrix[14];
    assimpMatrix.d4 = irrMatrix[15];

    return assimpMatrix;
}

aiColor3D irrToAssimpColor3(const video::SColor& color)
{
    return aiColor3D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f);
}

aiColor4D irrToAssimpColor4(const video::SColor& color)
{
    return aiColor4D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f, color.getAlpha() / 255.f);
}

core::array<scene::ISkinnedMesh::SJoint*> getRootJoints(const scene::ISkinnedMesh* mesh)
{
    core::array<scene::ISkinnedMesh::SJoint*> roots;

    core::array<scene::ISkinnedMesh::SJoint*> allJoints = mesh->getAllJoints();
    for (u32 i = 0; i < allJoints.size(); i++)
    {
        bool isRoot = true;
        scene::ISkinnedMesh::SJoint* testedJoint = allJoints[i];
        for (u32 j = 0; j < allJoints.size(); j++)
        {
           scene::ISkinnedMesh::SJoint* testedJoint2 = allJoints[j];
           for (u32 k = 0; k < testedJoint2->Children.size(); k++)
           {
               if (testedJoint == testedJoint2->Children[k])
                    isRoot = false;
           }
        }
        if (isRoot)
            roots.push_back(testedJoint);
    }

    return roots;
}

core::array<u16> IrrAssimpExport::getMeshesMovedByBone(const scene::ISkinnedMesh::SJoint* joint)
{
    core::array<u16> buffers;

    for (auto it = m_bonesPerMesh.begin(); it != m_bonesPerMesh.end(); it++)
    {
        core::array<const scene::ISkinnedMesh::SJoint*> bones = it->second;
        if (bones.linear_search(joint) != -1)
        {
            buffers.push_back(it->first);
        }
    }

    return buffers;
}

aiNode* IrrAssimpExport::createNode(const scene::ISkinnedMesh::SJoint* irrJoint)
{
    aiNode* node = new aiNode();
    node->mName = irrToAssimpString(irrJoint->Name);
    node->mTransformation = irrToAssimpMatrix(irrJoint->LocalMatrix);


    node->mNumMeshes = irrJoint->AttachedMeshes.size();
    node->mMeshes = new unsigned int[irrJoint->AttachedMeshes.size()];
    for (u32 i = 0; i < irrJoint->AttachedMeshes.size(); ++i)
    {
        const u32 attachedBuffer = irrJoint->AttachedMeshes[i];
        node->mMeshes[i] = attachedBuffer;
        if (m_attachedBuffers.linear_search(attachedBuffer) == -1)
            m_attachedBuffers.push_back(attachedBuffer);
    }


    core::array<u16> meshes = getMeshesMovedByBone(irrJoint);
    for (u32 i = 0; i < meshes.size(); ++i)
    {
        aiBone* bone = new aiBone();
        const u16 meshId = meshes[i];
        bone->mName = node->mName;


        bone->mNumWeights = 0;
        bone->mWeights = new aiVertexWeight[m_weightsCountPerMeshesAndBones[std::make_pair(meshId, irrJoint)]];
        for (u32 j = 0; j < irrJoint->Weights.size(); ++j)
        {
            const scene::ISkinnedMesh::SWeight& w = irrJoint->Weights[j];
            if (w.buffer_id == meshes[i])
            {
                bone->mWeights[bone->mNumWeights].mWeight = w.strength;
                bone->mWeights[bone->mNumWeights].mVertexId = w.vertex_id;
                bone->mNumWeights++;
            }
        }

        bone->mOffsetMatrix = irrToAssimpMatrix(irrJoint->GlobalMatrix).Inverse();
        bone->mNode = node;

        m_assimpScene->mMeshes[meshId]->mBones[m_assimpScene->mMeshes[meshId]->mNumBones] = bone;
        m_assimpScene->mMeshes[meshId]->mNumBones++;
    }


    node->mNumChildren = irrJoint->Children.size();
    node->mChildren = new aiNode*[irrJoint->Children.size()];
    for (u32 i = 0; i < irrJoint->Children.size(); ++i)
    {
        aiNode* child = createNode(irrJoint->Children[i]);
        node->mChildren[i] = child;
    }
    return node;
}

void IrrAssimpExport::createAnimations(const irr::scene::ISkinnedMesh* irrMesh)
{
    if (irrMesh->getFrameCount() == 0 || irrMesh->getFrameCount() == 1)
    {
        m_assimpScene->mNumAnimations = 0;
        return;
    }

    m_assimpScene->mNumAnimations = 1;
    m_assimpScene->mAnimations = new aiAnimation*[1];
    aiAnimation* animation = new aiAnimation();

    core::array<const scene::ISkinnedMesh::SJoint*> irrJoints;
    for (u32 i = 0; i < irrMesh->getJointCount(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* joint = irrMesh->getAllJoints()[i];
        if (joint->PositionKeys.size() + joint->RotationKeys.size() + joint->ScaleKeys.size() > 0)
            irrJoints.push_back(joint);
    }


    animation->mChannels = new aiNodeAnim*[irrJoints.size()];
    animation->mNumChannels = irrJoints.size();

    animation->mNumMeshChannels = 0;

    animation->mDuration = irrMesh->getFrameCount();
    animation->mTicksPerSecond = irrMesh->getAnimationSpeed();
    animation->mName = aiString("default");

    for (u32 i = 0; i < irrJoints.size(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* irrJoint = irrJoints[i];

        aiNodeAnim* channel = new aiNodeAnim();
        channel->mNodeName = irrToAssimpString(irrJoint->Name);

        channel->mNumPositionKeys = irrJoint->PositionKeys.size();
        channel->mPositionKeys = new aiVectorKey[irrJoint->PositionKeys.size()];
        for (u32 j = 0; j < irrJoint->PositionKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SPositionKey key = irrJoint->PositionKeys[j];
            channel->mPositionKeys[j].mTime = key.frame;
            channel->mPositionKeys[j].mValue = irrToAssimpVector3(key.position);
        }

        channel->mNumRotationKeys = irrJoint->RotationKeys.size();
        channel->mRotationKeys = new aiQuatKey[irrJoint->RotationKeys.size()];
        for (u32 j = 0; j < irrJoint->RotationKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SRotationKey key = irrJoint->RotationKeys[j];
            channel->mRotationKeys[j].mTime = key.frame;
            channel->mRotationKeys[j].mValue = irrToAssimpQuaternion(key.rotation);
        }

        channel->mNumScalingKeys = irrJoint->ScaleKeys.size();
        channel->mScalingKeys = new aiVectorKey[irrJoint->ScaleKeys.size()];
        for (u32 j = 0; j < irrJoint->ScaleKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SScaleKey key = irrJoint->ScaleKeys[j];
            channel->mScalingKeys[j].mTime = key.frame;
            channel->mScalingKeys[j].mValue = irrToAssimpVector3(key.scale);
        }

        animation->mChannels[i] = channel;
    }

    m_assimpScene->mAnimations[0] = animation;
}

void IrrAssimpExport::createMaterials(const scene::IMesh* irrMesh)
{
    m_assimpScene->mNumMaterials = irrMesh->getMeshBufferCount();
    m_assimpScene->mMaterials = new aiMaterial*[m_assimpScene->mNumMaterials];
    for (unsigned int i = 0; i < irrMesh->getMeshBufferCount(); ++i)
    {
        aiMaterial* material = new aiMaterial();

        video::SMaterial irrMaterial = irrMesh->getMeshBuffer(i)->getMaterial();

        aiColor3D diffuseColor = irrToAssimpColor3(irrMaterial.DiffuseColor);
        aiColor3D ambiantColor = irrToAssimpColor3(irrMaterial.AmbientColor);
        aiColor3D emissiveColor = irrToAssimpColor3(irrMaterial.EmissiveColor);
        aiColor3D specularColor = irrToAssimpColor3(irrMaterial.SpecularColor);
        float shininess = irrMaterial.Shininess;

        material->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
        material->AddProperty(&ambiantColor, 1, AI_MATKEY_COLOR_AMBIENT);
        material->AddProperty(&emissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
        material->AddProperty(&specularColor, 1, AI_MATKEY_COLOR_SPECULAR);
        material->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);

        if (irrMaterial.getTexture(0))
        {
            aiString textureName = irrToAssimpPath(irrMaterial.getTexture(0)->getName().getPath());
            material->AddProperty(&textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));
        }
        if (irrMaterial.getTexture(1))
        {
            // Normal map
            if (   irrMaterial.MaterialType == video::EMT_NORMAL_MAP_SOLID
                || irrMaterial.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_ADD_COLOR
                || irrMaterial.MaterialType == video::EMT_NORMAL_MAP_TRANSPARENT_VERTEX_ALPHA
                || irrMaterial.MaterialType == video::EMT_PARALLAX_MAP_SOLID
                || irrMaterial.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_ADD_COLOR
                || irrMaterial.MaterialType == video::EMT_PARALLAX_MAP_TRANSPARENT_VERTEX_ALPHA)
            {
                aiString textureName = irrToAssimpPath(irrMaterial.getTexture(1)->getName().getPath());
                material->AddProperty(&textureName, AI_MATKEY_TEXTURE_NORMALS(0));
            }

        }

        m_assimpScene->mMaterials[i] = material;
    }
}

void IrrAssimpExport::createMeshes(const scene::IMesh* irrMesh)
{
    m_assimpScene->mNumMeshes = irrMesh->getMeshBufferCount();
    m_assimpScene->mMeshes = new aiMesh*[m_assimpScene->mNumMeshes];
    for (unsigned int i = 0; i < irrMesh->getMeshBufferCount(); ++i)
    {
        aiMesh* mesh = new aiMesh();
        irr::scene::IMeshBuffer* irrBuffer = irrMesh->getMeshBuffer(i);
        video::E_VERTEX_TYPE verticesType = irrBuffer->getVertexType();

        mesh->mNumVertices = irrBuffer->getVertexCount();
        mesh->mVertices = new aiVector3D[mesh->mNumVertices];
        mesh->mNormals = new aiVector3D[mesh->mNumVertices];

        mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
        mesh->mNumUVComponents[0] = 2;

        mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];

        if (verticesType == video::EVT_2TCOORDS)
        {
            mesh->mTextureCoords[1] = new aiVector3D[mesh->mNumVertices];
            mesh->mNumUVComponents[1] = 2;
        }
        if (verticesType == video::EVT_TANGENTS)
        {
            mesh->mTangents = new aiVector3D[mesh->mNumVertices];
            mesh->mBitangents = new aiVector3D[mesh->mNumVertices];
        }

        void* irrVertices = (void*)irrBuffer->getVertices();
        for (unsigned int j = 0; j < irrBuffer->getVertexCount(); ++j)
        {
            mesh->mVertices[j] = irrToAssimpVector3(irrBuffer->getPosition(j));
            mesh->mNormals[j] = irrToAssimpVector3(irrBuffer->getNormal(j));
            mesh->mTextureCoords[0][j] = irrToAssimpVector3(irrBuffer->getTCoords(j));

            switch(verticesType)
            {
                case video::EVT_STANDARD:
                {
                    video::S3DVertex irrVertex = ((video::S3DVertex*)irrVertices)[j];
                    mesh->mColors[0][j] = irrToAssimpColor4(irrVertex.Color);
                    break;
                }
                case video::EVT_2TCOORDS:
                {
                    video::S3DVertex2TCoords irrVertex = ((video::S3DVertex2TCoords*)irrVertices)[j];
                    mesh->mColors[0][j] = irrToAssimpColor4(irrVertex.Color);
                    mesh->mTextureCoords[1][j] = irrToAssimpVector3(irrVertex.TCoords2);
                    break;
                }
                case video::EVT_TANGENTS:
                {
                    video::S3DVertexTangents irrVertex = ((video::S3DVertexTangents*)irrVertices)[j];
                    mesh->mColors[0][j] = irrToAssimpColor4(irrVertex.Color);
                    mesh->mTangents[j] = irrToAssimpVector3(irrVertex.Tangent);
                    mesh->mBitangents[j] = irrToAssimpVector3(irrVertex.Binormal);
                    break;
                }
            }
        }

        mesh->mNumFaces = irrBuffer->getIndexCount() / 3;
        mesh->mFaces = new aiFace[mesh->mNumFaces];
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
        {
            aiFace face;
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = irrBuffer->getIndices()[3 * j + 0];
            face.mIndices[1] = irrBuffer->getIndices()[3 * j + 1];
            face.mIndices[2] = irrBuffer->getIndices()[3 * j + 2];
            mesh->mFaces[j] = face;
        }

        mesh->mMaterialIndex = i;

        mesh->mNumBones = 0;
        mesh->mBones = new aiBone*[m_bonesPerMesh[i].size()];

        m_assimpScene->mMeshes[i] = mesh;
    }
}

void IrrAssimpExport::writeFile(scene::IMesh* mesh, core::stringc format, core::stringc filename)
{
    scene::ISkinnedMesh* irrSkinnedMesh = nullptr;
#if (IRRLICHT_VERSION_MAJOR >= 2) || (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >= 9)
    if (mesh->getMeshType() == scene::EAMT_SKINNED)
    {
        irrSkinnedMesh = static_cast<scene::ISkinnedMesh*>(mesh);
        irrSkinnedMesh->setHardwareSkinning(true); //seems to be a cheat to get static pose


        // Count some stuffs needed later
        m_attachedBuffers.clear();
        m_bonesPerMesh.clear();
        m_weightsCountPerMeshesAndBones.clear();
        for (u32 i = 0; i < irrSkinnedMesh->getMeshBufferCount(); ++i)
        {
            m_bonesPerMesh.insert(std::make_pair(i, core::array<const scene::ISkinnedMesh::SJoint*>()));
        }

        for (u32 i = 0; i < irrSkinnedMesh->getAllJoints().size(); ++i)
        {
            const scene::ISkinnedMesh::SJoint* irrJoint = irrSkinnedMesh->getAllJoints()[i];
            for (u32 j = 0; j < irrJoint->Weights.size(); ++j)
            {
                const scene::ISkinnedMesh::SWeight& w = irrJoint->Weights[j];
                if (m_bonesPerMesh[w.buffer_id].linear_search(irrJoint) == -1)
                {
                    m_bonesPerMesh[w.buffer_id].push_back(irrJoint);
                }

                std::pair<u16, const scene::ISkinnedMesh::SJoint*> meshBone = std::make_pair(w.buffer_id, irrJoint);
                if (m_weightsCountPerMeshesAndBones.find(meshBone) == m_weightsCountPerMeshesAndBones.end())
                {
                    m_weightsCountPerMeshesAndBones.insert(std::make_pair(meshBone, 1));
                }
                else
                {
                    m_weightsCountPerMeshesAndBones[meshBone]++;
                }
            }
        }
    }
#endif

    Assimp::Exporter exporter;

    m_assimpScene = new aiScene();

    // Load materials
    createMaterials(mesh);

    // Load meshes
    createMeshes(mesh);

    m_assimpScene->mRootNode = new aiNode("IRRASSIMP_ROOT");
    if (irrSkinnedMesh)
    {
        createAnimations(irrSkinnedMesh);

        core::array<scene::ISkinnedMesh::SJoint*> roots = getRootJoints(irrSkinnedMesh);
        m_assimpScene->mRootNode->mNumChildren = roots.size();
        m_assimpScene->mRootNode->mChildren = new aiNode*[roots.size()];
        for (u32 i = 0; i < roots.size(); ++i)
        {
            m_assimpScene->mRootNode->mChildren[i] = createNode(roots[i]);
        }

        irrSkinnedMesh->setHardwareSkinning(false);
    }

    const u32 attachedMeshesToRootCount = mesh->getMeshBufferCount() - m_attachedBuffers.size();
    m_assimpScene->mRootNode->mNumMeshes = attachedMeshesToRootCount;
    m_assimpScene->mRootNode->mMeshes = new unsigned int[attachedMeshesToRootCount];
    u32 meshesAddedCount = 0;
    for (u32 i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        if (m_attachedBuffers.linear_search(i) == -1)
        {
            m_assimpScene->mRootNode->mMeshes[meshesAddedCount] = i;
            meshesAddedCount++;
        }
    }

    exporter.Export(m_assimpScene, format.c_str(), filename.c_str(), aiProcess_FlipUVs);

	// Delete the scene
    delete m_assimpScene;
}

