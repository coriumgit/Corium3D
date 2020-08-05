#include "ImgsAtlas.h"
#include "ServiceLocator.h"
#include "lodepng.h"

namespace Corium3D {

	ImgsAtlas::ImgsAtlas(char const* _atlasPath, unsigned int _horizonCellsNr, unsigned int _vertCellsNr,
		unsigned int* imgsWidths, unsigned int* imgsHeights, unsigned int _imgsNr) :
		atlasPath(_atlasPath), horizonCellsNr(_horizonCellsNr), vertCellsNr(_vertCellsNr),
		cellUvWidth(1.0f / _horizonCellsNr), cellUvHeight(1.0f / _vertCellsNr), imgsNr(_imgsNr) {
		unsigned char* atlasData;
		unsigned int texWidth, texHeight;
		unsigned res = lodepng_decode_file(&atlasData, &texWidth, &texHeight, atlasPath.c_str(), LCT_RGBA, 8);
		if (res) {
			ServiceLocator::getLogger().loge("ImgsAtlas", lodepng_error_text(res));
			throw std::exception("lodepng failed.");
		}
		atlasAspectRatio = (float)texHeight / texWidth;
		delete atlasData;

		imgsUvsWidths = new float[imgsNr];
		imgsUvsHeights = new float[imgsNr];
		cellWidth = (float)texWidth / horizonCellsNr;
		cellHeight = (float)texHeight / vertCellsNr;
		for (unsigned int imgIdx = 0; imgIdx < imgsNr; imgIdx++) {
			imgsUvsWidths[imgIdx] = (float)imgsWidths[imgIdx] / texWidth;
			imgsUvsHeights[imgIdx] = (float)imgsHeights[imgIdx] / texHeight;
		}
	}

	ImgsAtlas::ImgsAtlas(ImgsAtlas const& imgsAtlas) : atlasPath(imgsAtlas.atlasPath), atlasAspectRatio(imgsAtlas.atlasAspectRatio),
		horizonCellsNr(imgsAtlas.horizonCellsNr), vertCellsNr(imgsAtlas.vertCellsNr),
		cellWidth(imgsAtlas.cellWidth), cellHeight(imgsAtlas.cellHeight),
		cellUvWidth(imgsAtlas.cellUvWidth), cellUvHeight(imgsAtlas.cellUvHeight), imgsNr(imgsAtlas.imgsNr) {
		imgsUvsWidths = new float[imgsAtlas.imgsNr];
		imgsUvsHeights = new float[imgsAtlas.imgsNr];
		memcpy(imgsUvsWidths, imgsAtlas.imgsUvsWidths, imgsAtlas.imgsNr * sizeof(float));
		memcpy(imgsUvsHeights, imgsAtlas.imgsUvsHeights, imgsAtlas.imgsNr * sizeof(float));
	}

	ImgsAtlas* ImgsAtlas::clone() {
		return new ImgsAtlas(*this);
	}

	ImgsAtlas::~ImgsAtlas() {
		delete[] imgsUvsWidths;
		delete[] imgsUvsHeights;
	}

	Rect ImgsAtlas::getImgUVs(unsigned int imgIdx) const {
		Rect uvs;
		uvs.lowerLeft.x = imgIdx % horizonCellsNr * cellUvWidth;
		uvs.upperRight.y = imgIdx / horizonCellsNr * cellUvHeight;
		uvs.upperRight.x = uvs.lowerLeft.x + imgsUvsWidths[imgIdx];
		uvs.lowerLeft.y = uvs.upperRight.y + imgsUvsHeights[imgIdx];

		return uvs;
	}

	GlyphsAtlas::GlyphsAtlas(char const* atlasPath, unsigned int horizonCellsNr, unsigned int vertCellsNr,
		unsigned int* imgsWidths, unsigned int* imgsHeights, unsigned int imgsNr,
		GlyphsIdxsMap const& _glyphsIdxsMap) :
		ImgsAtlas(atlasPath, horizonCellsNr, vertCellsNr, imgsWidths, imgsHeights, imgsNr), glyphsIdxsMap(_glyphsIdxsMap) {}

	GlyphsAtlas::GlyphsAtlas(GlyphsAtlas const& other) : ImgsAtlas(other), glyphsIdxsMap(other.glyphsIdxsMap) {}

	GlyphsAtlas* GlyphsAtlas::clone() {
		return new GlyphsAtlas(*this);
	}

	unsigned int GlyphsAtlas::convertCharToIndex(char c) {
		if (c > 64 && c < 91) // A-Z
			return c - 65 + glyphsIdxsMap.captialAIdx;
		else if (c > 96 && c < 123) // a-z
			return c - 97 + glyphsIdxsMap.smallAIdx;
		else if (c > 47 && c < 58) // 0-9
			return c - 48 + glyphsIdxsMap.zeroIdx;
		else if (c == 32) // (space)
			return glyphsIdxsMap.space;
		else if (c == 43) // +
			return 38;
		else if (c == 45) // -
			return 39;
		else if (c == 33) // !
			return glyphsIdxsMap.exclamationMarkIdx;
		else if (c == 63) // ?
			return 37;
		else if (c == 61) // =
			return 40;
		else if (c == 58) // :
			return 41;
		else if (c == 46) // .
			return glyphsIdxsMap.period;
		else if (c == 44) // ,
			return glyphsIdxsMap.comma;
		else if (c == 42) // *
			return 44;
		else if (c == 36) // $
			return 45;
		else if (c == 40) // (
			return glyphsIdxsMap.leftParenthesis;
		else if (c == 41) // )
			return glyphsIdxsMap.rightParenthesis;
	}

} // namespace Corium3D