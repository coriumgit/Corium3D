#pragma once

#include "FilesStructs.h"

namespace Corium3D {

	void readSceneDesc(std::string const& sceneDescFileName, SceneDesc& sceneDescOut);

	void readModelDesc(std::string const& modelDescFileName, ModelDesc& modelDescOut);

	void readColliderData(std::string const& colliderDataFileName, ColliderData& colliderDataOut);

	void writeSceneDesc(std::string const& sceneDescFileName, SceneDesc const& sceneDesc);

	void writeModelDesc(std::string const& modelDescFileName, ModelDesc const& modelDesc);	

	void writeColliderData(std::string const& colliderDataFileName, ColliderData const& colliderData);

	void readFileToStr(std::string const& fileName, std::string& strOut);

} // namespace Corium3D
