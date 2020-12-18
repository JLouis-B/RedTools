#include "IrrAssimpImport.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>
#include <IMeshManipulator.h>

using namespace irr;

IrrAssimpImport::IrrAssimpImport(scene::ISceneManager* smgr) : m_sceneManager(smgr), m_fileSystem(smgr->getFileSystem())
{
    //ctor
}

IrrAssimpImport::~IrrAssimpImport()
{
    //dtor
}

core::stringc assimpToIrrString(aiString str)
{
    return core::stringc(str.C_Str());
}

core::vector3df assimpToIrrVector3(const aiVector3D& vect)
{
    return core::vector3df(vect.x, vect.y, vect.z);
}

core::vector2df assimpToIrrVector2(const aiVector3D& vect)
{
    return core::vector2df(vect.x, vect.y);
}

core::quaternion assimpToIrrQuaternion(const aiQuaternion& quat)
{
    return core::quaternion(quat.x, quat.y, quat.z, -quat.w);
}

core::matrix4 assimpToIrrMatrix(aiMatrix4x4 assimpMatrix)
{
    core::matrix4 irrMatrix;

    irrMatrix[0] = assimpMatrix.a1;
    irrMatrix[1] = assimpMatrix.b1;
    irrMatrix[2] = assimpMatrix.c1;
    irrMatrix[3] = assimpMatrix.d1;

    irrMatrix[4] = assimpMatrix.a2;
    irrMatrix[5] = assimpMatrix.b2;
    irrMatrix[6] = assimpMatrix.c2;
    irrMatrix[7] = assimpMatrix.d2;

    irrMatrix[8] = assimpMatrix.a3;
    irrMatrix[9] = assimpMatrix.b3;
    irrMatrix[10] = assimpMatrix.c3;
    irrMatrix[11] = assimpMatrix.d3;

    irrMatrix[12] = assimpMatrix.a4;
    irrMatrix[13] = assimpMatrix.b4;
    irrMatrix[14] = assimpMatrix.c4;
    irrMatrix[15] = assimpMatrix.d4;

    return irrMatrix;
}

video::SColor assimpToIrrColor4(const aiColor4D& color)
{
    return video::SColor(static_cast<u32>(color.a * 255), static_cast<u32>(color.r * 255), static_cast<u32>(color.g * 255), static_cast<u32>(color.b * 255));
}


scene::ISkinnedMesh::SJoint* IrrAssimpImport::findJoint(const core::stringc jointName)
{
    for (unsigned int i = 0; i < m_irrMesh->getJointCount(); ++i)
    {
        if (core::stringc(m_irrMesh->getJointName(i)) == jointName)
            return m_irrMesh->getAllJoints()[i];
    }
    //std::cout << "Error, no joint" << std::endl;
    return 0;
}

aiNode* IrrAssimpImport::findNode(const aiString jointName)
{
    if (m_assimpScene->mRootNode->mName == jointName)
        return m_assimpScene->mRootNode;

    return m_assimpScene->mRootNode->FindNode(jointName);
}

void IrrAssimpImport::createNode(const aiNode* node)
{
    scene::ISkinnedMesh::SJoint* jointParent = 0;

    if (node->mParent != 0)
    {
        jointParent = findJoint(assimpToIrrString(node->mParent->mName));
    }

    scene::ISkinnedMesh::SJoint* joint = m_irrMesh->addJoint(jointParent);
    joint->Name = assimpToIrrString(node->mName);
    joint->LocalMatrix = assimpToIrrMatrix(node->mTransformation);

    if (jointParent)
        joint->GlobalMatrix = jointParent->GlobalMatrix * joint->LocalMatrix;
    else
        joint->GlobalMatrix = joint->LocalMatrix;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        joint->AttachedMeshes.push_back(node->mMeshes[i]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        aiNode* childNode = node->mChildren[i];
        createNode(childNode);
    }
}

