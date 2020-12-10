#include "IrrAssimpExport.h"
#include <iostream>

using namespace irr;

IrrAssimpExport::IrrAssimpExport() : AssimpScene(0)
{
    //ctor
}

IrrAssimpExport::~IrrAssimpExport()
{
    //dtor
}

aiColor3D IrrToAssimpColor3(video::SColor color)
{
    return aiColor3D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f);
}

aiColor4D IrrToAssimpColor4(video::SColor color)
{
    return aiColor4D(color.getRed() / 255.f, color.getGreen() / 255.f, color.getBlue() / 255.f, color.getAlpha() / 255.f);
}

aiVector3D IrrToAssimpVector3(core::vector3df vect)
{
    return aiVector3D(vect.X, vect.Y, vect.Z);
}

aiQuaternion IrrToAssimpQuaternion(core::quaternion quat)
{
    return aiQuaternion(-quat.W, quat.X, quat.Y, quat.Z);
}

aiMatrix4x4 IrrToAssimpMatrix(core::matrix4 irrMatrix)
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
        if (bones.binary_search(joint) != -1)
        {
            buffers.push_back(it->first);
        }
    }

    return buffers;
}

aiNode* IrrAssimpExport::createNode(const scene::ISkinnedMesh::SJoint* joint)
{
    aiNode* node = new aiNode();
    node->mName = aiString(joint->Name.c_str());
    node->mTransformation = IrrToAssimpMatrix(joint->LocalMatrix);


    node->mNumMeshes = joint->AttachedMeshes.size();
    node->mMeshes = new unsigned int[joint->AttachedMeshes.size()];
    for (u32 i = 0; i < joint->AttachedMeshes.size(); ++i)
    {
        node->mMeshes[i] = joint->AttachedMeshes[i];
    }


    core::array<u16> meshes = getMeshesMovedByBone(joint);
    for (u32 i = 0; i < meshes.size(); ++i)
    {
        aiBone* bone = new aiBone();
        const u16 meshId = meshes[i];
        bone->mName = aiString(joint->Name.c_str());


        bone->mNumWeights = 0;
        bone->mWeights = new aiVertexWeight[m_weightsCountPerMeshesAndBones[std::make_pair(meshId, joint)]];
        for (u32 j = 0; j < joint->Weights.size(); ++j)
        {
            const scene::ISkinnedMesh::SWeight& w = joint->Weights[j];
            if (w.buffer_id == meshes[i])
            {
                bone->mWeights[bone->mNumWeights].mWeight = w.strength;
                bone->mWeights[bone->mNumWeights].mVertexId = w.vertex_id;
                bone->mNumWeights++;
            }
        }

        bone->mOffsetMatrix = IrrToAssimpMatrix(joint->GlobalMatrix);
        bone->mNode = node;

        AssimpScene->mMeshes[meshId]->mBones[AssimpScene->mMeshes[meshId]->mNumBones] = bone;
        AssimpScene->mMeshes[meshId]->mNumBones++;
    }


    node->mNumChildren = joint->Children.size();
    node->mChildren = new aiNode*[joint->Children.size()];
    for (u32 i = 0; i < joint->Children.size(); ++i)
    {
        aiNode* child = createNode(joint->Children[i]);
        node->mChildren[i] = child;
    }
    return node;
}

