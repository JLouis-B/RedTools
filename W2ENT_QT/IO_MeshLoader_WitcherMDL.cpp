#include "IO_MeshLoader_WitcherMDL.h"

#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IWriteFile.h"
#include "Settings.h"

#include <iostream>

#include "Utils_Loaders_Irr.h"
#include "Utils_Qt_Irr.h"


enum NodeType
{
    kNodeTypeNode         = 0x00000001,
    kNodeTypeLight        = 0x00000003,
    kNodeTypeEmitter      = 0x00000005,
    kNodeTypeCamera       = 0x00000009,
    kNodeTypeReference    = 0x00000011,
    kNodeTypeTrimesh      = 0x00000021,
    kNodeTypeSkin         = 0x00000061,
    kNodeTypeAABB         = 0x00000221,
    kNodeTypeTrigger      = 0x00000421,
    kNodeTypeSectorInfo   = 0x00001001,
    kNodeTypeWalkmesh     = 0x00002001,
    kNodeTypeDanglyNode   = 0x00004001,
    kNodeTypeTexturePaint = 0x00008001,
    kNodeTypeSpeedTree    = 0x00010001,
    kNodeTypeChain        = 0x00020001,
    kNodeTypeCloth        = 0x00040001
};

enum ControllerType
{
    ControllerPosition = 84,
    ControllerOrientation = 96,
    ControllerScale = 184
};

enum NodeTrimeshControllerType
{
    kNodeTrimeshControllerTypeSelfIllumColor = 276,
    kNodeTrimeshControllerTypeAlpha          = 292
};

// Material parser ----------------------------------------
TW1_MaterialParser::TW1_MaterialParser(io::IFileSystem* fs)
    : FileSystem(fs),
    _shader("")
{
}

bool TW1_MaterialParser::loadFile(core::stringc filename)
{
    std::cout << "load file " << filename.c_str() << std::endl;
    core::array<io::path> texFolders;
    texFolders.push_back("materials00/");

    core::stringc path = "";
    for (u32 i = 0; i < texFolders.size(); ++i)
    {
        core::stringc testedPath = core::stringc(Settings::_pack0.toStdString().c_str()) + texFolders[i] + filename + core::stringc(".mat");
        //std::cout << "get texture : " << filename.c_str() << std::endl;

        if (FileSystem->existFile(testedPath))
        {
            //std::cout << "file found" << std::endl;
            path = testedPath;
        }
    }
    if (path == "")
        return false;


    io::IReadFile* file = FileSystem->createAndOpenFile(path);
    if (!file)
        return false;

    long fileSize = file->getSize();
    char content[fileSize + 1];
    file->read(content, file->getSize());
    file->drop();
    content[fileSize] = '\0';
    core::stringc contentString(content);

    return loadFromString(contentString);
}

void TW1_MaterialParser::debugPrint()
{
    std::cout << std::endl << "Shader = " << _shader.c_str() << std::endl << std::endl;
    if (_textures.size() > 0)
    {
        std::cout << "TEXTURES" << std::endl;
        std::cout << "--------" << std::endl;
        auto it = _textures.begin();
        for (; it != _textures.end(); it++)
        {
            std::cout << it->first.c_str() << " : " << it->second.c_str() << std::endl;
        }
        std::cout << std::endl;
    }
    if (_strings.size() > 0)
    {
        std::cout << "STRINGS" << std::endl;
        std::cout << "-------" << std::endl;
        auto it = _strings.begin();
        for (; it != _strings.end(); it++)
        {
            std::cout << it->first.c_str() << " : " << it->second.c_str() << std::endl;
        }
        std::cout << std::endl;
    }
    if (_bumpmaps.size() > 0)
    {
        std::cout << "BUMPMAPS" << std::endl;
        std::cout << "--------" << std::endl;
        auto it = _bumpmaps.begin();
        for (; it != _bumpmaps.end(); it++)
        {
            std::cout << it->first.c_str() << " : " << it->second.c_str() << std::endl;
        }
        std::cout << std::endl;
    }
    if (_floats.size() > 0)
    {
        std::cout << "FLOAT" << std::endl;
        std::cout << "-----" << std::endl;
        auto it = _floats.begin();
        for (; it != _floats.end(); it++)
        {
            std::cout << it->first.c_str() << " : " << core::stringc(it->second).c_str() << std::endl;
        }
        std::cout << std::endl;
    }
    if (_vectors.size() > 0)
    {
        std::cout << "VECTORS" << std::endl;
        std::cout << "-------" << std::endl;
        auto it = _vectors.begin();
        for (; it != _vectors.end(); it++)
        {
            std::cout << it->first.c_str() << " : " << core::stringc(it->second.X).c_str()
                                           << " , " << core::stringc(it->second.Y).c_str()
                                           << " , " << core::stringc(it->second.Z).c_str() << std::endl;
        }
        std::cout << std::endl;
    }
}

