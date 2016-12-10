#include "OPMconvert.h"
#include "./Data/include/OPstring.h"
#include "./Human/include/Rendering/OPMvertex.h"


void OPexporter::Init(OPstream* stream, OPmodel* desc) {
	path = stream->Source;
	existingModel = desc;

	Feature_Normals = false;
	Feature_UVs = false;
	Feature_Colors = false;
	Feature_Bones = Export_Skeleton = false;
	Feature_BiTangents = Feature_Tangents = false;
	HasAnimations = false;
	Export_Model = false;

	const OPchar* ext = strrchr(stream->Source, '.');
	ui32 features = aiProcess_CalcTangentSpace |
		aiProcess_GenNormals |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType;
	/* | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes*/

	if (OPstringEquals(ext, ".obj")) {
		// If it's a .obj file, we're going to reload the entire file
		// with Assimp so that it loads the material (.mtl)
		// The from memory method doesn't support loading .mtl yet
		scene = importer.ReadFile(stream->Source, features);
	}
	else {
		scene = importer.ReadFileFromMemory(stream->Data, stream->Size, features);
	}


	// If the import failed, report it
	if (!scene)
	{
		OPlogErr("Failed");
		return;
	}

	HasAnimations = Export_Animations = scene->HasAnimations();
	if (scene->HasMeshes()) {
		aiMesh* mesh = scene->mMeshes[0];
		Feature_Normals = mesh->HasNormals();
		Feature_UVs = mesh->HasTextureCoords(0);
		Feature_Tangents = Feature_BiTangents = mesh->HasTangentsAndBitangents();
		Feature_Colors = mesh->HasVertexColors(0);
		Feature_Bones = Export_Skeleton = mesh->HasBones();
		Export_Model = true;
		existingModel = NULL;

		OPstream* result = LoadModelStream();
		OPMloader(result, (void**)&existingModel);
	}

}

void OPexporter::Init(const OPchar* filename, OPmodel* desc) {
	OPstream* stream = OPfile::ReadFromFile(filename);
	Init(stream, desc);
}

OPmat4 aiMatrixToOPmat4(aiMatrix4x4 mat) {
	OPmat4 result;
	for (ui32 i = 0; i < 4; i++) {
		for (ui32 j = 0; j < 4; j++) {
			result[i][j] = mat[i][j];
		}
	}
	return result;
}

void OPexporter::_loadBones() {
	boneMapping.clear();
	_setHierarchy();

	for (ui32 m = 0; m < scene->mNumMeshes; m++) {
		aiMesh* mesh = scene->mMeshes[m];

		for (ui32 i = 0; i < mesh->mNumBones; i++) {
			string BoneName(mesh->mBones[i]->mName.data);
			ui32 BoneIndex = boneMapping[BoneName]; 
			boneInfo[BoneIndex].BoneOffset = aiMatrixToOPmat4(mesh->mBones[i]->mOffsetMatrix);
			boneInfo[BoneIndex].Bone = mesh->mBones[i];
		}
	}

}

void OPexporter::_setHierarchy(aiNode* node, i32 parent, aiMatrix4x4 transform) {
	string NodeName(node->mName.data);
	boneMapping[NodeName] = index;
	BoneInfo bi;
	boneInfo.push_back(bi);
	boneInfo[index].Node = node;
	boneInfo[index].ParentIndex = parent;
	boneInfo[index].Name = node->mName.C_Str();
	boneInfo[index].Transform = node->mTransformation; //   transform * 
	OPlogErr("%s : %d", boneInfo[index].Name, boneInfo[index].ParentIndex);
	OPmat4Log("  Transform: ", aiMatrixToOPmat4(boneInfo[index].Transform));
	index++;
	numBones++;
	ui32 curr = index - 1;
	for (ui32 i = 0; i < node->mNumChildren; i++) {
		_setHierarchy(node->mChildren[i], curr, boneInfo[curr].Transform);
	}
}

void OPexporter::_setHierarchy() {
	index = 0;
	numBones = 0;
	_setHierarchy(scene->mRootNode, -1, aiMatrix4x4());
}


