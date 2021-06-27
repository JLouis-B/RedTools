#include "IO_MeshLoader_WitcherMDL.h"

#include <ISceneManager.h>
#include <IVideoDriver.h>

#include <iostream>

#include "Settings.h"
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
    _log = LoggerManager::Instance();
}

bool TW1_MaterialParser::loadFile(core::stringc filename)
{
    _log->addLineAndFlush(formatString("load file %s", filename.c_str()));
    core::array<io::path> texFolders;
    texFolders.push_back("materials00/");

    core::stringc path = "";
    for (u32 i = 0; i < texFolders.size(); ++i)
    {
        core::stringc testedPath = core::stringc(Settings::_baseDir.toStdString().c_str()) + texFolders[i] + filename + core::stringc(".mat");
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
    if (_shader == "dblsided_atest"
     || _shader == "leaves"
     || _shader == "leaves_lm"
     || _shader == "leaves_lm_bill"
     || _shader == "leaves_singles")
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
: meshToAnimate(nullptr), SceneManager(smgr), FileSystem(fs)
{
    NodeTypeNames.insert(std::make_pair(kNodeTypeNode, "kNodeTypeNode"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeLight, "kNodeTypeLight"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeEmitter, "kNodeTypeEmitter"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeCamera, "kNodeTypeCamera"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeReference, "kNodeTypeReference"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeTrimesh, "kNodeTypeTrimesh"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeSkin, "kNodeTypeSkin"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeAABB, "kNodeTypeAABB"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeTrigger, "kNodeTypeTrigger"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeSectorInfo, "kNodeTypeSectorInfo"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeWalkmesh, "kNodeTypeWalkmesh"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeDanglyNode, "kNodeTypeDanglyNode"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeTexturePaint, "kNodeTypeTexturePaint"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeSpeedTree, "kNodeTypeSpeedTree"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeChain, "kNodeTypeChain"));
    NodeTypeNames.insert(std::make_pair(kNodeTypeCloth, "kNodeTypeCloth"));
}

bool IO_MeshLoader_WitcherMDL::isALoadableFileExtension(const io::path& filename) const
{
    // MBA (model binary animation) seem to be basically the same file format
    return core::hasFileExtension ( filename, "mdb" ) || core::hasFileExtension ( filename, "mba" );
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

    _depth = 0;

    _log = LoggerManager::Instance();
    _log->addLine("");
    _log->addLine(formatString("-> File : %s", file->getFileName().c_str()));
    _log->add("_________________________________________________________\n\n\n");
    _log->addLineAndFlush("Start loading");

    // if we want to add animations to an existing mesh
    if (meshToAnimate)
        AnimatedMesh = meshToAnimate;
    else
        AnimatedMesh = SceneManager->createSkinnedMesh();

    if (load(file))
    {
        // because we need to have loaded all the node before the skinning
        for (u32 i = 0; i < SkinMeshToLoad.size(); ++i)
        {
            long back = file->getPos();
            file->seek(SkinMeshToLoad[i].Seek);
            readSkinNode(file, SkinMeshToLoad[i].ControllersData);
#ifdef TW1_ATTACH_MESHES_TO_NODES
            SkinMeshToLoad[i].Joint->AttachedMeshes.push_back(AnimatedMesh->getMeshBufferCount() - 1);
#endif
            file->seek(back);
        }

        AnimatedMesh->finalize();
    }
    else
    {
        AnimatedMesh->drop();
        AnimatedMesh = nullptr;
    }
    _log->addLineAndFlush("Loading finished");

    SkinMeshToLoad.clear();

    return AnimatedMesh;
}

bool IO_MeshLoader_WitcherMDL::load(io::IReadFile* file)
{
    GameTexturesPath = SceneManager->getParameters()->getAttributeAsStringW("TW_GAME_PATH");
    
    if (readU8(file) != 0) // 0 = binary file
    {
        _log->addLineAndFlush("Error : not a binary file");
        return false;
    }
    file->seek(4);

    ModelInfos.fileVersion = readU32(file) & 0x0FFFFFFF; // should be 133 or 136
    u32 modelCount = readU32(file); // should be 1 ?
    if (modelCount != 1)
    {
        _log->addLineAndFlush("Error : modelCount != 1 isn't handled");
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
    _log->addLineAndFlush(formatString("model name = %s", name.c_str()));
    u32 offsetRootNode = readU32(file);

    file->seek(32, true);
    u8 type = readU8(file);

    file->seek(3, true);
    file->seek(48, true);

    f32 firstLOD = readF32(file);
    f32 lastLOD  = readF32(file);

    file->seek(16, true);

    core::stringc detailMap = readStringFixedSize(file, 64);
    _log->addLineAndFlush(formatString("detail map = %s", detailMap.c_str()));

    file->seek(4, true);

    f32 modelScale = readF32(file);

    core::stringc superModel = readStringFixedSize(file, 60);
    _log->addLineAndFlush(formatString("superModel = %s", superModel.c_str()));
    file->seek(4, true); // animaion scale, float

    file->seek(16, true);

    file->seek(ModelInfos.offsetModelData + offsetRootNode);
    loadNode(file, nullptr, core::matrix4());

    readAnimations(file);

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
                _log->addLineAndFlush("return valid tex");
                return texture;
            }
        }
    }

    _log->addLineAndFlush("return nullptr");
    return texture; 
}