bool TW1_MaterialParser::loadFromString(core::stringc content)
{
    if (content.empty())
        return true;

    core::array<core::stringc> contentData;
    content.split(contentData, " \t\r\n", 4);

    for (u32 i = 0; i < contentData.size(); ++i)
    {
        core::stringc data = contentData[i];
        //std::cout << "data = " << data.c_str() << std::endl;
        if (data == "shader")
        {
            _shader = contentData[++i];
        }
        else if (data == "texture")
        {
            core::stringc texId = contentData[++i];
            core::stringc texName = contentData[++i];
            _textures.insert(std::make_pair(texId, texName));
        }
        else if (data == "bumpmap")
        {
            core::stringc texId = contentData[++i];
            core::stringc texName = contentData[++i];
            _bumpmaps.insert(std::make_pair(texId, texName));
        }
        else if (data == "string")
        {
            core::stringc stringId = contentData[++i];
            core::stringc string = contentData[++i];
            _textures.insert(std::make_pair(stringId, string));
        }
        else if (data == "float")
        {
            core::stringc floatId = contentData[++i];
            core::stringc floatStr = contentData[++i];
            f32 floatValue = strtof(floatStr.c_str(), nullptr);
            _floats.insert(std::make_pair(floatId, floatValue));
        }
        else if (data == "vector")
        {
            core::stringc vectorId = contentData[++i];
            core::stringc xStr = contentData[++i];
            core::stringc yStr = contentData[++i];
            core::stringc zStr = contentData[++i];
            core::stringc wStr = contentData[++i];

            float x = strtof(xStr.c_str(), nullptr);
            float y = strtof(yStr.c_str(), nullptr);
            float z = strtof(zStr.c_str(), nullptr);
            float w = strtof(wStr.c_str(), nullptr);
            // TODO: change for a vector4
            core::vector3df vect(x, y, z);

            _vectors.insert(std::make_pair(vectorId, vect));
        }
        else
        {
            std::cout << "Unknown data type : " << data.c_str() << std::endl;
        }
    }
    return true;
}

video::E_MATERIAL_TYPE TW1_MaterialParser::getMaterialTypeFromShader()
{
    if (_shader == "leaves_lm_bill"
     || _shader == "dblsided_atest"
     || _shader == "leaves")
        return video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

    return video::EMT_SOLID;
}

core::stringc TW1_MaterialParser::getShader()
{
    return _shader;
}

core::stringc TW1_MaterialParser::getTexture(u32 slot)
{
    if (slot != 0 || _textures.size() == 0)
        return "";

    if (slot < _IRR_MATERIAL_MAX_TEXTURES_)
    {
        auto it = _textures.find("tex");
        if (it != _textures.end())
            return it->second;

        it = _textures.find("texture0");
        if (it != _textures.end())
            return it->second;

        it = _textures.find("diffuse_texture");
        if (it != _textures.end())
            return it->second;

        it = _textures.find("diffuse_map");
        if (it != _textures.end())
            return it->second;
    }
    return "";
}

bool TW1_MaterialParser::hasMaterial()
{
    return !_shader.empty() || _textures.size() > 0;
}

// -------------------------------------------------------

IO_MeshLoader_WitcherMDL::IO_MeshLoader_WitcherMDL(scene::ISceneManager* smgr, io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs)
{

}

bool IO_MeshLoader_WitcherMDL::isALoadableFileExtension(const io::path& filename) const
{
    return core::hasFileExtension ( filename, "mdb" );
}

ArrayDef readArrayDef(io::IReadFile* file)
{
    ArrayDef def;
    def.firstElemOffest = readU32(file);

    // nbUsedEntries is equal to nbAllocatedEntries
    def.nbUsedEntries = readU32(file);
    def.nbAllocatedEntries = readU32(file);
    return def;
}

scene::IAnimatedMesh* IO_MeshLoader_WitcherMDL::createMesh(io::IReadFile* file)
{
    if (!file)
        return nullptr;

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
    return AnimatedMesh;
}