OPstream* OPexporter::_loadSkeletonStream() {

	OPstream* str = OPstream::Create(256);

	str->I16(numBones); // bone count

	OPmat4 global = aiMatrixToOPmat4(scene->mRootNode->mTransformation);
	OPmat4Inverse(&global, global);

	// Global Inverse Transform
	for (ui32 j = 0; j < 4; j++) {
		for (ui32 k = 0; k < 4; k++) {
			str->F32((f32)global[j][k]);
		}
	}

	for (ui32 i = 0; i < numBones; i++) {
		str->I16(boneInfo[i].ParentIndex);
		str->WriteString(boneInfo[i].Name);

		OPmat4 invBindPose, invOffset;
		OPmat4Inverse(&invBindPose, aiMatrixToOPmat4(boneInfo[i].Transform));
		OPmat4Inverse(&invOffset, boneInfo[i].BoneOffset);

		OPmat4 transform = aiMatrixToOPmat4(boneInfo[i].Transform);

		// Bind pose
		for (ui32 j = 0; j < 4; j++) {
			for (ui32 k = 0; k < 4; k++) {
				//str->F32((f32)OPMAT4_IDENTITY.cols[j].row[k]);
				//str->F32((f32)boneInfo[i].Transform[j][k]);
				//str->F32((f32)invBindPose.cols[j].row[k]);				
				str->F32((f32)transform[j][k]);
			}
		}

		// Offset
		for (ui32 j = 0; j < 4; j++) {
			for (ui32 k = 0; k < 4; k++) {
				//str->F32((f32)invOffset.cols[j].row[k]);
				//str->F32((f32)OPMAT4_IDENTITY.cols[j].row[k]);
				str->F32((f32)boneInfo[i].BoneOffset[j][k]);
			}
		}
	}

	str->Reset();

	return str;
}

OPskeleton* OPexporter::LoadSkeleton() {

	_loadBones();

	OPskeleton* result = NULL;
	
	OPstream* str = _loadSkeletonStream();
	OPloaderOPskeletonLoad(str, &result);

	return result;
}

const aiNodeAnim* OPexporter::_findNodeAnim(const aiAnimation* pAnimation, const string NodeName)
{
	for (ui32 i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

		if (string(pNodeAnim->mNodeName.data) == NodeName) {
			return pNodeAnim;
		}
	}

	return NULL;
}

i32 _animBonePos(aiAnimation* anim, aiBone* bone) {
	if (bone == NULL) return -1;

	for (ui32 i = 0; i < anim->mNumChannels; i++) {
		if (anim->mChannels[i]->mNodeName == bone->mName) {
			return i;
		}
	}
	return -1;
}

void __writeAnimationframe(OPstream* str, OPanimationFrame frame) {
	str->F32(frame.Time); // Time
	str->F32(frame.Position.x); str->F32(frame.Position.y); str->F32(frame.Position.z); // Position
	str->F32(frame.Rotation.x); str->F32(frame.Rotation.y); str->F32(frame.Rotation.z); str->F32(frame.Rotation.w); // Rotation
	str->F32(frame.Scale.x); str->F32(frame.Scale.y); str->F32(frame.Scale.z); // Scale
}

void __writeEmptyFrame(OPstream* str, ui32 frameCount, ui32 start, ui32 end) {
	OPanimationFrame frame = {
		0,
		OPVEC3_ZERO,
		OPQUAT_IDENTITY,
		OPVEC3_ONE
	};

	for (ui32 i = start; i <= end && i < frameCount; i++) {
		__writeAnimationframe(str, frame);
	}
}

ui32 __getFrameCount(aiAnimation* anim) {
	ui32 max = 0;
	for (ui32 i = 0; i < anim->mNumChannels; i++) {
		if (anim->mChannels[i]->mNumPositionKeys > max) {
			max = anim->mChannels[i]->mNumPositionKeys;
		}
		if (anim->mChannels[i]->mNumRotationKeys > max) {
			max = anim->mChannels[i]->mNumRotationKeys;
		}
		if (anim->mChannels[i]->mNumScalingKeys > max) {
			max = anim->mChannels[i]->mNumScalingKeys;
		}
	}
	return max;
}

