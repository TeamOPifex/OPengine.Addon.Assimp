#pragma once

#include "./Core/include/OPtypes.h"
#include "./Data/include/OPcman.h"
#include "./Human/include/Rendering/OPmesh.h"

#include "assimp/Importer.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>

OPint OPassimpLoadFile(OPstream* stream, void** asset);
OPint OPassimpUnLoad(void*);

inline void OPassimpAddLoaders() {

	OPassetLoader loader;
	loader.Extension = ".obj";
	loader.AssetTypePath = "Models/";
	loader.AssetSize = sizeof(OPmesh);
	loader.Load = (OPint(*)(OPstream*, void**))OPassimpLoadFile;
	loader.Unload = (OPint(*)(void*))OPassimpUnLoad;
	loader.Reload = NULL;
	OPCMAN.AddLoader(&loader);



	OPassetLoader loaderBlend;
	loaderBlend.Extension = ".blend";
	loaderBlend.AssetTypePath = "Models/";
	loaderBlend.AssetSize = sizeof(OPmesh);
	loaderBlend.Load = (OPint(*)(OPstream*, void**))OPassimpLoadFile;
	loaderBlend.Unload = (OPint(*)(void*))OPassimpUnLoad;
	OPCMAN.AddLoader(&loaderBlend);


	OPassetLoader loaderFBX;
	loaderFBX.Extension = ".fbx";
	loaderFBX.AssetTypePath = "Models/";
	loaderFBX.AssetSize = sizeof(OPmesh);
	loaderFBX.Load = (OPint(*)(OPstream*, void**))OPassimpLoadFile;
	loaderFBX.Unload = (OPint(*)(void*))OPassimpUnLoad;
	OPCMAN.AddLoader(&loaderFBX);


	OPassetLoader loaderDAE;
	loaderDAE.Extension = ".dae";
	loaderDAE.AssetTypePath = "Models/";
	loaderDAE.AssetSize = sizeof(OPmesh);
	loaderDAE.Load = (OPint(*)(OPstream*, void**))OPassimpLoadFile;
	loaderDAE.Unload = (OPint(*)(void*))OPassimpUnLoad;
	OPCMAN.AddLoader(&loaderDAE);

}