StaticControllersData IO_MeshLoader_WitcherMDL::getStaticNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data)
{
    StaticControllersData controllersData;

    ControllersData controllers = readNodeControllers(file, key, data);
    if (controllers.position.size() > 0)
        controllersData.position = controllers.position[0];

    if (controllers.rotation.size() > 0)
        controllersData.rotation = controllers.rotation[0];

    if (controllers.scale.size() > 0)
        controllersData.scale = controllers.scale[0];

    controllersData.computeLocalTransform();


    if (controllers.alpha.size() > 0)
        controllersData.alpha = controllers.alpha[0];

    if (controllers.selphIllumColor.size() > 0)
        controllersData.selphIllumColor = controllers.selphIllumColor[0];

    return controllersData;
}

void IO_MeshLoader_WitcherMDL::readTexturePaintNode(io::IReadFile* file, StaticControllersData controllers)
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

    file->seek(ModelInfos.offsetRawData + offMeshArrays);
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
        _log->addLineAndFlush(formatString("layer %d has a texture : %s with %d weights", i, texture.c_str(), weights.size()));

        // Material
        // more complicated than that : need to make a splatting shader
        video::SMaterial mat;
        if (texture != "" && i == 0)
        {
            _log->addLineAndFlush(formatString("try to set texture : %s", texture.c_str()));
            video::ITexture* tex = getTexture(texture);
            if (tex)
                mat.setTexture(0, tex);
        }
        buffer->Material = mat;
    }

    // Read vertices
    buffer->VertexType = video::EVT_STANDARD;

    //std::cout << "nb vertex = " << vertexDef.nbUsedEntries << std::endl;
    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);

#ifdef TW1_ATTACH_MESHES_TO_NODES
        core::vector3df pos(x, y, z);
#else
        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);
#endif
        buffer->Vertices_Standard[i].Pos = pos;

        // to see what's texture paint
        buffer->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);
    }

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readS16(file) / 8192.f;
        f32 y = readS16(file) / 8192.f;
        f32 z = readS16(file) / 8192.f;
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
            u = readF32(file);
            v = readF32(file);

            buffer->Vertices_Standard[i].TCoords = core::vector2df(u, v);
        }
    }

    // TODO : Binormals + tangents


    // Faces
    buffer->Indices.set_used(facesCount * 3);

    file->seek(ModelInfos.offsetRawData + facesDef.firstElemOffest);
    for (u32 i = 0; i < facesCount; ++i)
    {
        buffer->Indices[i * 3 + 0] = readU32(file);
        buffer->Indices[i * 3 + 1] = readU32(file);
        buffer->Indices[i * 3 + 2] = readU32(file);

        file->seek(68, true);
    }
}

void IO_MeshLoader_WitcherMDL::readMeshNode(io::IReadFile* file, StaticControllersData controllers)
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
        _log->addLineAndFlush("transparencyHint != 0");

    file->seek(4, true); // Unknown

    core::stringc textureStrings[4];
    for (u32 i = 0; i < 4; ++i)
    {
        textureStrings[i] = readStringFixedSize(file, 64);

        if (textureStrings[i] == "NULL")
            textureStrings[i] = "";

        _log->addLineAndFlush(formatString("Mesh texture %d : %s", i, textureStrings[i].c_str()));
    }

    file->seek(7, true); // render settings

    // Unknown
    file->seek(1, true);

    // don't know what is it
    f32 transparencyShift = readF32(file);
    if (transparencyShift != 0)
        _log->addLineAndFlush(formatString("transparencyShift=", transparencyShift));

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

    file->seek(ModelInfos.offsetRawData + offMeshArrays);

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
        return;
    }

    TW1_MaterialParser materialParser = readTextures(file);
    
    // Vertices
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    buffer->VertexType = video::EVT_STANDARD;

    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);

