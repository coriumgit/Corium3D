#pragma once

#include "TransformsStructs.h"
#include <string>
#include <glm/glm.hpp>

using namespace glm;

namespace Corium3D {

	class ImgsAtlas {
	public:
		//REMINDER: imgsWidth, imgsHeights -> actual imgs sizes inside their cells
		ImgsAtlas(char const* atlasPath, unsigned int horizonCellsNr, unsigned int vertCellsNr,
			unsigned int* imgsWidths, unsigned int* imgsHeights, unsigned int imgsNr);
		virtual ImgsAtlas* clone();
		~ImgsAtlas();
		std::string const& getAtlasPath() const { return atlasPath; }
		float getAtlasAspectRatio() const { return atlasAspectRatio; }
		unsigned int getImgsNr() const { return imgsNr; }
		Rect getImgUVs(unsigned int imgIdx) const;

	protected:
		std::string atlasPath;
		float atlasAspectRatio;
		unsigned int horizonCellsNr;
		unsigned int vertCellsNr;
		float cellWidth;
		float cellHeight;
		float cellUvWidth;
		float cellUvHeight;
		float* imgsUvsWidths;
		float* imgsUvsHeights;
		unsigned int imgsNr;

		ImgsAtlas(ImgsAtlas const& imgsAtlas);
	};

	class GlyphsAtlas : public ImgsAtlas {
	public:
		struct GlyphsIdxsMap {
			unsigned int space;
			unsigned int zeroIdx;
			unsigned int captialAIdx;
			unsigned int smallAIdx;
			unsigned int exclamationMarkIdx;
			unsigned int comma;
			unsigned int period;
			unsigned int leftParenthesis;
			unsigned int rightParenthesis;
		};

		GlyphsAtlas(char const* atlasPath, unsigned int horizonCellsNr, unsigned int vertCellsNr,
			unsigned int* imgsWidths, unsigned int* imgsHeights, unsigned int imgsNr,
			GlyphsIdxsMap const& glyphsIdxsMap);
		GlyphsAtlas* clone() override;
		unsigned int convertCharToIndex(char c);

	private:
		GlyphsIdxsMap glyphsIdxsMap;

		GlyphsAtlas(GlyphsAtlas const& imgsAtlas);
	};

} // namespace Corium3D