bool IO_MeshLoader_WitcherMDL::load(io::IReadFile* file)
{
    GameTexturesPath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    
    if (readU8(file) != 0) // 0 = binary file
    {
        std::cout << "Error : not a binary file" << std::endl;
        return false;
    }
    file->seek(4);

    ModelInfos.fileVersion = readU32(file) & 0x0FFFFFFF; // should be 133 or 136
    u32 modelCount = readU32(file); // should be 1 ?
    if (modelCount != 1)
    {
        std::cout << "Error : modelCount != 1 isn't handled" << std::endl;
        return false;
    }

    file->seek(4, true);
    ModelInfos.sizeModelData = readU32(file);
    file->seek(4, true);
    ModelInfos.offsetModelData = 32;

    if (ModelInfos.fileVersion == 133)
    {
        ModelInfos.offsetRawData  = readU32(file) + ModelInfos.offsetModelData;
        ModelInfos.sizeRawData = readU32(file);
        ModelInfos.offsetTexData  = ModelInfos.offsetModelData;
        ModelInfos.sizeTexData = 0;
    }
    else
    {
        ModelInfos.offsetRawData  = ModelInfos.offsetModelData;
        ModelInfos.sizeRawData = 0;
        ModelInfos.offsetTexData  = readU32(file) + ModelInfos.offsetModelData;
        ModelInfos.sizeTexData = readU32(file);
    }

    file->seek(8, true);

    core::stringc name = readStringFixedSize(file, 64);
    std::cout << "model name = " << name.c_str() << std::endl;
    u32 offsetRootNode = readU32(file);

    file->seek(32, true);
    u8 type = readU8(file);

    file->seek(3, true);
    file->seek(48, true);

    f32 firstLOD = readF32(file);
    f32 lastLOD  = readF32(file);

    file->seek(16, true);

    core::stringc detailMap = readStringFixedSize(file, 64);
    std::cout << "detail map = " << detailMap.c_str() << std::endl;

    file->seek(4, true);

    f32 modelScale = readF32(file);

    core::stringc superModel = readStringFixedSize(file, 64);
    std::cout << "superModel = " << superModel.c_str() << std::endl;

    file->seek(16, true);

    file->seek(ModelInfos.offsetModelData + offsetRootNode);
    loadNode(file, nullptr, core::matrix4());

    return true;
}

bool IO_MeshLoader_WitcherMDL::hasTexture(core::stringc texPath)
{
    core::array<io::path> texFolders;
    texFolders.push_back("textures00/");
    texFolders.push_back("textures01/");
    texFolders.push_back("meshes00/");
    texFolders.push_back("items00/");

    core::array<io::path> possibleExtensions;
    possibleExtensions.push_back(".dds");
    possibleExtensions.push_back(".bmp");
    possibleExtensions.push_back(".jpg");
    possibleExtensions.push_back(".jpeg");
    possibleExtensions.push_back(".tga");
    possibleExtensions.push_back(".png");

    for (u32 i = 0; i < possibleExtensions.size(); ++i)
    {
        for (u32 j = 0; j < texFolders.size(); ++j)
        {
            core::stringc filename = GameTexturesPath + texFolders[j] + texPath + possibleExtensions[i];
            //std::cout << "get texture : " << filename.c_str() << std::endl;

            if (FileSystem->existFile(filename))
            {
                //std::cout << "file found" << std::endl;
                return true;
            }
         }
    }

    return false;
}

video::ITexture* IO_MeshLoader_WitcherMDL::getTexture(core::stringc texPath)
{
    video::ITexture* texture = nullptr;

    core::array<io::path> texFolders;
    texFolders.push_back("textures00/");
    texFolders.push_back("textures01/");
    texFolders.push_back("meshes00/");
    texFolders.push_back("items00/");

    core::array<io::path> possibleExtensions;
    possibleExtensions.push_back(".dds");
    possibleExtensions.push_back(".bmp");
    possibleExtensions.push_back(".jpg");
    possibleExtensions.push_back(".jpeg");
    possibleExtensions.push_back(".tga");
    possibleExtensions.push_back(".png");

    for (u32 i = 0; i < possibleExtensions.size(); ++i)
    {
        for (u32 j = 0; j < texFolders.size(); ++j)
        {
            core::stringc filename = GameTexturesPath + texFolders[j] + texPath + possibleExtensions[i];
            //std::cout << "get texture : " << filename.c_str() << std::endl;

            if (FileSystem->existFile(filename))
            {
                //std::cout << "file found" << std::endl;
                texture = SceneManager->getVideoDriver()->getTexture(filename);
            }

            if (texture)
            {
                std::cout << "return valid tex" << std::endl;
                return texture;
            }
        }
    }

    std::cout << "return nullptr" << std::endl;
    return texture;
    
}