#ifdef TW1_ATTACH_MESHES_TO_NODES
        core::vector3df pos(x, y, z);
#else
        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);
#endif

        buffer->Vertices_Standard[i].Pos = pos;
        buffer->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);
    }

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readS16(file) / 8192.f;
        f32 y = readS16(file) / 8192.f;
        f32 z = readS16(file) / 8192.f;
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
    }

    // TODO : Binormals + tangents
    // need FVF


    // Faces
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
        _log->addLineAndFlush(formatString("materialFile : %s", materialFile.c_str()));

        TW1_MaterialParser parser(FileSystem);
        parser.loadFile(materialFile);
        textureDiffuse = parser.getTexture(0);
        bufferMaterial.MaterialType = parser.getMaterialTypeFromShader();
#ifdef IS_A_DEVELOPMENT_BUILD
        parser.debugPrint();
#endif

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
#ifdef IS_A_DEVELOPMENT_BUILD
        materialParser.debugPrint();
#endif
    }


    video::ITexture* texture = getTexture(textureDiffuse);
    if (texture)
        bufferMaterial.setTexture(0, texture);

    if (bufferMaterial.getTexture(0) == nullptr)
    {
        _log->addLineAndFlush("No texture !");
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
}

void IO_MeshLoader_WitcherMDL::readSkinNode(io::IReadFile* file, StaticControllersData controllers)
{
    _log->addLineAndFlush(formatString("Skin POS = %d", file->getPos()));
    file->seek(72, true);
    file->seek(16, true); // 0xFFFFFFFFFFFFFFFFFFF...

    file->seek(60, true);
    core::stringc textureStrings[4];
    for (u32 i = 0; i < 4; ++i)
    {
        textureStrings[i] = readStringFixedSize(file, 64);

        if (textureStrings[i] == "NULL")
            textureStrings[i] = "";

        _log->addLineAndFlush(formatString("Mesh texture %d : %s", i, textureStrings[i].c_str()));
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

    file->seek(4, true);

    ArrayDef bonesInfos = readArrayDef(file);

    const long back = file->getPos();
    core::array<scene::ISkinnedMesh::SJoint*> bones;
    file->seek(ModelInfos.offsetTexData + bonesInfos.firstElemOffest);
    for (u32 i = 0; i < bonesInfos.nbUsedEntries; ++i)
    {
        u32 boneId = readU32(file);
        core::stringc boneName = readStringFixedSize(file, 92);
        _log->addLineAndFlush(formatString("Add Bone : %s", boneName.c_str()));
        scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(AnimatedMesh, boneName);
        bones.push_back(joint);
    }
    file->seek(back);

    file->seek(4, true);

    ArrayDef vertexDef = readArrayDef(file);
    ArrayDef normalsDef = readArrayDef(file);

    ArrayDef tangentsDef = readArrayDef(file);
    ArrayDef binormalsDef = readArrayDef(file);
    //std::cout << "vertexDef size : " << vertexDef.nbUsedEntries <<", adress = " << ModelInfos.offsetModelData + vertexDef.firstElemOffest << std::endl;
    //std::cout << "normalsDef size : " << normalsDef.nbUsedEntries <<", adress = " << ModelInfos.offsetModelData + normalsDef.firstElemOffest << std::endl;
    //std::cout << "tangentsDef size : " << tangentsDef.nbUsedEntries <<", adress = " << ModelInfos.offsetModelData + tangentsDef.firstElemOffest << std::endl;
    //std::cout << "binormalsDef size : " << binormalsDef.nbUsedEntries <<", adress = " << ModelInfos.offsetModelData + binormalsDef.firstElemOffest << std::endl;

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
        return;
    }

    file->seek(24, true);
    file->seek(12, true);
    ArrayDef weightingDef = readArrayDef(file);
    core::array<f32> weights = readArray<f32>(file, weightingDef);

    ArrayDef bonesDef = readArrayDef(file);
    core::array<u8> bonesId = readArray<u8>(file, bonesDef);


    TW1_MaterialParser materialParser = readTextures(file);

    // Vertices
    scene::SSkinMeshBuffer* buffer = AnimatedMesh->addMeshBuffer();
    const u16 bufferId = AnimatedMesh->getMeshBufferCount() - 1;
    buffer->VertexType = video::EVT_STANDARD;

    buffer->Vertices_Standard.set_used(vertexDef.nbUsedEntries);

    u32 skinningIndex = 0;
    core::matrix4 transform = controllers.globalTransform;
    file->seek(ModelInfos.offsetRawData + vertexDef.firstElemOffest);
    for (u32 i = 0; i < vertexDef.nbUsedEntries; ++i)
    {
        f32 x = readF32(file);
        f32 y = readF32(file);
        f32 z = readF32(file);

#ifdef TW1_ATTACH_MESHES_TO_NODES
        core::vector3df pos(x, y, z);
#else
        f32 vectIn[3] = {x, y, z};
        f32 vectOut[3];
        transform.transformVec3(vectOut, vectIn);
        core::vector3df pos(vectOut[0], vectOut[1], vectOut[2]);
#endif

        buffer->Vertices_Standard[i].Pos = pos;
        buffer->Vertices_Standard[i].Color = video::SColor(255, 255, 255, 255);
        buffer->Vertices_Standard[i].TCoords = core::vector2df(0.f, 0.f);

        // Skinning of the vertex
        for (int j = 0; j < 4; ++j)
        {
            u32 currentSkinningIndex = skinningIndex++;
            u8 boneId = bonesId[currentSkinningIndex];
            if (boneId == 255)
                continue;

            if (boneId < bones.size()) // supposed to be always true
            {
                f32 weight = weights[currentSkinningIndex];
                scene::ISkinnedMesh::SWeight* w = AnimatedMesh->addWeight(bones[boneId]);
                w->buffer_id = bufferId;
                w->vertex_id = i;
                w->strength = weight;
            }
            else
                _log->addLineAndFlush("Error: boneId >= bones.size()");
        }
    }

    // Normals
    file->seek(ModelInfos.offsetRawData + normalsDef.firstElemOffest);
    for (u32 i = 0; i < normalsDef.nbUsedEntries; ++i)
    {
        f32 x = readS16(file) / 8192.f;
        f32 y = readS16(file) / 8192.f;
        f32 z = readS16(file) / 8192.f;
        buffer->Vertices_Standard[i].Normal = core::vector3df(x, y, z);
        //std::cout << "X=" << x << ", Y=" << y << ", Z=" << z << std::endl;
    }

    // TODO : Binormals + tangents
    // need FVF
    /*
    file->seek(ModelInfos.offsetRawData + tangentsDef.firstElemOffest);
    for (u32 i = 0; i < tangentsDef.nbUsedEntries; ++i)
    {
        f32 x = readS16(file) / 8192.f;
        f32 y = readS16(file) / 8192.f;
        f32 z = readS16(file) / 8192.f;
        //std::cout << "X=" << x << ", Y=" << y << ", Z=" << z << std::endl;
    }
    */
    // need FVF
    /*
    file->seek(ModelInfos.offsetRawData + binormalsDef.firstElemOffest);
    for (u32 i = 0; i < binormalsDef.nbUsedEntries; ++i)
    {
        f32 x = readS16(file) / 8192.f;
        f32 y = readS16(file) / 8192.f;
        f32 z = readS16(file) / 8192.f;
        //std::cout << "X=" << x << ", Y=" << y << ", Z=" << z << std::endl;
    }
    */



    // Faces
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
        _log->addLineAndFlush(formatString("materialFile : %s", materialFile.c_str()));

        TW1_MaterialParser parser(FileSystem);
        parser.loadFile(materialFile);
        textureDiffuse = parser.getTexture(0);
        bufferMaterial.MaterialType = parser.getMaterialTypeFromShader();
        //parser.debugPrint();

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
        _log->addLineAndFlush(formatString("SKIN : try to set texture : %s", textureDiffuse.c_str()));
        //materialParser.debugPrint();
    }


    video::ITexture* texture = getTexture(textureDiffuse);
    if (texture)
        bufferMaterial.setTexture(0, texture);

    if (bufferMaterial.getTexture(0) == nullptr)
    {
        _log->addLineAndFlush("No texture !");
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
}