video::ITexture* IrrAssimpImport::getTexture(core::stringc path, core::stringc fileDir)
{
    video::ITexture* texture = 0;

    if (m_fileSystem->existFile(path.c_str()))
        texture = m_sceneManager->getVideoDriver()->getTexture(path.c_str());
    else if (m_fileSystem->existFile(fileDir + "/" + path.c_str()))
        texture = m_sceneManager->getVideoDriver()->getTexture(fileDir + "/" + path.c_str());
    else if (m_fileSystem->existFile(fileDir + "/" + m_fileSystem->getFileBasename(path.c_str())))
        texture = m_sceneManager->getVideoDriver()->getTexture(fileDir + "/" + m_fileSystem->getFileBasename(path.c_str()));

    return texture;
    // TODO after 1.9 release : Rewrite this with IMeshTextureLoader
}


bool IrrAssimpImport::isALoadableFileExtension(const io::path& filename) const
{
    Assimp::Importer importer;

    io::path extension;
    core::getFileNameExtension(extension, filename);
    return importer.IsExtensionSupported (extension.c_str());
}

scene::IAnimatedMesh* IrrAssimpImport::createMesh(io::IReadFile* file)
{
    m_filePath = file->getFileName();
    Assimp::Importer importer;

    m_assimpScene = importer.ReadFile(irrToAssimpPath(file->getFileName()).C_Str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
    if (!m_assimpScene)
    {
        error = importer.GetErrorString();
        return 0;
    }
    else
        error = "";

    // Create mesh
    m_irrMesh = m_sceneManager->createSkinnedMesh();

    // Load materials
    createMaterials();

    // Load nodes
    const aiNode* root = m_assimpScene->mRootNode;
    createNode(root);

    // Load meshes
    createMeshes();

    // Load animations
    createAnimation();

    m_irrMesh->setDirty();
    m_irrMesh->finalize();

    return m_irrMesh;
}

void IrrAssimpImport::createMaterials()
{
    // Basic material support
    const core::stringc fileDir = m_fileSystem->getFileDir(m_filePath);
    m_materials.clear();
    for (unsigned int i = 0; i < m_assimpScene->mNumMaterials; ++i)
    {
        video::SMaterial irrMaterial;
        irrMaterial.MaterialType = video::EMT_SOLID;

        aiMaterial* material = m_assimpScene->mMaterials[i];

        aiColor4D color;
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color)) {
            irrMaterial.DiffuseColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &color)) {
            irrMaterial.AmbientColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &color)) {
            irrMaterial.EmissiveColor = assimpToIrrColor4(color);
        }
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &color)) {
            irrMaterial.SpecularColor = assimpToIrrColor4(color);
        }
        float shininess;
        if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess)) {
            irrMaterial.Shininess = shininess;
        }


        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &path);

            video::ITexture* diffuseTexture = getTexture(assimpToIrrString(path), fileDir);
            irrMaterial.setTexture(0, diffuseTexture);
        }
        if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_NORMALS, 0, &path);

            video::ITexture* normalsTexture = getTexture(assimpToIrrString(path), fileDir);
            if (normalsTexture)
            {
                irrMaterial.setTexture(1, normalsTexture);
                irrMaterial.MaterialType = video::EMT_PARALLAX_MAP_SOLID;
            }
        }

        m_materials.push_back(irrMaterial);
    }
}