ControllersData IO_MeshLoader_WitcherMDL::readNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data)
{
    ControllersData controllers;

    core::matrix4 pos, rot, scale;
    core::array<f32> controllerData = readArray<f32>(file, data);

    const long back = file->getPos();
    file->seek(ModelInfos.offsetModelData + key.firstElemOffest);

    for (u32 i = 0; i < key.nbUsedEntries; ++i)
    {
        s32 controllerType = readS32(file);
        s16 nbRows = readS16(file);
        s16 firstKeyIndex = readS16(file);
        s16 firstValueIndex = readS16(file);
        s8 nbColumns = readS8(file);
        file->seek(1, true); // Pad, not used


        if (controllerType == ControllerPosition)
        {
            f32 x = controllerData[firstValueIndex];
            f32 y = controllerData[firstValueIndex + 1];
            f32 z = controllerData[firstValueIndex + 2];
            controllers.position = core::vector3df(x, y, z);
            pos.setTranslation(controllers.position);
        }
        else if (controllerType == ControllerOrientation)
        {
            f32 x = controllerData[firstValueIndex];
            f32 y = controllerData[firstValueIndex + 1];
            f32 z = controllerData[firstValueIndex + 2];
            f32 w = controllerData[firstValueIndex + 3];
            controllers.rotation = core::quaternion(x, y, z, w);
            core::vector3df euler;
            controllers.rotation.toEuler(euler);
            rot.setRotationRadians(euler);
        }
        else if (controllerType == ControllerScale)
        {
            f32 scaleValue = controllerData[firstValueIndex];
            controllers.scale = core::vector3df(scaleValue, scaleValue, scaleValue);
            scale.setScale(controllers.scale);
        }
        else if (controllerType == kNodeTrimeshControllerTypeAlpha)
        {
            f32 alpha = controllerData[firstValueIndex];
            if (alpha != 1.f)
                std::cout << "alpha != 1" << std::endl;
            controllers.alpha = alpha;
        }
        else if (controllerType == kNodeTrimeshControllerTypeSelfIllumColor)
        {
            video::SColor color;
            color.setRed    (controllerData[firstValueIndex]);
            color.setGreen  (controllerData[firstValueIndex + 1]);
            color.setBlue   (controllerData[firstValueIndex + 2]);
            controllers.selphIllumColor = color;
        }
        else
        {
            std::cout << "Unknown controller type=" << controllerType << std::endl;
        }
    }
    controllers.localTransform = pos * rot * scale;

    file->seek(back);
    return controllers;
}

void IO_MeshLoader_WitcherMDL::readTexturePaint(io::IReadFile* file, ControllersData controllers)
{
    ArrayDef layersDef = readArrayDef(file);

    file->seek(28, true);

    u32 offMeshArrays = readU32(file);

    // sector ID
    file->seek(16, true);

    // bbox
    file->seek(24, true);

    // diffuse & ambient colors
    file->seek(24, true);

    // many useless material options
    file->seek(53, true);

    // dayNightTransition ?
    file->seek(200, true);

    file->seek(28, true);

    // lightmap name
    file->seek(64, true);

    file->seek(3, true);

    u32 endPos = file->seek(ModelInfos.offsetRawData + offMeshArrays);
    file->seek(4, true);


    ArrayDef vertexDef = readArrayDef(file);
    ArrayDef normalsDef = readArrayDef(file);

    ArrayDef tangentsDef = readArrayDef(file);
    ArrayDef binormalsDef = readArrayDef(file);

    ArrayDef tVerts[4];
    for (u32 t = 0; t < 4; t++)
        tVerts[t] = readArrayDef(file);


    ArrayDef unknownDef = readArrayDef(file);
    ArrayDef facesDef = readArrayDef(file);
    u32 facesCount = facesDef.nbUsedEntries;

    if ((vertexDef.nbUsedEntries == 0) || (facesDef.nbUsedEntries == 0))
    {
        file->seek(endPos);
        return;
    }


    // TODO
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    for (u32 i = 0; i < layersDef.nbUsedEntries; ++i)
    {
        file->seek(ModelInfos.offsetRawData + layersDef.firstElemOffest + i * 52);
        bool hasTexture = (readU8(file) == 1);
        if (!hasTexture)
            continue;

        file->seek(7, true);
        core::stringc texture = readStringFixedSize(file, 32);

        ArrayDef weightsDef = readArrayDef(file);
        core::array<f32> weights = readArray<f32>(file, weightsDef);
        // All the texture layer have the same number of weights
        // One weight per vertex
        std::cout << "layer " << i << " has a texture : " << texture.c_str() << " with " << weights.size() << "weights" << std::endl;

        // Material
        // more complicated than that : need to make a splatting shader
        video::SMaterial mat;
        if (texture != "" && i == 0)
        {
            std::cout << "try to set texture : " << texture.c_str() << std::endl;
            video::ITexture* tex = getTexture(texture);
            if (tex)
                mat.setTexture(0, tex);
        }
        buffer->Material = mat;
    }

    // Read vertices
    buffer->VertexType = video::EVT_STANDARD;

    //std::cout << "nb vertex = " << vertexDef.nbUsedEntries << std::endl;
    buffer->Vertices_Standard.reallocate(vertexDef.nbUsedEntries);
    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);


        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);

        buffer->Vertices_Standard[i].Pos = pos;

        // to see what's texture paint
        buffer->Vertices_Standard[i].Color = video::SColor(255.f, 255.f, 255.f, 255.f);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);
    }

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
    }


    // UV
    for (u32 t = 0; t < 4; t++)
    {
        // TODO : TCoords2 -> FVF
        if (t != 0)
            continue;

        file->seek(ModelInfos.offsetRawData + tVerts[t].firstElemOffest);
        for (u32 i = 0; i < tVerts[t].nbUsedEntries; i++)
        {
            float u = 0.f, v = 0.f;
            if (i < tVerts[t].nbUsedEntries)
            {
                u = readF32(file);
                v = readF32(file);
            }
            buffer->Vertices_Standard[i].TCoords = core::vector2df(u, v);
        }
    }

    // TODO : Binormals + tangents


    // Faces
    buffer->Indices.reallocate(facesCount * 3);
    buffer->Indices.set_used(facesCount * 3);

    file->seek(ModelInfos.offsetRawData + facesDef.firstElemOffest);
    for (u32 i = 0; i < facesCount; ++i)
    {
        buffer->Indices[i * 3 + 0] = readU32(file);
        buffer->Indices[i * 3 + 1] = readU32(file);
        buffer->Indices[i * 3 + 2] = readU32(file);

        file->seek(68, true);
    }

    file->seek(endPos);
}

