#include "./OPassimp.h"
#include "./Human/include/Rendering/OPmodel.h"
#include "OPMconvert.h"

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>

OPint OPassimpLoadFile(OPstream* stream, void** asset) {
	OPexporter exporter(stream, NULL);
	return OPMloader(exporter.LoadModelStream(), asset);
}

OPint OPassimpUnLoad(void*) {
	return 1;
}