void IrrAssimpImport::createMeshes()
{
    for (unsigned int i = 0; i < m_assimpScene->mNumMeshes; ++i)
    {
        //std::cout << "i=" << i << " of " << m_assimpScene->mNumMeshes << std::endl;
        aiMesh* mesh = m_assimpScene->mMeshes[i];
        const unsigned int uvCount = mesh->GetNumUVChannels();
        const unsigned int verticesCount = mesh->mNumVertices;

        scene::SSkinMeshBuffer* irrBuffer = m_irrMesh->addMeshBuffer();
        irrBuffer->VertexType = (uvCount > 1) ? video::EVT_2TCOORDS : (mesh->HasTangentsAndBitangents() ? video::EVT_TANGENTS : video::EVT_STANDARD);

        // Resize Buffer
        switch (irrBuffer->VertexType)
        {
        case video::EVT_STANDARD:
            irrBuffer->Vertices_Standard.set_used(verticesCount);
            break;
        case video::EVT_2TCOORDS:
            irrBuffer->Vertices_2TCoords.set_used(verticesCount);
            break;
        case video::EVT_TANGENTS:
            irrBuffer->Vertices_Tangents.set_used(verticesCount);
            break;
        }


        for (unsigned int j = 0; j < verticesCount; ++j)
        {
            irrBuffer->getPosition(j) = assimpToIrrVector3(mesh->mVertices[j]);
            if (mesh->HasNormals())
            {
                irrBuffer->getNormal(j) = assimpToIrrVector3(mesh->mNormals[j]);
            }

            video::SColor irrColor = video::SColor(255, 255, 255, 255);
            if (mesh->HasVertexColors(0))
            {
                irrColor = assimpToIrrColor4(mesh->mColors[0][j]);
            }

            core::vector2df irrUv = core::vector2df(0.f, 0.f);
            if (uvCount > 0)
            {
                irrUv = assimpToIrrVector2(mesh->mTextureCoords[0][j]);
            }
            irrBuffer->getTCoords(j) = irrUv;

            switch (irrBuffer->VertexType)
            {
                case video::EVT_STANDARD:
                {
                    irrBuffer->Vertices_Standard[j].Color = irrColor;
                    break;
                }
                case video::EVT_2TCOORDS:
                {
                    irrBuffer->Vertices_2TCoords[j].Color = irrColor;
                    irrBuffer->Vertices_2TCoords[j].TCoords2 = assimpToIrrVector2(mesh->mTextureCoords[1][j]);
                    break;
                }
                case video::EVT_TANGENTS:
                {
                    irrBuffer->Vertices_Tangents[j].Color = irrColor;
                    irrBuffer->Vertices_Tangents[j].Tangent = assimpToIrrVector3(mesh->mTangents[j]);
                    irrBuffer->Vertices_Tangents[j].Binormal = assimpToIrrVector3(mesh->mBitangents[j]);
                    break;
                }
            }
        }


        irrBuffer->Indices.set_used(mesh->mNumFaces * 3);
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j)
        {
            const aiFace face = mesh->mFaces[j];

            irrBuffer->Indices[3*j] = face.mIndices[0];
            irrBuffer->Indices[3*j + 1] = face.mIndices[1];
            irrBuffer->Indices[3*j + 2] = face.mIndices[2];
        }

        irrBuffer->Material = m_materials[mesh->mMaterialIndex];
        irrBuffer->recalculateBoundingBox();

        if (!mesh->HasNormals())
            m_sceneManager->getMeshManipulator()->recalculateNormals(irrBuffer);


        // Skinning
        buildSkinnedVertexArray(irrBuffer);
        for (unsigned int j = 0; j < mesh->mNumBones; ++j)
        {
            aiBone* bone = mesh->mBones[j];

            scene::ISkinnedMesh::SJoint* irrJoint = findJoint(assimpToIrrString(bone->mName));
            if (irrJoint == 0)
            {
                //std::cout << "Error, no joint" << std::endl;
                continue;
            }

            /* The old version
            core::matrix4 boneOffset = AssimpToIrrMatrix(bone->mOffsetMatrix);
            core::matrix4 invBoneOffset;
            boneOffset.getInverse(invBoneOffset);

            core::matrix4 rotationMatrix;
            rotationMatrix.setRotationDegrees(invBoneOffset.getRotationDegrees());
            core::matrix4 translationMatrix;
            translationMatrix.setTranslation(boneOffset.getTranslation());
			core::matrix4 scaleMatrix;
            scaleMatrix.setScale(invBoneOffset.getScale());

            core::matrix4 globalBoneMatrix = scaleMatrix * rotationMatrix * translationMatrix;
            globalBoneMatrix.setInverseTranslation(globalBoneMatrix.getTranslation());

            joint->GlobalMatrix = globalBoneMatrix;
            // And compute the local from global after (joint->LocalMatrix = invGlobalParent * joint->GlobalMatrix; if it's not a root)
            */


            for (unsigned int h = 0; h < bone->mNumWeights; ++h)
            {
                const aiVertexWeight weight = bone->mWeights[h];
                scene::ISkinnedMesh::SWeight* irrWeight = m_irrMesh->addWeight(irrJoint);
                irrWeight->buffer_id = m_irrMesh->getMeshBufferCount() - 1;
                irrWeight->strength = weight.mWeight;
                irrWeight->vertex_id = weight.mVertexId;
            }

            skinJoint(irrJoint, bone);
        }
        applySkinnedVertexArray(irrBuffer);
    }
}