void IO_MeshLoader_WitcherMDL::readMesh(io::IReadFile* file, ControllersData controllers)
{
    file->seek(8, true); // Function pointer

    u32 offMeshArrays = readU32(file);

    file->seek(4, true); // Unknown
    file->seek(24, true); // bbox
    file->seek(28, true); // Unknown
    file->seek(4, true); // fog scale
    file->seek(16, true); // Unknown

    file->seek(36, true); // diffuse, ambient and specular color

    file->seek(16, true); // render settings

    // don't know what is it
    u32 transparencyHint = readU32(file) == 1;
    if (transparencyHint != 0)
        std::cout << "transparencyHint != 0" << std::endl;

    file->seek(4, true); // Unknown

    core::stringc textureStrings[4];
    for (u32 i = 0; i < 4; ++i)
    {
        textureStrings[i] = readStringFixedSize(file, 64);

        if (textureStrings[i] == "NULL")
            textureStrings[i] = "";

        std::cout << "Mesh texture " << i << " : " << textureStrings[i].c_str() << std::endl;
    }

    file->seek(7, true); // render settings

    // Unknown
    file->seek(1, true);

    // don't know what is it
    f32 transparencyShift = readF32(file);
    if (transparencyShift != 0)
        std::cout << "transparencyShift=" << transparencyShift << std::endl;

    file->seek(12, true); // render settings
    file->seek(4, true); // Unknown
    file->seek(13, true); // render settings
    file->seek(3, true); // Unknown
    file->seek(5, true);
    file->seek(3, true); // Unknown
    file->seek(9, true);

    core::stringc dayNightTransition = readStringFixedSize(file, 200);

    file->seek(2, true);

    // Unknown
    file->seek(1, true);
    file->seek(12, true);
    file->seek(8, true);

    core::stringc lightMapName = readStringFixedSize(file, 64);

    // Unknown
    file->seek(4, true);
    file->seek(4, true);

    ModelInfos.offsetTextureInfo = readU32(file);

    u32 endPos = file->seek(ModelInfos.offsetRawData + offMeshArrays);

    file->seek(4, true);

    ArrayDef vertexDef = readArrayDef(file);
    ArrayDef normalsDef = readArrayDef(file);

    ArrayDef tangentsDef = readArrayDef(file);
    ArrayDef binormalsDef = readArrayDef(file);

    ArrayDef tVerts[4];
    for (u32 t = 0; t < 4; t++)
        tVerts[t] = readArrayDef(file);


    ArrayDef unknownDef = readArrayDef(file);
    ArrayDef facesDef = readArrayDef(file);
    u32 facesCount = facesDef.nbUsedEntries;


    if (ModelInfos.fileVersion == 133)
        ModelInfos.offsetTexData = readU32(file);


    if ((vertexDef.nbUsedEntries == 0) || (facesDef.nbUsedEntries == 0))
    {
        file->seek(endPos);
        return;
    }

    TW1_MaterialParser materialParser = readTextures(file);
    
    // Vertices
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;

    buffer->Vertices_Standard.reallocate(vertexDef.nbUsedEntries);
    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);

        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);

        buffer->Vertices_Standard[i].Pos = pos;
        buffer->Vertices_Standard[i].Color = video::SColor(255.f, 255.f, 255.f, 255.f);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);
    }

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
    }

    // TODO : Binormals + tangents
    // need FVF


    // Faces
    buffer->Indices.reallocate(facesCount * 3);
    buffer->Indices.set_used(facesCount * 3);

    file->seek(ModelInfos.offsetRawData + facesDef.firstElemOffest);
    for (u32 i = 0; i < facesCount; ++i)
    {
        file->seek(4 * 4 + 4, true);

        if (ModelInfos.fileVersion == 133)
            file->seek(3 * 4, true);

        buffer->Indices[i * 3 + 0] = readU32(file);
        buffer->Indices[i * 3 + 1] = readU32(file);
        buffer->Indices[i * 3 + 2] = readU32(file);

        if (ModelInfos.fileVersion == 133)
            file->seek(4, true);
    }

    // Material
    video::SMaterial bufferMaterial;
    bufferMaterial.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
    bufferMaterial.MaterialTypeParam = 0.5f;

    core::stringc textureDiffuse;
    s32 uvSet = -1;
    if (textureStrings[0] == "_shader_")
    {
        // textureStrings[1] refers to a mat file
        core::stringc materialFile = textureStrings[1];
        std::cout << "materialFile : " << materialFile.c_str() << std::endl;

        TW1_MaterialParser parser(FileSystem);
        parser.loadFile(materialFile);
        textureDiffuse = parser.getTexture(0);
        bufferMaterial.MaterialType = parser.getMaterialTypeFromShader();
        parser.debugPrint();

        uvSet = 1;
    }
    else if (hasTexture(textureStrings[0]))
    {
        textureDiffuse = textureStrings[0];
        uvSet = 0;
    }
    else if (hasTexture(textureStrings[1]))
    {
        textureDiffuse = textureStrings[1];
        uvSet = 1;
    }
    else if (materialParser.hasMaterial())
    {
        textureDiffuse = materialParser.getTexture(0);
        uvSet = 1; // 0 in some case, 1 in some other cases : TODO find the rule to get the good uvSet
        bufferMaterial.MaterialType = materialParser.getMaterialTypeFromShader();
        //std::cout << "try to set texture : " << textures[0].c_str() << std::endl;
        materialParser.debugPrint();
    }


    video::ITexture* texture = getTexture(textureDiffuse);
    if (texture)
        bufferMaterial.setTexture(0, texture);

    if (bufferMaterial.getTexture(0) == nullptr)
    {
        std::cout << "No texture !" << std::endl;
    }
    buffer->Material = bufferMaterial;


    // UV
    if (uvSet != -1)
    {
        file->seek(ModelInfos.offsetRawData + tVerts[uvSet].firstElemOffest);
        for (u32 i = 0; i < tVerts[uvSet].nbUsedEntries; i++)
        {
            f32 u = readF32(file);
            f32 v = readF32(file);
            buffer->Vertices_Standard[i].TCoords = core::vector2df(u, v);
        }
    }

    file->seek(endPos);
}