void IrrAssimpExport::createAnimations(const irr::scene::ISkinnedMesh* mesh)
{
    if (mesh->getFrameCount() == 0 || mesh->getFrameCount() == 1)
    {
        AssimpScene->mNumAnimations = 0;
        return;
    }

    AssimpScene->mNumAnimations = 1;
    AssimpScene->mAnimations = new aiAnimation*[1];
    aiAnimation* animation = new aiAnimation();

    core::array<const scene::ISkinnedMesh::SJoint*> joints;
    for (u32 i = 0; i < mesh->getJointCount(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* joint = mesh->getAllJoints()[i];
        if (joint->PositionKeys.size() + joint->RotationKeys.size() + joint->ScaleKeys.size() > 0)
            joints.push_back(joint);
    }


    animation->mChannels = new aiNodeAnim*[joints.size()];
    animation->mNumChannels = joints.size();

    animation->mNumMeshChannels = 0;

    animation->mDuration = mesh->getFrameCount();
    animation->mTicksPerSecond = mesh->getAnimationSpeed();
    animation->mName = aiString("default");

    for (u32 i = 0; i < joints.size(); ++i)
    {
        const scene::ISkinnedMesh::SJoint* joint = joints[i];

        aiNodeAnim* channel = new aiNodeAnim();
        channel->mNodeName = aiString(joint->Name.c_str());

        channel->mNumPositionKeys = joint->PositionKeys.size();
        channel->mPositionKeys = new aiVectorKey[joint->PositionKeys.size()];
        for (u32 j = 0; j < joint->PositionKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SPositionKey key = joint->PositionKeys[j];
            channel->mPositionKeys[j].mTime = key.frame;
            channel->mPositionKeys[j].mValue = IrrToAssimpVector3(key.position);
        }

        channel->mNumRotationKeys = joint->RotationKeys.size();
        channel->mRotationKeys = new aiQuatKey[joint->RotationKeys.size()];
        for (u32 j = 0; j < joint->RotationKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SRotationKey key = joint->RotationKeys[j];
            channel->mRotationKeys[j].mTime = key.frame;
            channel->mRotationKeys[j].mValue = IrrToAssimpQuaternion(key.rotation);
        }

        channel->mNumScalingKeys = joint->ScaleKeys.size();
        channel->mScalingKeys = new aiVectorKey[joint->ScaleKeys.size()];
        for (u32 j = 0; j < joint->ScaleKeys.size(); ++j)
        {
            const scene::ISkinnedMesh::SScaleKey key = joint->ScaleKeys[j];
            channel->mScalingKeys[j].mTime = key.frame;
            channel->mScalingKeys[j].mValue = IrrToAssimpVector3(key.scale);
        }

        animation->mChannels[i] = channel;
    }

    AssimpScene->mAnimations[0] = animation;
}

void IrrAssimpExport::createMaterials(const scene::IMesh* mesh)
{
    AssimpScene->mNumMaterials = mesh->getMeshBufferCount();
    AssimpScene->mMaterials = new aiMaterial*[AssimpScene->mNumMaterials];
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        AssimpScene->mMaterials[i] = new aiMaterial();
        AssimpScene->mMaterials[i]->mNumProperties = 0;


        video::SMaterial mat = mesh->getMeshBuffer(i)->getMaterial();

        aiColor3D diffuseColor = IrrToAssimpColor3(mat.DiffuseColor);
        aiColor3D ambiantColor = IrrToAssimpColor3(mat.AmbientColor);
        aiColor3D emissiveColor = IrrToAssimpColor3(mat.EmissiveColor);
        aiColor3D specularColor = IrrToAssimpColor3(mat.SpecularColor);
        float shininess = mat.Shininess;

        AssimpScene->mMaterials[i]->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);
        AssimpScene->mMaterials[i]->AddProperty(&ambiantColor, 1, AI_MATKEY_COLOR_AMBIENT);
        AssimpScene->mMaterials[i]->AddProperty(&emissiveColor, 1, AI_MATKEY_COLOR_EMISSIVE);
        AssimpScene->mMaterials[i]->AddProperty(&specularColor, 1, AI_MATKEY_COLOR_SPECULAR);
        AssimpScene->mMaterials[i]->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);

        if (mat.getTexture(0))
        {
            aiString textureName = aiString(to_char_string(mat.getTexture(0)->getName().getPath()).c_str());
            AssimpScene->mMaterials[i]->AddProperty(&textureName, AI_MATKEY_TEXTURE_DIFFUSE(0));
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
                AssimpScene->mMaterials[i]->AddProperty(&textureName, AI_MATKEY_TEXTURE_NORMALS(0));
            }

        }
    }
}

void IrrAssimpExport::createMeshes(const scene::IMesh* mesh)
{
    AssimpScene->mNumMeshes = mesh->getMeshBufferCount();
    AssimpScene->mMeshes = new aiMesh*[AssimpScene->mNumMeshes];
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
    {
        aiMesh* assimpMesh = new aiMesh();
        irr::scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
        video::E_VERTEX_TYPE verticesType = buffer->getVertexType();

        assimpMesh->mNumVertices = buffer->getVertexCount();
        assimpMesh->mVertices = new aiVector3D[assimpMesh->mNumVertices];
        assimpMesh->mNormals = new aiVector3D[assimpMesh->mNumVertices];

        assimpMesh->mTextureCoords[0] = new aiVector3D[assimpMesh->mNumVertices];
        assimpMesh->mNumUVComponents[0] = 2;

        assimpMesh->mColors[0] = new aiColor4D[assimpMesh->mNumVertices];

        if (verticesType == video::EVT_2TCOORDS)
        {
            assimpMesh->mTextureCoords[1] = new aiVector3D[assimpMesh->mNumVertices];
            assimpMesh->mNumUVComponents[1] = 2;
        }
        if (verticesType == video::EVT_TANGENTS)
        {
            assimpMesh->mTangents = new aiVector3D[assimpMesh->mNumVertices];
            assimpMesh->mBitangents = new aiVector3D[assimpMesh->mNumVertices];
        }

        void* vertices = (void*)buffer->getVertices();
        for (unsigned int j = 0; j < buffer->getVertexCount(); ++j)
        {
            core::vector3df position = buffer->getPosition(j);
            core::vector3df normal = buffer->getNormal(j);
            core::vector2df uv = buffer->getTCoords(j);
            assimpMesh->mVertices[j] = aiVector3D(position.X, position.Y, position.Z);
            assimpMesh->mNormals[j] = aiVector3D(normal.X, normal.Y, normal.Z);
            assimpMesh->mTextureCoords[0][j] = aiVector3D(uv.X, uv.Y, 0);

            switch(verticesType)
            {
                case video::EVT_STANDARD:
                {
                    video::S3DVertex vertex = ((video::S3DVertex*)vertices)[j];
                    video::SColor color = vertex.Color;
                    assimpMesh->mColors[0][j] = IrrToAssimpColor4(color);
                    break;
                }
                case video::EVT_2TCOORDS:
                {
                    video::S3DVertex2TCoords vertex = ((video::S3DVertex2TCoords*)vertices)[j];
                    video::SColor color = vertex.Color;
                    assimpMesh->mColors[0][j] = IrrToAssimpColor4(color);

                    core::vector2df uv2 = vertex.TCoords2;
                    assimpMesh->mTextureCoords[1][j] = aiVector3D(uv2.X, uv2.Y, 0);
                    break;
                }
                case video::EVT_TANGENTS:
                {
                    video::S3DVertexTangents vertex = ((video::S3DVertexTangents*)vertices)[j];
                    video::SColor color = vertex.Color;
                    assimpMesh->mColors[0][j] = IrrToAssimpColor4(color);

                    core::vector3df tangent = vertex.Tangent;
                    core::vector3df binormal = vertex.Binormal;

                    assimpMesh->mTangents[j] = aiVector3D(tangent.X, tangent.Y, tangent.Z);
                    assimpMesh->mBitangents[j] = aiVector3D(binormal.X, binormal.Y, binormal.Z);
                    break;
                }
            }
        }

        assimpMesh->mNumFaces = buffer->getIndexCount() / 3;
        assimpMesh->mFaces = new aiFace[assimpMesh->mNumFaces];
        for (unsigned int j = 0; j < assimpMesh->mNumFaces; ++j)
        {
            aiFace face;
            face.mNumIndices = 3;
            face.mIndices = new unsigned int[3];
            face.mIndices[0] = buffer->getIndices()[3 * j + 0];
            face.mIndices[1] = buffer->getIndices()[3 * j + 1];
            face.mIndices[2] = buffer->getIndices()[3 * j + 2];
            assimpMesh->mFaces[j] = face;
        }

        assimpMesh->mMaterialIndex = i;

        assimpMesh->mNumBones = 0;
        assimpMesh->mBones = new aiBone*[m_bonesPerMesh[i].size()];

        AssimpScene->mMeshes[i] = assimpMesh;
    }
}