void __writeChannels(OPstream* str, aiNodeAnim* channel, ui32 frameCount, ui32 start, ui32 end) {
	OPanimationFrame frame = {
		0,
		OPVEC3_ZERO,
		OPQUAT_IDENTITY,
		OPVEC3_ONE
	};

	for (ui32 i = start; i <= end && i < frameCount; i++) {
		if (i < channel->mNumPositionKeys)
			frame.Time = channel->mPositionKeys[i].mTime;
		else if (i < channel->mNumRotationKeys)
			frame.Time = channel->mRotationKeys[i].mTime;
		else if (i < channel->mNumScalingKeys)
			frame.Time = channel->mScalingKeys[i].mTime;

		if (i < channel->mNumPositionKeys)
			frame.Position = OPvec3(channel->mPositionKeys[i].mValue.x, channel->mPositionKeys[i].mValue.y, channel->mPositionKeys[i].mValue.z);
		if (i < channel->mNumRotationKeys)
			frame.Rotation = OPquat(channel->mRotationKeys[i].mValue.x, channel->mRotationKeys[i].mValue.y, channel->mRotationKeys[i].mValue.z, channel->mRotationKeys[i].mValue.w);
		if (i < channel->mNumScalingKeys)
			frame.Scale = OPvec3(channel->mScalingKeys[i].mValue.x, channel->mScalingKeys[i].mValue.y, channel->mScalingKeys[i].mValue.z);

		__writeAnimationframe(str, frame);
	}
}

void OPexporter::__writeAnimData(OPstream* str, aiAnimation* anim, ui32 frameCount, ui32 start, ui32 end) {
	for (ui32 j = 0; j < numBones; j++) {
		i32 ind = _animBonePos(anim, boneInfo[j].Bone);
		if (ind == -1) {
			// Write an empty frame
			__writeEmptyFrame(str, frameCount, start, end);
		}
		else {
			// Write Channels
			__writeChannels(str, anim->mChannels[ind], frameCount, start, end);
		}
	}
}

AnimationsResult OPexporter::LoadAnimationStreams() {
	AnimationsResult result;

	result.Count = scene->mNumAnimations;
	result.Animations = OPALLOC(OPstream*, scene->mNumAnimations);

	_loadBones();

	for (ui32 i = 0; i < scene->mNumAnimations; i++) {
		aiAnimation* anim = scene->mAnimations[i];
		OPstream* str = OPstream::Create(1024); 
		result.Animations[i] = str;
		
		str->Source = OPstringCopy(anim->mName.C_Str());
		str->WriteString(anim->mName.C_Str()); // Animation Name
		str->I16(numBones); // Number of bones in Animation Track

		ui32 frameCount = __getFrameCount(anim);
		str->UI32(frameCount); // Total number of Frames

		__writeAnimData(str, anim, frameCount, 0, frameCount);

		str->Reset();
	}

	return result;
}

AnimationsResult OPexporter::LoadAnimationStreams(AnimationSplit* splitters, ui32 count) {
	AnimationsResult result;

	result.Count = count;
	result.Animations = OPALLOC(OPstream*, count);

	_loadBones();

	// for splitting we assume there's only 1 animation track for now
	aiAnimation* anim = scene->mAnimations[0];

	for (ui32 i = 0; i < count; i++) {
		OPstream* str = OPstream::Create(1024);
		result.Animations[i] = str;

		str->Source = OPstringCopy(splitters[i].Name);
		str->WriteString(splitters[i].Name); // Animation Name
		str->I16(numBones); // Number of bones in Animation Track

		ui32 frameCount = __getFrameCount(anim);

		str->UI32(splitters[i].End - splitters[i].Start + 1); // Total number of Frames

		__writeAnimData(str, anim, frameCount, splitters[i].Start, splitters[i].End);

		str->Reset();
	}

	return result;
}

OPskeletonAnimationResult OPexporter::LoadAnimations() {
	OPskeletonAnimationResult result;

	AnimationsResult anims = LoadAnimationStreams();

	result.AnimationsCount = anims.Count;
	result.Animations = OPALLOC(OPskeletonAnimation*, anims.Count);
	result.AnimationNames = OPALLOC(OPchar*, anims.Count);
	for (ui32 i = 0; i < anims.Count; i++) {
		if (OPloaderOPanimationLoad(anims.Animations[i], &result.Animations[i])) {
			result.AnimationNames[i] = result.Animations[i]->Name;
		}
	}

	return result;
}

OPskeletonAnimationResult OPexporter::LoadAnimations(AnimationSplit* splitters, ui32 count) {
	OPskeletonAnimationResult result;

	AnimationsResult anims = LoadAnimationStreams(splitters, count);

	result.AnimationsCount = count;
	result.Animations = OPALLOC(OPskeletonAnimation*, count);
	result.AnimationNames = OPALLOC(OPchar*, count);

	for (ui32 i = 0; i < count; i++) {
		if (OPloaderOPanimationLoad(anims.Animations[i], &result.Animations[i])) {
			result.AnimationNames[i] = result.Animations[i]->Name;
		}
	}

	return result;
}

