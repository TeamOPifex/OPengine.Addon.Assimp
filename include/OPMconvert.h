#pragma once

#include "./OPengine.h"
#include "./OPassimp.h"
#include "Utils.h"


enum ModelFeatures {
	Model_Positions = 0,
	Model_Normals = 1,
	Model_UVs = 2,
	Model_Colors = 3,
	Model_Indices = 4,
	Model_Tangents = 5,
	Model_Bones = 6,
	Model_Skinning = 7,
	Model_Animations = 8,
	Model_Skeletons = 9,
	Model_Meta = 10,
	Model_Bitangents = 11,
	MAX_FEATURES
};

struct AnimationTrack {
	OPchar* Name;
	double Duration;
	ui32 Start;
	ui32 End;
};


struct OPskeletonAnimationResult {
	OPskeletonAnimation** Animations;
	OPchar** AnimationNames;
	OPuint AnimationsCount;
};

#include <map>
#include <vector>

struct BoneInfo
{
	i32 ParentIndex = -1;
	const OPchar* Name = NULL;
	aiNode* Node = NULL;
	aiBone* Bone = NULL;
	OPmat4 BoneOffset = OPMAT4_IDENTITY;
	OPmat4 FinalTransformation = OPMAT4_IDENTITY;
	aiMatrix4x4 Transform;
};

struct AnimationsResult {
	OPstream** Animations;
	OPuint Count;
};

struct AnimationSplit {
	OPchar* Name;
	ui32 Start;
	ui32 End;
};

struct OPexporter {
	bool Feature_Normals,
		Feature_UVs,
		Feature_Tangents,
		Feature_BiTangents,
		Feature_Colors,
		Feature_Bones,
		Export_Model,
		Export_Skeleton,
		Export_Animations;

	bool HasAnimations;

	ui32 splitCount;
	ui32* splitStart;
	ui32* splitEnd;
	OPchar** splitName;

	f32 scale = 1.0f;

	const OPchar* path;

	Assimp::Importer importer;
	const aiScene* scene;

	AnimationTrack* animationTracks;
	ui32 animationCount;

	ui32 features[MAX_FEATURES];
	OPindexSize::Enum indexSize;

	ui32 index = 0;
	OPfloat* boneWeights;
	i32* boneIndices;

	std::map<std::string, ui32> boneMapping;
	ui32 numBones;
	vector<BoneInfo> boneInfo;

	OPmodel* existingModel;

	OPexporter() { }

	OPexporter(const OPchar* filename, OPmodel* desc) {
		Init(filename, desc);
	}
	OPexporter(OPstream* stream, OPmodel* desc) {
		Init(stream, desc);
	}

	void Init(const OPchar* filename, OPmodel* desc);
	void Init(OPstream* stream, OPmodel* desc);
	void Export();
	void Export(const OPchar* output);
	void Export(AnimationSplit* splitters, ui32 count);
	void Export(AnimationSplit* splitters, ui32 count, const OPchar* output);
	OPskeleton* LoadSkeleton();
	OPskeletonAnimationResult LoadAnimations();
	OPskeletonAnimationResult LoadAnimations(const OPchar* name);
	OPskeletonAnimationResult LoadAnimations(AnimationSplit* splitters, ui32 count);
	OPstream* LoadModelStream();
	AnimationsResult LoadAnimationStreams();
	AnimationsResult LoadAnimationStreams(AnimationSplit* splitters, ui32 count);

	// Private
	void _write(const OPchar* outputFinal);
	void _write(AnimationSplit* splitters, ui32 count, const OPchar* outputFinal);
	void _writeSkeleton(const OPchar* outputFinal);
	void _writeAnimations(const OPchar* outputFinal);
	void _setFeatures();
	ui32 _getFeaturesFlag();
	ui32 _getTotalVertices();
	ui32 _getTotalIndices();
	ui32 _getTotalVertices(aiMesh* mesh);
	ui32 _getTotalIndices(aiMesh* mesh);
	void _writeMeshData(OPstream* str);
	void _setBoneData(aiMesh* mesh);
	void _loadBones();
	void _setHierarchy();
	void _setHierarchy(aiNode* node, i32 parent, aiMatrix4x4 transform);
	const aiNodeAnim* _findNodeAnim(const aiAnimation* pAnimation, const string NodeName);
	void __writeAnimData(OPstream* str, aiAnimation* anim, ui32 frameCount, ui32 start, ui32 end);
	OPstream* _loadSkeletonStream();
};

struct BoneWeight {
	OPfloat weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	i32 bones[4] = { 0, 0, 0, 0 };
};

ui32 GetAvailableTracks(const OPchar* path, OPchar** buff, double* durations, ui32 max);