void IrrAssimpImport::createAnimation()
{
    double frameOffset = 0.f;
    for (unsigned int i = 0; i < m_assimpScene->mNumAnimations; ++i)
    {
        aiAnimation* anim = m_assimpScene->mAnimations[i];

        if (anim->mTicksPerSecond != 0.f)
        {
            m_irrMesh->setAnimationSpeed(anim->mTicksPerSecond);
        }

        //std::cout << "numChannels : " << anim->mNumChannels << std::endl;
        for (unsigned int j = 0; j < anim->mNumChannels; ++j)
        {
            aiNodeAnim* nodeAnim = anim->mChannels[j];
            scene::ISkinnedMesh::SJoint* joint = findJoint(assimpToIrrString(nodeAnim->mNodeName));

            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mPositionKeys[k];

                scene::ISkinnedMesh::SPositionKey* irrKey = m_irrMesh->addPositionKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->position = assimpToIrrVector3(key.mValue);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; ++k)
            {
                aiQuatKey key = nodeAnim->mRotationKeys[k];

                scene::ISkinnedMesh::SRotationKey* irrKey = m_irrMesh->addRotationKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->rotation = assimpToIrrQuaternion(key.mValue);
            }
            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; ++k)
            {
                aiVectorKey key = nodeAnim->mScalingKeys[k];

                scene::ISkinnedMesh::SScaleKey* irrKey = m_irrMesh->addScaleKey(joint);
                irrKey->frame = key.mTime + frameOffset;
                irrKey->scale = assimpToIrrVector3(key.mValue);
            }

        }

        frameOffset += anim->mDuration;
    }
}

// Adapted from http://sourceforge.net/p/assimp/discussion/817654/thread/5462cbf5
void IrrAssimpImport::skinJoint(scene::ISkinnedMesh::SJoint* joint, aiBone* bone)
{
	if (bone->mNumWeights)
	{
        core::matrix4 boneOffset = assimpToIrrMatrix(bone->mOffsetMatrix);
	    core::matrix4 boneMat = joint->GlobalMatrix * boneOffset; //* InverseRootNodeWorldTransform;

        const u32 bufferId = m_irrMesh->getMeshBufferCount() - 1;

		for (u32 i = 0; i < bone->mNumWeights; ++i)
		{
		    const aiVertexWeight weight = bone->mWeights[i];
			const u32 vertexId = weight.mVertexId;

            core::vector3df sourcePos = m_irrMesh->getMeshBuffer(bufferId)->getPosition(vertexId);
            core::vector3df sourceNorm = m_irrMesh->getMeshBuffer(bufferId)->getNormal(vertexId);
			core::vector3df destPos, destNormal;
			boneMat.transformVect(destPos, sourcePos);
			boneMat.rotateVect(destNormal, sourceNorm);

            m_skinnedVertex[vertexId].moved = true;
            m_skinnedVertex[vertexId].position += destPos * weight.mWeight;
            m_skinnedVertex[vertexId].normal += destNormal * weight.mWeight;
		}
	}
}

void IrrAssimpImport::buildSkinnedVertexArray(scene::IMeshBuffer* buffer)
{
    m_skinnedVertex.clear();

    m_skinnedVertex.reallocate(buffer->getVertexCount());
    for (u32 i = 0; i < buffer->getVertexCount(); ++i)
    {
        m_skinnedVertex.push_back(SkinnedVertex());
    }

}

void IrrAssimpImport::applySkinnedVertexArray(scene::IMeshBuffer* buffer)
{
    for (u32 i = 0; i < buffer->getVertexCount(); ++i)
    {
        if (m_skinnedVertex[i].moved)
        {
            buffer->getPosition(i) = m_skinnedVertex[i].position;
            buffer->getNormal(i) = m_skinnedVertex[i].normal;
        }
    }
    m_skinnedVertex.clear();
}