void OPexporter::Export(AnimationSplit* splitters, ui32 count, const OPchar* output) {
	OPchar* out;
	if (output == NULL) {
		out = OPstringCopy(path);
	}
	else {
		out = OPstringCopy(output);
	}

	OPstring check = OPstring(out);
	check.ToLower();
	OPint contains = check.Contains(".opm");
	if (contains > 0) {
		out[contains] = NULL;
	}

	// Now we can access the file's contents
	OPchar* outputFinal = OPstringCreateMerged(out, ".opm");

	_write(splitters, count, outputFinal);

	OPfree(out);
}

void OPexporter::Export() {
	Export(NULL, 0, NULL);
}

void OPexporter::Export(const OPchar* output) {
	Export(NULL, 0, output);
}

void OPexporter::Export(AnimationSplit* splitters, ui32 count) {
	Export(splitters, count, NULL);
}

void OPexporter::_setFeatures() {
	OPbzero(features, sizeof(ui32) * MAX_FEATURES);
	features[Model_Positions] = 1;
	features[Model_Normals] = Feature_Normals;
	features[Model_UVs] = Feature_UVs;
	features[Model_Tangents] = Feature_Tangents;
	features[Model_Bitangents] = Feature_BiTangents;
	features[Model_Indices] = 1;
	features[Model_Colors] = Feature_Colors;
	features[Model_Bones] = Feature_Bones;
	features[Model_Skinning] = Feature_Bones;
	features[Model_Skeletons] = Export_Skeleton;
	features[Model_Animations] = Export_Animations;
	features[Model_Meta] = 0;


	// All of the meshes must have the same layout
	// We'll turn features off if we have to
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		ui32 stride = 0;
		if (features[Model_Normals] && !mesh->HasNormals()) {
			OPlogErr("Mesh %d didn't have 'Normals' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Normals] = 0;
		}
		if (features[Model_UVs] && !mesh->HasTextureCoords(0)) {
			OPlogErr("Mesh %d didn't have 'UVs' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_UVs] = 0;
		}
		if (features[Model_Tangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Tangents' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Tangents] = 0;
		}
		if (features[Model_Bitangents] && !mesh->HasTangentsAndBitangents()) {
			OPlogErr("Mesh %d didn't have 'Bitangents' to add: '%s'", i, mesh->mName.C_Str());
			features[Model_Bitangents] = 0;
		}
		if (features[Model_Bones] && !mesh->HasBones()) {
			OPlogErr("Mesh %d didn't have 'Bones' to add: '%s'", i, mesh->mName.C_Str());
			//features[Model_Bones] = 0;
		}
	}
}

ui32 OPexporter::_getFeaturesFlag() {
	ui32 featureFlags = 0;
	if (features[Model_Positions]) featureFlags += 0x01;
	if (features[Model_Normals]) featureFlags += 0x02;
	if (features[Model_UVs]) featureFlags += 0x04;
	if (features[Model_Tangents]) featureFlags += 0x08;
	if (features[Model_Bitangents]) featureFlags += 0x400;
	if (features[Model_Colors]) featureFlags += 0x100;
	if (features[Model_Indices]) featureFlags += 0x10;
	if (features[Model_Bones]) featureFlags += 0x20;
	if (features[Model_Skinning]) featureFlags += 0x40;
	if (features[Model_Animations]) featureFlags += 0x80;
	if (features[Model_Meta]) featureFlags += 0x200;

	return featureFlags;
}

ui32 OPexporter::_getTotalVertices() {
	ui32 totalVerticesEntireModel = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			totalVerticesEntireModel += face.mNumIndices;
		}
	}

	return totalVerticesEntireModel;
}

ui32 OPexporter::_getTotalIndices() {
	ui32 totalIndicesEntireModel = 0;

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices == 3) {
				totalIndicesEntireModel += 3;
			}
			else if (face.mNumIndices == 4) {
				totalIndicesEntireModel += 6;
			}
		}
	}

	return totalIndicesEntireModel;
}

ui32 OPexporter::_getTotalVertices(aiMesh* mesh) {
	ui32 totalVertices = 0;
	for (ui32 j = 0; j < mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		totalVertices += face.mNumIndices;
	}
	return totalVertices;
}