void IO_MeshLoader_WitcherMDL::readSkin(io::IReadFile* file, ControllersData controllers)
{
    std::cout << "Skin POS = " << file->getPos() << std::endl;
    file->seek(72, true);
    file->seek(16, true); // 0xFFFFFFFFFFFFFFFFFFF...

    file->seek(60, true);
    core::stringc textureStrings[4];
    for (u32 i = 0; i < 4; ++i)
    {
        textureStrings[i] = readStringFixedSize(file, 64);

        if (textureStrings[i] == "NULL")
            textureStrings[i] = "";

        std::cout << "Mesh texture " << i << " : " << textureStrings[i].c_str() << std::endl;
    }
    file->seek(61, true);

    core::stringc dayNightTransition = readStringFixedSize(file, 200);

    file->seek(2, true);

    // Unknown
    file->seek(1, true);
    file->seek(12, true);
    file->seek(8, true);

    core::stringc lightMapName = readStringFixedSize(file, 64);

    file->seek(8, true);
    ModelInfos.offsetTextureInfo = readU32(file);
    file->seek(20, true);

    ArrayDef vertexDef = readArrayDef(file);
    ArrayDef normalsDef = readArrayDef(file);

    ArrayDef tangentsDef = readArrayDef(file);
    ArrayDef binormalsDef = readArrayDef(file);

    ArrayDef tVerts[4];
    for (u32 t = 0; t < 4; t++)
        tVerts[t] = readArrayDef(file);


    ArrayDef unknownDef = readArrayDef(file);
    ArrayDef facesDef = readArrayDef(file);
    u32 facesCount = facesDef.nbUsedEntries;

    if (ModelInfos.fileVersion == 133)
        ModelInfos.offsetTexData = readU32(file);


    if ((vertexDef.nbUsedEntries == 0) || (facesDef.nbUsedEntries == 0))
    {
        //file->seek(endPos);
        return;
    }

    file->seek(24, true);
    file->seek(12, true);
    ArrayDef weightingDef = readArrayDef(file);
    core::array<f32> weights = readArray<f32>(file, weightingDef);

    ArrayDef bonesDef = readArrayDef(file);
    core::array<u8> bones = readArray<u8>(file, bonesDef);


    TW1_MaterialParser materialParser = readTextures(file);

    // Vertices
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;

    buffer->Vertices_Standard.reallocate(vertexDef.nbUsedEntries);
    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    u32 skinningIndex = 0;
    u32 minBone = 1000, maxBone = 0;
    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);

        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);

        buffer->Vertices_Standard[i].Pos = pos;
        buffer->Vertices_Standard[i].Color = video::SColor(255.f, 255.f, 255.f, 255.f);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);

        // Skinning of the vertex
        for (int j = 0; j < 4; ++j)
        {
            u32 currentSkinningIndex = skinningIndex++;
            s8 boneId = bones[currentSkinningIndex];
            if (boneId == -1)
                continue;
            if (boneId >= AnimatedMesh->getJointCount())
            {
                std::cout << "boneId " << (s32)boneId << " >= boneCount " << AnimatedMesh->getJointCount() << std::endl;
            }

            f32 weight = weights[currentSkinningIndex];
            scene::ISkinnedMesh::SWeight* w = AnimatedMesh->addWeight(AnimatedMesh->getAllJoints()[boneId]);//[boneId]);
            w->buffer_id = AnimatedMesh->getMeshBufferCount() - 1;
            w->vertex_id = i;
            w->strength = weight;

            if (boneId > maxBone)
                maxBone = boneId;
            if (boneId < minBone)
                minBone = boneId;
        }
    }
    std::cout << "minBone=" << minBone << ", maxBone=" << maxBone << std::endl;

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
    }

    // TODO : Binormals + tangents
    // need FVF


    // Faces
    buffer->Indices.reallocate(facesCount * 3);
    buffer->Indices.set_used(facesCount * 3);

    file->seek(ModelInfos.offsetRawData + facesDef.firstElemOffest);
    for (u32 i = 0; i < facesCount; ++i)
    {
        file->seek(4 * 4 + 4, true);

        if (ModelInfos.fileVersion == 133)
            file->seek(3 * 4, true);

        buffer->Indices[i * 3 + 0] = readU32(file);
        buffer->Indices[i * 3 + 1] = readU32(file);
        buffer->Indices[i * 3 + 2] = readU32(file);

        if (ModelInfos.fileVersion == 133)
            file->seek(4, true);
    }

    // Material
    video::SMaterial bufferMaterial;
    bufferMaterial.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
    bufferMaterial.MaterialTypeParam = 0.5f;

    core::stringc textureDiffuse;
    s32 uvSet = -1;
    if (textureStrings[0] == "_shader_")
    {
        // textureStrings[1] refers to a mat file
        core::stringc materialFile = textureStrings[1];
        std::cout << "materialFile : " << materialFile.c_str() << std::endl;

        TW1_MaterialParser parser(FileSystem);
        parser.loadFile(materialFile);
        textureDiffuse = parser.getTexture(0);
        bufferMaterial.MaterialType = parser.getMaterialTypeFromShader();
        parser.debugPrint();

        uvSet = 1;
    }
    else if (hasTexture(textureStrings[0]))
    {
        textureDiffuse = textureStrings[0];
        uvSet = 0;
    }
    else if (hasTexture(textureStrings[1]))
    {
        textureDiffuse = textureStrings[1];
        uvSet = 1;
    }
    else if (materialParser.hasMaterial())
    {
        textureDiffuse = materialParser.getTexture(0);
        uvSet = 0; // 0 in some case, 1 in some other cases : TODO find the rule to get the good uvSet
        bufferMaterial.MaterialType = materialParser.getMaterialTypeFromShader();
        std::cout << "SKIN : try to set texture : " << textureDiffuse.c_str() << std::endl;
        materialParser.debugPrint();
    }


    video::ITexture* texture = getTexture(textureDiffuse);
    if (texture)
        bufferMaterial.setTexture(0, texture);

    if (bufferMaterial.getTexture(0) == nullptr)
    {
        std::cout << "No texture !" << std::endl;
    }
    buffer->Material = bufferMaterial;


    // UV
    if (uvSet != -1)
    {
        file->seek(ModelInfos.offsetRawData + tVerts[uvSet].firstElemOffest);
        for (u32 i = 0; i < tVerts[uvSet].nbUsedEntries; i++)
        {
            f32 u = readF32(file);
            f32 v = readF32(file);
            buffer->Vertices_Standard[i].TCoords = core::vector2df(u, v);
        }
    }

    //file->seek(endPos);
}