void IO_MeshLoader_WitcherMDL::readSpeedtreeNode(io::IReadFile* file, StaticControllersData controllers)
{
    //std::cout << "SpeedTree POS = " << file->getPos() << std::endl;

    // Refer to a .spt file to describe the tree
    // seems complicated to use now
    core::stringc filename = GameTexturesPath + readStringFixedSize(file, 32);
    //std::cout << "filename: " << filename.c_str() << std::endl;

    IO_SpeedTreeLoader loader(FileSystem);
    //loader.readSpeedTreeFile(filename);
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
    file->seek(4, true); // inherit color flag
    u32 id = readU32(file);

    core::stringc name = readStringFixedSize(file, 64);
    for (u32 i = 0; i < _depth; ++i)
        _log->add("--");

    _log->addLineAndFlush(formatString("> Node name=%s, id=%d", name.c_str(), id));

    file->seek(8, true); // parent geometry + parent node

    // Children nodes
    ArrayDef def = readArrayDef(file);
    //std::cout << "def count = " << def.nbUsedEntries << ", allocated = " << def.nbAllocatedEntries << ", offset = " << def.firstElemOffest << std::endl;
    core::array<u32> children = readArray<u32>(file, def);
    //std::cout << "child count = " << children.size() << std::endl;


    // Controllers
    ArrayDef controllerKeyDef = readArrayDef(file);
    ArrayDef controllerDataDef = readArrayDef(file);

    StaticControllersData controllersData = getStaticNodeControllers(file, controllerKeyDef, controllerDataDef);


    controllersData.globalTransform = parentMatrix * controllersData.localTransform;
    //


    file->seek(4, true); // node flags/type

    file->seek(8, true); // fixed rot + imposter group ?

    s32 minLOD = readS32(file);
    s32 maxLOD = readS32(file);

    NodeType type = (NodeType) readU32(file);

    if (NodeTypeNames.find(type) != NodeTypeNames.end())
    {
        _log->addLineAndFlush(formatString("type=%s", NodeTypeNames.at(type).c_str()));
    }

    scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(AnimatedMesh, name);
    if (!joint) // when we load a MBA file on the top of a MDL files, joints already exist
    {
        joint = AnimatedMesh->addJoint(parentJoint);
        joint->LocalMatrix = controllersData.localTransform;
        joint->GlobalMatrix= controllersData.globalTransform;
        joint->Name = name.c_str();
        joint->Animatedposition = controllersData.position;
        joint->Animatedrotation = controllersData.rotation.makeInverse();
        joint->Animatedscale = controllersData.scale;
    }

    ConfigNodeType toLoad = (ConfigNodeType)SceneManager->getParameters()->getAttributeAsInt("TW_TW1_NODE_TYPES_TO_LOAD");
    switch (type)
    {
        case kNodeTypeTrimesh:
            if (toLoad & ConfigNodeTrimesh)
            {
                readMeshNode(file, controllersData);
#ifdef TW1_ATTACH_MESHES_TO_NODES
                joint->AttachedMeshes.push_back(AnimatedMesh->getMeshBufferCount() - 1);
#endif
            }
        break;

        case kNodeTypeTexturePaint:
            if (toLoad & ConfigNodeTexturePaint)
            {
                readTexturePaintNode(file, controllersData);
#ifdef TW1_ATTACH_MESHES_TO_NODES
                joint->AttachedMeshes.push_back(AnimatedMesh->getMeshBufferCount() - 1);
#endif
            }
        break;

        case kNodeTypeSkin:
        {
            if (toLoad & ConfigNodeSkin)
            {
                SkinMeshToLoadEntry skinMesh;
                skinMesh.Seek = file->getPos();
                skinMesh.ControllersData = controllersData;
                skinMesh.Joint = joint;
                SkinMeshToLoad.push_back(skinMesh);
            }
        }
        break;


        case kNodeTypeSpeedTree:
            readSpeedtreeNode(file, controllersData);
        break;

        default:
        break;

    }

    _log->addLineAndFlush(formatString("ID = %d, @=%d", (int)(id & 0x00FF), file->getPos()));
    _log->addLineAndFlush("Node END");


    // Load the children
    for (u32 i = 0; i < children.size(); ++i)
    {
        _depth++;
        file->seek(ModelInfos.offsetModelData + children[i]);
        loadNode(file, joint, controllersData.globalTransform);
        _depth--;
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
        _log->addLineAndFlush("offTexture != 0");
    }

    core::stringc materialContent;
    for (u32 i = 0; i < textureCount; ++i)
    {
        core::stringc line = readStringUntilNull(file);

        materialContent += line;
        materialContent += "\n";
    }
    //_log->addLineAndFlush(formatString("mat content : %s", materialContent.c_str()));

    TW1_MaterialParser parser(FileSystem);
    parser.loadFromString(materialContent);
    return parser;
}