ui32 OPexporter::_getTotalIndices(aiMesh* mesh) {
	ui32 totalIndices = 0;
	for (ui32 j = 0; j < mesh->mNumFaces; j++) {
		aiFace face = mesh->mFaces[j];
		if (face.mNumIndices == 3) {
			totalIndices += 3;
		}
		else if (face.mNumIndices == 4) {
			totalIndices += 6;
		}
	}
	return totalIndices;
}

void OPexporter::_setBoneData(aiMesh* mesh) {
	i32* boneCounts = NULL;
	boneCounts = (i32*)OPallocZero(sizeof(i32) * mesh->mNumVertices);
	for (ui32 boneInd = 0; boneInd < mesh->mNumBones; boneInd++) {
		const aiBone* bone = mesh->mBones[boneInd];
		for (int boneWeightInd = 0; boneWeightInd < bone->mNumWeights; boneWeightInd++) {
			const aiVertexWeight* weight = &bone->mWeights[boneWeightInd];
			boneCounts[weight->mVertexId]++;
		}
	}

	ui32 maxBones = 0;
	for (ui32 maxBoneInd = 0; maxBoneInd < mesh->mNumVertices; maxBoneInd++) {
		if (maxBones < boneCounts[maxBoneInd]) {
			maxBones = boneCounts[maxBoneInd];
		}
	}
	for (ui32 boneCountInd = 0; boneCountInd < mesh->mNumVertices; boneCountInd++) {
		boneCounts[boneCountInd] = 0;
	}

	// maxBones is now the largest number of bones per vertex
	if (maxBones > 4) {
		OPlogErr("Can't handle more than 4 weights right now");
		return;
	}

	boneWeights = (OPfloat*)OPallocZero(sizeof(OPfloat) * mesh->mNumVertices * 4);
	boneIndices = (i32*)OPallocZero(sizeof(i32) * mesh->mNumVertices * 4);
	for (ui32 boneInd = 0; boneInd < mesh->mNumBones; boneInd++) {
		const aiBone* bone = mesh->mBones[boneInd];
		string BoneName(bone->mName.data);
		ui32 index = boneMapping[BoneName];

		for (int boneWeightInd = 0; boneWeightInd < bone->mNumWeights; boneWeightInd++) {
			const aiVertexWeight* weight = &bone->mWeights[boneWeightInd];
			ui32 offset = boneCounts[weight->mVertexId]++;

			boneWeights[(weight->mVertexId * 4) + offset] = weight->mWeight;
			boneIndices[(weight->mVertexId * 4) + offset] = index;
		}
	}

	OPfree(boneCounts);
}

void __writeStringNoPath(OPstream* str, const OPchar* path) {
	if (strlen(path) != 0) {
#ifdef OPIFEX_WINDOWS
		const OPchar* ext = strrchr(path, '\\');
#else
		const OPchar* ext = strrchr(path, '/');
#endif
		if (ext == NULL) { // There was no path
			str->WriteString(path);
		}
		else { // Path has been removed
			str->WriteString(&ext[1]);
		}
	}
	else {
		// Path has no length, empty string
		str->WriteString("");
	}
}

