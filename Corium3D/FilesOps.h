#pragma once

#include "FilesStructs.h"

namespace Corium3D {

	void readSceneDesc(std::string const& sceneDescFileName, SceneData& sceneDescOut);

	void readModelDesc(std::string const& modelDescFileName, ModelDesc& modelDescOut);	

	void writeSceneDesc(std::string const& sceneDescFileName, SceneData const& sceneDesc);

	void writeModelDesc(std::string const& modelDescFileName, ModelDesc const& modelDesc);		

	void readFileToStr(std::string const& fileName, std::string& strOut);

} // namespace Corium3D