void IO_MeshLoader_WitcherMDL::readAnimations(io::IReadFile* file)
{
    f32 timeOffset = 0.f;
    if (ModelInfos.fileVersion == 133)
        file->seek(ModelInfos.offsetModelData + ModelInfos.offsetRawData);
    else
        file->seek(ModelInfos.offsetTexData);

    const long chunkStart = file->getPos();

    file->seek(4, true);
    //std::cout << "ADRESS = " << file->getPos() << std::endl;

    ArrayDef animArrayDef = readArrayDef(file);
    file->seek(chunkStart + animArrayDef.firstElemOffest);
    for (u32 i = 0; i < animArrayDef.nbUsedEntries; ++i)
    {
        u32 animOffset = readU32(file);
        const long back = file->getPos();

        file->seek(ModelInfos.offsetModelData + animOffset);
        // Geom header
        file->seek(8, true);
        core::stringc animationName = readStringFixedSize(file, 64);
        u32 offsetRootNode = readU32(file);
        //std::cout << "offsetRootNode : " << offsetRootNode << std::endl;
        file->seek(32, true);
        u8 geometryType = readU8(file);
        file->seek(3, true);

        // Anim header
        f32 animationLength = readF32(file);
        //std::cout << "animationLength : " << animationLength << std::endl;
        f32 transitionTime = readF32(file);
        core::stringc animationRootName = readStringFixedSize(file, 64);
        ArrayDef eventArrayDef = readArrayDef(file);
        file->seek(6 * 4, true); // animBox
        file->seek(4 * 4, true); // animSphere
        file->seek(4, true); //unknown

        // Load the root node
        file->seek(ModelInfos.offsetModelData + offsetRootNode);
        loadAnimationNode(file, timeOffset);

        // Next animation
        file->seek(back);
        timeOffset += animationLength;
    }
    AnimatedMesh->setAnimationSpeed(1.f);
}