void IO_MeshLoader_WitcherMDL::readSpeedtree(io::IReadFile* file, ControllersData controllers)
{
    //std::cout << "SpeedTree POS = " << file->getPos() << std::endl;

    // Refer to a .spt file to describe the tree
    // seems complicated to use now
    core::stringc filename = readStringFixedSize(file, 32);
    //std::cout << "filename: " << filename.c_str() << std::endl;
}


template <class T>
core::array<T> IO_MeshLoader_WitcherMDL::readArray(io::IReadFile* file, ArrayDef def)
{
    const long back = file->getPos();
    file->seek(ModelInfos.offsetModelData + def.firstElemOffest);

    core::array<T> array;
    for (u32 i = 0; i < def.nbUsedEntries; ++i)
    {
        array.push_back(readData<T>(file));
    }

    file->seek(back);
    return array;
}

void IO_MeshLoader_WitcherMDL::loadNode(io::IReadFile* file, scene::ISkinnedMesh::SJoint* parentJoint, core::matrix4 parentMatrix)
{
    file->seek(24, true); // Function pointers
    //file->seek(8, true); // inherit color flag + id
    file->seek(4, true); // inherit color flag
    u32 id = readU32(file);



    core::stringc name = readStringFixedSize(file, 64);
    std::cout << "Node name=" << name.c_str() << std::endl;

    file->seek(8, true); // parent geometry + parent node

    // Children nodes
    ArrayDef def = readArrayDef(file);
    //std::cout << "def count = " << def.nbUsedEntries << ", allocated = " << def.nbAllocatedEntries << ", offset = " << def.firstElemOffest << std::endl;
    core::array<u32> children = readArray<u32>(file, def);
    //std::cout << "child count = " << children.size() << std::endl;


    // Controllers
    ArrayDef controllerKeyDef = readArrayDef(file);
    ArrayDef controllerDataDef = readArrayDef(file);

    ControllersData controllers = readNodeControllers(file, controllerKeyDef, controllerDataDef);


    controllers.globalTransform = parentMatrix * controllers.localTransform;
    //


    file->seek(4, true); // node flags/type

    file->seek(8, true); // fixed rot + imposter group ?

    s32 minLOD = readS32(file);
    s32 maxLOD = readS32(file);

    NodeType type = (NodeType) readU32(file);
    //std::cout << "type = " << type << std::endl;


    switch (type)
    {
        case kNodeTypeTrimesh:
            std::cout << "Trimesh node" << std::endl;
            readMesh(file, controllers);
            break;

        case kNodeTypeTexturePaint:
            std::cout << "Texture paint node" << std::endl;
            readTexturePaint(file, controllers);
            break;

        case kNodeTypeSkin:
            std::cout << "Skin node" << std::endl;
            readSkin(file, controllers);
            break;

        case kNodeTypeSpeedTree:
            std::cout << "Speed tree" << std::endl;
            readSpeedtree(file, controllers);
        break;

        default:
            break;
    }

    scene::ISkinnedMesh::SJoint* joint = AnimatedMesh->addJoint(parentJoint);
    joint->LocalMatrix = controllers.localTransform;
    joint->GlobalMatrix= controllers.globalTransform;
    joint->Name = name.c_str();
    joint->Animatedposition = controllers.position;
    joint->Animatedrotation = controllers.rotation.makeInverse();
    joint->Animatedscale = controllers.scale;
    //BonesId.insert(std::make_pair(id & 0x00FF, joint));
    std::cout << "ID = " << (int)(id & 0x00FF) << ", @=" << file->getPos() << std::endl;
    std::cout << "Node END" << std::endl;


    // Load the children
    for (u32 i = 0; i < children.size(); ++i)
    {
        file->seek(ModelInfos.offsetModelData + children[i]);
        loadNode(file, joint, controllers.globalTransform);
    }
}

// similar to the mat parser ?
TW1_MaterialParser IO_MeshLoader_WitcherMDL::readTextures(io::IReadFile* file)
{
    u32 offset;
    if (ModelInfos.fileVersion == 133)
        offset = ModelInfos.offsetRawData + ModelInfos.offsetTexData;
    else
        offset = ModelInfos.offsetTexData + ModelInfos.offsetTextureInfo;

    file->seek(offset);

    u32 textureCount = readU32(file);
    u32 offTexture = readU32(file);
    if (offTexture != 0)
    {
        std::cout << "offTexture != 0" << std::endl;
    }

    core::stringc materialContent;
    for (u32 i = 0; i < textureCount; ++i)
    {
        core::stringc line = readStringUntilNull(file);

        materialContent += line;
        materialContent += "\n";
    }
    std::cout << "mat content : " << materialContent.c_str() << std::endl;

    TW1_MaterialParser parser(FileSystem);
    parser.loadFromString(materialContent);
    return parser;
}