void OPexporter::_writeMeshData(OPstream* str) {

	OPlogInfo("Model Export\n========================");
	ui32 offset = 0;
	
	_loadBones();

	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];

		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}

		// Mesh name
		str->WriteString(mesh->mName.C_Str());

		// Total Vertices and Indices in Mesh
		ui32 totalVertices = _getTotalVertices(mesh);
		ui32 totalIndices = _getTotalIndices(mesh);
		str->UI32(totalVertices);
		str->UI32(totalIndices);

		OPboundingBox3D boundingBox;


		if (mesh->HasBones() && Feature_Bones) {
			_setBoneData(mesh);
		}

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			aiVector3D verts[4];
			aiVector3D normals[4];
			aiVector3D uvs[4];
			aiColor4D colors[4];
			aiVector3D bitangents[4];
			aiVector3D tangents[4];
			BoneWeight boneWeightIndex[4];

			for (ui32 k = 0; k < face.mNumIndices; k++) {
				verts[k] = mesh->mVertices[face.mIndices[k]];

				OPvec3 point = OPvec3(verts[k].x * scale, verts[k].y * scale, verts[k].z * scale);

				if (point.x < boundingBox.min.x) boundingBox.min.x = point.x;
				if (point.y < boundingBox.min.y) boundingBox.min.y = point.y;
				if (point.z < boundingBox.min.z) boundingBox.min.z = point.z;
				if (point.x > boundingBox.max.x) boundingBox.max.x = point.x;
				if (point.y > boundingBox.max.y) boundingBox.max.y = point.y;
				if (point.z > boundingBox.max.z) boundingBox.max.z = point.z;

				if (mesh->HasNormals()) {
					normals[k] = mesh->mNormals[face.mIndices[k]];
				}
				if (mesh->HasTextureCoords(0)) {
					// Only supporting 1 layer of texture coordinates right now
					uvs[k] = mesh->mTextureCoords[0][face.mIndices[k]];
				}
				if (mesh->HasVertexColors(0)) {
					// Only supporting 1 layer of colors right now
					colors[k] = mesh->mColors[0][face.mIndices[k]];
				}
				if (mesh->HasTangentsAndBitangents()) {
					bitangents[k] = mesh->mBitangents[face.mIndices[k]];
					tangents[k] = mesh->mTangents[face.mIndices[k]];
				}
				if (mesh->HasBones() && Feature_Bones) {
					boneWeightIndex[k].weights[0] = boneWeights[face.mIndices[k] * 4 + 0];
					boneWeightIndex[k].weights[1] = boneWeights[face.mIndices[k] * 4 + 1];
					boneWeightIndex[k].weights[2] = boneWeights[face.mIndices[k] * 4 + 2];
					boneWeightIndex[k].weights[3] = boneWeights[face.mIndices[k] * 4 + 3];
					boneWeightIndex[k].bones[0] = boneIndices[face.mIndices[k] * 4 + 0];
					boneWeightIndex[k].bones[1] = boneIndices[face.mIndices[k] * 4 + 1];
					boneWeightIndex[k].bones[2] = boneIndices[face.mIndices[k] * 4 + 2];
					boneWeightIndex[k].bones[3] = boneIndices[face.mIndices[k] * 4 + 3];
				}
			}

			// Write each vertex
			for (ui32 k = 0; k < face.mNumIndices; k++) {
				// Position
				str->F32(verts[k].x * scale);
				str->F32(verts[k].y * scale);
				str->F32(verts[k].z * scale);

				// Normal
				if (mesh->HasNormals() && features[Model_Normals]) {
					str->F32(normals[k].x);
					str->F32(normals[k].y);
					str->F32(normals[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Tangents]) {
					str->F32(tangents[k].x);
					str->F32(tangents[k].y);
					str->F32(tangents[k].z);
				}

				if (mesh->HasTangentsAndBitangents() && features[Model_Bitangents]) {
					str->F32(bitangents[k].x);
					str->F32(bitangents[k].y);
					str->F32(bitangents[k].z);
				}

				if (mesh->HasTextureCoords(0) && features[Model_UVs]) {
					str->F32(uvs[k].x);
					str->F32(uvs[k].y);
				}

				if (mesh->HasBones() && features[Model_Bones]) {
					// TODO
					str->F32(boneWeightIndex[k].bones[0]);
					str->F32(boneWeightIndex[k].bones[1]);
					str->F32(boneWeightIndex[k].bones[2]);
					str->F32(boneWeightIndex[k].bones[3]);
					str->F32(boneWeightIndex[k].weights[0]);
					str->F32(boneWeightIndex[k].weights[1]);
					str->F32(boneWeightIndex[k].weights[2]);
					str->F32(boneWeightIndex[k].weights[3]);
				}

				if (mesh->HasVertexColors(0) && features[Model_Colors]) {
					str->F32(colors[k].r);
					str->F32(colors[k].g);
					str->F32(colors[k].b);
				}
				else if (features[Model_Colors]) {
					str->F32(0);
					str->F32(0);
					str->F32(0);
				}
			}
		}

		if (mesh->HasBones() && Feature_Bones) {
			if (boneWeights != NULL) OPfree(boneWeights);
			if (boneIndices != NULL) OPfree(boneIndices);
		}

		OPlogInfo(" Mesh[%d] Offset %d", i, offset);

		for (ui32 j = 0; j < mesh->mNumFaces; j++) {
			aiFace face = mesh->mFaces[j];
			if (face.mNumIndices > 4) {
				OPlogErr("Only Supporting 3 and 4 point faces.");
				continue;
			}

			if (indexSize == OPindexSize::INT) {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					str->UI32(offset++);
					str->UI32(offset++);
					str->UI32(offset++);
				}
				else {
					//OPlog("Quad");
					str->UI32(offset + 0);
					str->UI32(offset + 1);
					str->UI32(offset + 2);
					str->UI32(offset + 0);
					str->UI32(offset + 2);
					str->UI32(offset + 3);
					offset += 4;
				}
			}
			else {
				if (face.mNumIndices == 3) {
					//OPlog("Triangle");
					str->UI16(offset++);
					str->UI16(offset++);
					str->UI16(offset++);
				}
				else {
					//OPlog("Quad");
					str->UI16(offset + 0);
					str->UI16(offset + 1);
					str->UI16(offset + 2);
					str->UI16(offset + 0);
					str->UI16(offset + 2);
					str->UI16(offset + 3);
					offset += 4;
				}
			}
		}



		str->F32(boundingBox.min.x);
		str->F32(boundingBox.min.y);
		str->F32(boundingBox.min.z);
		str->F32(boundingBox.max.x);
		str->F32(boundingBox.max.y);
		str->F32(boundingBox.max.z);



		aiString path;

		scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &path);
		__writeStringNoPath(str, path.data); // Diffuse
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");
		str->WriteString("");

		// Write Meta Data

		//if (model != NULL) {
		//	writeU32(myFile, model->meshes[i].meshMeta->count);
		//	for (ui32 j = 0; j < model->meshes[i].meshMeta->count; j++) {
		//		writeU32(myFile, (ui32)model->meshes[i].meshMeta->metaType[j]);
		//		OPchar* test = model->meshes[i].meshMeta->data[j]->String();
		//		writeString(myFile, test);
		//		//write(&myFile, model->meshes[i].meshMeta->data->Data, model->meshes[i].meshMeta->data->Length);
		//	}
		//}
		//else {
		str->UI32(0);
			//writeU32(myFile, 0);
		//}

	}

	OPlogInfo(" Offset %d", offset);

}