void IO_MeshLoader_WitcherMDL::loadAnimationNode(io::IReadFile* file, f32 timeOffset)
{
    file->seek(24, true); // Function pointers
    file->seek(4, true); // inherit color flag
    u32 id = readU32(file);

    core::stringc name = readStringFixedSize(file, 64);
    for (u32 i = 0; i < _depth; ++i)
        _log->add("--");

    _log->addLineAndFlush(formatString("> Node name=%s, id=%d", name.c_str(), id));

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

    file->seek(4, true); // node flags/type

    file->seek(8, true); // fixed rot + imposter group ?
    file->seek(8, true); // min/max LOD

    NodeType type = (NodeType) readU32(file);

    if (NodeTypeNames.find(type) != NodeTypeNames.end())
    {
        _log->addLineAndFlush(formatString("type=%s", NodeTypeNames.at(type).c_str()));
    }

    scene::ISkinnedMesh::SJoint* joint = JointHelper::GetJointByName(AnimatedMesh, name);
    if (!joint)
    {
        _log->addLineAndFlush(formatString("joint not found: %s", name.c_str()));
    }
    else
    {
        for (u32 i = 0; i < controllers.position.size(); ++i)
        {
            scene::ISkinnedMesh::SPositionKey* key = AnimatedMesh->addPositionKey(joint);
            key->frame = timeOffset + controllers.positionTime[i];
            key->position = controllers.position[i];
        }
        for (u32 i = 0; i < controllers.rotation.size(); ++i)
        {
            scene::ISkinnedMesh::SRotationKey* key = AnimatedMesh->addRotationKey(joint);
            key->frame = timeOffset + controllers.rotationTime[i];
            key->rotation = controllers.rotation[i].makeInverse();
            //std::cout << "key->frame:" << key->frame << " and key->rotation: " << key->rotation.X << ", " << key->rotation.Y << ", " << key->rotation.Z << ", " << key->rotation.W << std::endl;
        }
        for (u32 i = 0; i < controllers.scale.size(); ++i)
        {
            scene::ISkinnedMesh::SScaleKey* key = AnimatedMesh->addScaleKey(joint);
            key->frame = timeOffset + controllers.scaleTime[i];
            key->scale = controllers.scale[i];
        }
    }


    _log->addLineAndFlush(formatString("ID = %d, @=%d", (int)(id & 0x00FF), file->getPos()));
    _log->addLineAndFlush("Animation node END");


    // Load the children
    for (u32 i = 0; i < children.size(); ++i)
    {
        _depth++;
        file->seek(ModelInfos.offsetModelData + children[i]);
        loadAnimationNode(file, timeOffset);
        _depth--;
    }
}