void IrrAssimpExport::writeFile(scene::IMesh* mesh, core::stringc format, core::stringc filename)
{
    scene::ISkinnedMesh* skinnedMesh = nullptr;
#if (IRRLICHT_VERSION_MAJOR >= 2) || (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR >= 9)
    if (mesh->getMeshType() == scene::EAMT_SKINNED)
    {
        skinnedMesh = static_cast<scene::ISkinnedMesh*>(mesh);
        skinnedMesh->setHardwareSkinning(true); //seems to be a cheat to get static pose


        // Count some stuffs needed later
        m_bonesPerMesh.clear();
        m_weightsCountPerMeshesAndBones.clear();
        for (u32 i = 0; i < skinnedMesh->getMeshBufferCount(); ++i)
        {
            m_bonesPerMesh.insert(std::make_pair(i, core::array<const scene::ISkinnedMesh::SJoint*>()));
        }

        for (u32 i = 0; i < skinnedMesh->getAllJoints().size(); ++i)
        {
            const scene::ISkinnedMesh::SJoint* joint = skinnedMesh->getAllJoints()[i];
            for (u32 j = 0; j < joint->Weights.size(); ++j)
            {
                const scene::ISkinnedMesh::SWeight& w = joint->Weights[j];
                if (m_bonesPerMesh[w.buffer_id].binary_search(joint) == -1)
                {
                    m_bonesPerMesh[w.buffer_id].push_back(joint);
                }

                std::pair<u16, const scene::ISkinnedMesh::SJoint*> meshBone = std::make_pair(w.buffer_id, joint);
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

    AssimpScene = new aiScene();

    AssimpScene->mRootNode = new aiNode("IRRASSIMP_ROOT");
	AssimpScene->mRootNode->mNumMeshes = mesh->getMeshBufferCount();
    AssimpScene->mRootNode->mMeshes = new unsigned int[mesh->getMeshBufferCount()];
    for (unsigned int i = 0; i < mesh->getMeshBufferCount(); ++i)
        AssimpScene->mRootNode->mMeshes[i] = i;

    // Load materials
    createMaterials(mesh);

    // Load meshes
    createMeshes(mesh);

    if (skinnedMesh)
    {
        createAnimations(skinnedMesh);

        core::array<scene::ISkinnedMesh::SJoint*> roots = getRootJoints(skinnedMesh);
        AssimpScene->mRootNode->mNumChildren = roots.size();
        AssimpScene->mRootNode->mChildren = new aiNode*[roots.size()];
        for (u32 i = 0; i < roots.size(); ++i)
        {
            AssimpScene->mRootNode->mChildren[i] = createNode(roots[i]);
        }

        skinnedMesh->setHardwareSkinning(false);
    }

    exporter.Export(AssimpScene, format.c_str(), to_char_string(filename).c_str(), aiProcess_FlipUVs);

	// Delete the scene
	delete AssimpScene;
}