void OPexporter::_write(const OPchar* outputFinal) {
	_write(NULL, 0, outputFinal);
}

void OPexporter::_write(AnimationSplit* splitters, ui32 count, const OPchar* outputFinal) {
	{ // Model File
		ofstream myFile(outputFinal, ios::binary);
		OPstream* result = LoadModelStream();
		write(&myFile, result->Data, result->Size);
		myFile.close();
	}

	// Now create skeleton
	if (Export_Skeleton) {
		OPstream* str = _loadSkeletonStream();

		OPstring skelOut(outputFinal);
		skelOut.Add(".skel");

		ofstream myFile(skelOut.C_Str(), ios::binary);

		write(&myFile, str->Data, str->Size);
		myFile.close();
		//_writeSkeleton(outputFinal);
	}

	// Now create animations
	if (Export_Animations && scene->HasAnimations()) {
		AnimationsResult result;
		if (count == 0) {
			result = LoadAnimationStreams();
		}
		else {
			result = LoadAnimationStreams(splitters, count);
		}
	
		for (ui32 i = 0; i < result.Count; i++) {
			OPstring animOut(outputFinal);
			animOut.Add(".");
			animOut.Add(result.Animations[i]->Source);
			animOut.Add(".anim");

			ofstream myFile(animOut.C_Str(), ios::binary);
			write(&myFile, result.Animations[i]->Data, result.Animations[i]->Size);
			myFile.close();
		}
		//_writeAnimations(outputFinal);
	}

	OPlog(output);

}

OPstream* OPexporter::LoadModelStream() {
	
	OPstream* str = OPstream::Create(1024);

	_setFeatures();

	str->UI16(3);

	str->WriteString(scene->mRootNode->mName.C_Str());

	// Number of meshes
	ui32 meshCount = 0;
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		if (features[Model_Bones] && !mesh->HasBones()) {
			continue;
		}
		meshCount++;
	}
	str->UI32(meshCount);

	// Features in the OPM
	ui32 featureFlags = _getFeaturesFlag();
	str->UI32(featureFlags);

	// Vertex Mode
	// 1 == Vertex Stride ( Pos/Norm/Uv )[]
	// 2 == Vertex Arrays ( Pos )[] ( Norm )[] ( Uv )[]
	str->UI16(1);


	ui32 totalVerticesEntireModel = _getTotalVertices();
	ui32 totalIndicesEntireModel = _getTotalIndices();

	str->UI32(totalVerticesEntireModel);
	str->UI32(totalIndicesEntireModel);

	// Index Size SHORT (16) or INT (32)
	indexSize = OPindexSize::INT;
	str->UI8((ui8)indexSize);

	_writeMeshData(str);

	str->Reset();

	return str;
}