ControllersData IO_MeshLoader_WitcherMDL::readNodeControllers(io::IReadFile* file, ArrayDef key, ArrayDef data)
{
    ControllersData controllers;

    core::array<f32> controllerData = readArray<f32>(file, data);

    const long back = file->getPos();
    file->seek(ModelInfos.offsetModelData + key.firstElemOffest);

    for (u32 i = 0; i < key.nbUsedEntries; ++i)
    {
        u32 controllerType = readU32(file);
        u16 nbRows = readU16(file);
        u16 firstKeyIndex = readU16(file);
        u16 firstValueIndex = readU16(file);
        u8 nbColumns = readU8(file);
        file->seek(1, true); // Pad, not used


        if (controllerType == ControllerPosition)
        {
            for (u32 j = 0; j < nbRows; ++j)
            {
                const u32 offset = j * nbColumns;

                f32 time = controllerData[firstKeyIndex + j];
                controllers.positionTime.push_back(time);

                f32 x = controllerData[offset + firstValueIndex];
                f32 y = controllerData[offset + firstValueIndex + 1];
                f32 z = controllerData[offset + firstValueIndex + 2];
                controllers.position.push_back(core::vector3df(x, y, z));
                //std::cout << "Position: " << x << ", " << y << ", " << z << std::endl;
            }
        }
        else if (controllerType == ControllerOrientation)
        {
            for (u32 j = 0; j < nbRows; ++j)
            {
                const u32 offset = j * nbColumns;

                f32 time = controllerData[firstKeyIndex + j];
                controllers.rotationTime.push_back(time);

                f32 x = controllerData[offset + firstValueIndex];
                f32 y = controllerData[offset + firstValueIndex + 1];
                f32 z = controllerData[offset + firstValueIndex + 2];
                f32 w = controllerData[offset + firstValueIndex + 3];
                controllers.rotation.push_back(core::quaternion(x, y, z, w));
            }
        }
        else if (controllerType == ControllerScale)
        {
            for (u32 j = 0; j < nbRows; ++j)
            {
                const u32 offset = j * nbColumns;

                f32 time = controllerData[firstKeyIndex + j];
                controllers.rotationTime.push_back(time);

                f32 scaleValue = controllerData[offset + firstValueIndex];
                controllers.scale.push_back(core::vector3df(scaleValue, scaleValue, scaleValue));
            }
        }
        else if (controllerType == kNodeTrimeshControllerTypeAlpha)
        {
            for (u32 j = 0; j < nbRows; ++j)
            {
                const u32 offset = j * nbColumns;

                f32 time = controllerData[firstKeyIndex + j];
                controllers.rotationTime.push_back(time);

                f32 alpha = controllerData[offset + firstValueIndex];
                if (alpha != 1.f)
                    _log->addLineAndFlush("alpha != 1");
                controllers.alpha.push_back(alpha);
            }
        }
        else if (controllerType == kNodeTrimeshControllerTypeSelfIllumColor)
        {
            for (u32 j = 0; j < nbRows; ++j)
            {
                const u32 offset = j * nbColumns;

                f32 time = controllerData[firstKeyIndex + j];
                controllers.rotationTime.push_back(time);

                video::SColor color;
                color.setRed    (controllerData[offset + firstValueIndex]);
                color.setGreen  (controllerData[offset + firstValueIndex + 1]);
                color.setBlue   (controllerData[offset + firstValueIndex + 2]);
                controllers.selphIllumColor.push_back(color);
            }
        }
        else
        {
            _log->addLineAndFlush(formatString("Unknown controller type=%d", controllerType));
        }
    }

    file->seek(back);
    return controllers;
}
