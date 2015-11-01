// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_W2ENT_MESH_FILE_LOADER_H_INCLUDED__
#define __C_W2ENT_MESH_FILE_LOADER_H_INCLUDED__

#include "IMeshLoader.h"
#include "irrString.h"
#include "SMesh.h"
#include "SAnimatedMesh.h"
#include "IMeshManipulator.h"
#include "ISkinnedMesh.h"

//#include "os.h"

#include "log.h"



namespace irr
{

struct DataInfos
{
    int size;
    int adress;
};

struct Meshdata
{
    int nModel;
    core::array<int> nMat;
    DataInfos infos;
};

struct Submesh_data
{
    int vertype;
    core::array <int> dataI;
    core::array <unsigned short> dataH;
};

struct UV
{
    float u;
    float v;
};

struct Mat
{
    int id;
    video::SMaterial material;
};

struct bone_data
{
    core::stringc name;
    irr::core::matrix4 matr;
};



namespace io
{
	class IFileSystem;
	class IReadFile;
} // end namespace io
namespace scene
{
class IMeshManipulator;

//! Meshloader capable of loading w2ent meshes.
class CW2ENTMeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CW2ENTMeshFileLoader(scene::ISceneManager* smgr, io::IFileSystem* fs);

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IReferenceCounted::drop() for more information.
	virtual IAnimatedMesh* createMesh(io::IReadFile* file);


private:

    // Main function
	bool load(io::IReadFile* file);

    void CMesh(io::IReadFile* file, Meshdata tmp);
    void CSkeleton(io::IReadFile* file, DataInfos infos);

    void skeleton(io::IReadFile* file);
    void drawmesh(io::IReadFile* file, core::array<int> data, core::array<int> mats, int nModel);
    void CMaterialInstance(io::IReadFile* file, DataInfos infos, int nMats);
    void TEXTURE(io::IReadFile* file, core::stringc xbm_file, core::array<int> data, core::array<core::stringc> stringsXBM);
    void convertXBMToDDS(core::stringc xbm_file);
    void drawmesh_static(io::IReadFile* file, core::array<int> data, core::array<int> mats, int nModel);
    void vert_format(io::IReadFile* file);
    void static_meshes(io::IReadFile* file, core::array<int> mats, int nModel);

	bool find (core::array<core::stringc> stringVect, core::stringc name);
	core::stringc searchParent(core::stringc bonename);

	void check_armature(io::IReadFile* file);
    void make_bone(io::IReadFile* file);
    void make_bone_parent(io::IReadFile* file);
    void make_bone_position(io::IReadFile* file);
    void make_localMatrix_from_global(io::IReadFile* file);
    void computeLocal(scene::ISkinnedMesh::SJoint* joint);

    void addVectorToLog(irr::core::vector3df vec);
    void addMatrixToLog(irr::core::matrix4 mat);


    void make_vertex_group(Submesh_data dataSubMesh, core::array<core::array<unsigned char> > weighting);

    video::ITexture* getTexture(core::stringc filename);

    // Read functions
    core::stringc readWord(io::IReadFile* f, int nbLetter);
    core::array<int> readInts (io::IReadFile* f, int nbInt);
    core::array<unsigned short> readUnsignedShorts (io::IReadFile* f, int nbShorts);
    core::array<unsigned char> readUnsignedChars (io::IReadFile* f, int nbChar);
    core::array<float> readFloats (io::IReadFile* f, int nbInt);

    // Attributes
    scene::ISceneManager* SceneManager;
    io::IFileSystem* FileSystem;
    //scene::SAnimatedMesh* AnimatedMesh;
    scene::ISkinnedMesh* AnimatedMesh;
	//scene::SMesh* Mesh;
	unsigned int NbSubMesh;

    core::array<Submesh_data> SubMeshData;
    core::array<core::array<unsigned char> >IdLOD;
    // Data type table
    core::array<core::stringc> Strings;
    // External files table
    core::array<core::stringc> FilesTable;
    // Informations about a CMesh
    core::array <Meshdata> MeshesToLoad;
    // Materials of the meshes
    core::array<Mat> Materials;

    io::path GamePath;

    // Bones data
    irr::core::array<core::stringc> bonenames;
    irr::core::array<bone_data> bones_data;

    //DEBUG
    Log log;


};

} // end namespace scene
} // end namespace irr

#endif