void OPexporter::_writeSkeleton(const OPchar* outputFinal) {
	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		char buffer[20];
        sprintf(buffer, "%d", i);
		OPchar* skelFileNum = OPstringCreateMerged(outputFinal, buffer);
		OPchar* skeletonOutput = OPstringCreateMerged(skelFileNum, ".skel");
		ofstream skelFile(outputFinal, ios::binary);

		writeI16(&skelFile, mesh->mNumBones);

		for (ui32 i = 0; i < mesh->mNumBones; i++) {
			const aiBone* bone = mesh->mBones[i];
			writeI16(&skelFile, i);
			writeString(&skelFile, bone->mName.C_Str());
			for (ui32 j = 0; j < 4; j++) {
				for (ui32 k = 0; k < 4; k++) {
					writeF32(&skelFile, bone->mOffsetMatrix[j][k]);
				}
			}
		}

		skelFile.close();
	}
}

void OPexporter::_writeAnimations(const OPchar* outputFinal) {
	for (ui32 i = 0; i < scene->mNumAnimations; i++) {
		const aiAnimation* animation = scene->mAnimations[i];

		OPchar* animationWithName = OPstringCreateMerged(outputFinal, animation->mName.C_Str());
		OPchar* animationOutput = OPstringCreateMerged(animationWithName, ".anim");
		ofstream animFile(animationOutput, ios::binary);

		writeI16(&animFile, animation->mNumChannels);
		writeString(&animFile, animation->mName.C_Str());
		writeU32(&animFile, (ui32)animation->mDuration);

		for (ui32 j = 0; j < scene->mNumMeshes; j++) {
			const aiMesh* mesh = scene->mMeshes[j];
		}

		for (ui32 j = 0; j < animation->mNumChannels; j++) {
			const aiNodeAnim* data = animation->mChannels[j];

		}

		animFile.close();
	}
}

ui32 GetAvailableTracks(const OPchar* filename, OPchar** buff, double* durations, ui32 max) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_OptimizeGraph
	);

	// If the import failed, report it
	if (!scene)
	{
		OPlogErr("Failed");
		return 0;
	}

	if (!scene->HasAnimations()) {
		return 0;
	}

	ui32 i = 0;
	for (; i < scene->mNumAnimations && i < max; i++) {
		buff[i] = OPstringCopy(scene->mAnimations[i]->mName.C_Str());
		durations[i] = scene->mAnimations[i]->mDuration;
	}

	return i;
}





//
//ui32 OPexporter::_findBoneIndex(const OPchar* name) {
//	ui32 ind = 0;
//
//	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
//		const aiMesh* mesh = scene->mMeshes[i];
//
//		if (!mesh->HasBones()) continue;
//
//		for (ui32 j = 0; j < mesh->mNumBones; j++) {
//			if (OPstringEquals(mesh->mBones[j]->mName.C_Str(), name)) {
//				// This is the same bone
//				return ind;
//			}
//			ind++;
//		}
//	}
//}
//
//OPmat4 OPexporter::_findBoneOffset(ui32 boneInd) {
//	ui32 ind = 0;
//
//	for (ui32 i = 0; i < scene->mNumMeshes; i++) {
//		const aiMesh* mesh = scene->mMeshes[i];
//
//		if (!mesh->HasBones()) continue;
//		for (ui32 b = 0; b < mesh->mNumBones; b++) {
//			if (ind == boneInd) { // This is the bone we're looking for
//				OPmat4 result;
//				for (ui32 j = 0; j < 4; j++) {
//					for (ui32 k = 0; k < 4; k++) {
//						result[j][k] = mesh->mBones[b]->mOffsetMatrix[j][k];
//					}
//				}
//				return result;
//			}
//			ind++;
//		}
//	}
//
//	return OPMAT4_IDENTITY;
//}
//
//OPmat4 OPexporter::_findBoneOffset(const OPchar* name) {
//	return _findBoneOffset(_findBoneIndex(name));
//}