//
// Created by omer on 15/02/02018.
//

#pragma once

#include "OpenGL.h"
#include "ImgsAtlas.h"
#include "TransformsStructs.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <functional>
#include <stdexcept>

namespace Corium3D {

	class GUI {
	public:
		typedef std::function<void(float x, float y)> OnSelectCallback;

		struct ImgsControlParams {
			unsigned int atlasIdx;
			unsigned int quadsNr;
			glm::vec2 lowerLeftVertex;
		};

		struct TxtControlParams {
			ImgsControlParams imgsControlParams;
			float spaceSz;
			float txtHeight;
			vec4 txtColor;
		};

		class Control {
		public:
			friend GUI;

			void assignSelectCallback(OnSelectCallback onSelectCallback);
			void updateGraphics(unsigned int* quadsIdxs, unsigned int quadsNr, Rect* quads, Rect* uvs);
			//void updateQuads(unsigned int* quadsIdxs, Rect const* quad);
			//void updateUvs(unsigned int* quadsIdxs, Rect const* uvs);

		protected:
			ImgsAtlas const& imgsAtlas;
			unsigned int quadsNr;
			glm::vec2 lowerLeftVertex;
			OnSelectCallback onSelectCallback = NULL;

			Control(GUI& gui, ImgsAtlas const& imgsAtlas, unsigned int quadsNr,
				glm::vec2 lowerLeftVertex, unsigned int quadsRelatedBuffersIdx);
			~Control();

		private:
			GUI& gui;
			Rect containingQuad;
			Rect* quads;
			Rect* uvs;
			unsigned int quadsRelatedBuffersIdx;

			static Rect calcContainingQuad(Rect* quads, unsigned int quadsNr);
		};

		class TxtControl : public GUI::Control {
		public:
			friend GUI;
			TxtControl& setTxt(unsigned int number);
			TxtControl& setTxt(std::string const& txt);

		private:
			TxtControl(GUI& gui, GlyphsAtlas const& glyphsAtlas, unsigned int quadsNr,
				glm::vec2 lowerLeftVertex, unsigned int _quadsRelatedBuffersIdx,
				float spaceSz, float txtHeight, vec4 txtColor);
			~TxtControl();

			//char* txt;	
			float txtHeight;
			//float glyphsAtlasAspectRatio;
			vec4 txtColor;
			float spaceSz;
			// TODO: see how to avoid updating all the quads every time.
			unsigned int* quadsIdxsForUpdate;

			void updateTxtControlGraphics(std::string const& txt);
		};

		static constexpr unsigned int TEXES_WIDTH = 1024;
		static constexpr unsigned int TEXES_HEIGHT = 384;

		GUI(ImgsAtlas* imgsAtlases, unsigned int imgsAtlasesNr, ImgsControlParams const* imgsControlsParams, unsigned int imgsControlsNr,
			GlyphsAtlas* glyphsAtlases, unsigned int glyphsAtlasesNr, TxtControlParams const* txtControlsParams, unsigned int txtControlsNr);
		GUI(GUI const&) = delete;
		~GUI();
		bool initOpenGlLmnts(unsigned int screenWidth, unsigned int screenHeight);
		void destroyOpenGlLmnts();
		bool render();
		bool select(float x, float y);
		Control& accessControl(unsigned int controlIdx) {
#if DEBUG
			if (controlIdx > imgsControlsNr)
				throw std::out_of_range("Img control index out of range.");
#endif
			return *(controls[controlIdx]);
		}

		TxtControl& accessTxtControl(unsigned int controlIdx) {
#if DEBUG
			if (controlIdx > txtControlsNr)
				throw std::out_of_range("Txt control index out of range.");
#endif
			return *((TxtControl*)controls[imgsControlsNr + controlIdx]);
		}

		void show() { isActive = true; }
		void hide() { isActive = false; }

		//bool hover(float x, float y);

	private:
		ImgsAtlas** atlases;
		unsigned int atlasesNr;
		unsigned int* controlsIdxsInTex;
		unsigned int imgsControlsNr;
		unsigned int txtControlsNr;
		Control** controls; // TODO: change to callbacks pointers and containing quads	
		unsigned int quadsNrDemanded = 0;
		bool isActive = false;
#if DEBUG	
		bool isInited = false;
#endif

		GLuint vaos[2];
		GLuint controlsTex;
		GLuint quadsBuffer;
		GLuint uvsBuffer;
		GLuint texIdxsBuffer;
		//GLuint transformatsBuffer;
		GLuint renderProg;
		GLuint renderProgVertexShader;
		GLuint renderProgFragShader;
		GLuint renderProgVertexPosBufferIdx;
		GLuint renderProgUvsBufferIdx;
		GLuint renderProgTexIdxAttribLoc;
		//GLuint renderProgTransformatsAttribLoc;
		GLuint texSamplerUnifLoc;

		GLuint controlsMapFramebuffer;
		GLuint controlsMapRenderbuffer;
		GLuint controlsIdxsBuffer;
		GLuint controlsMapProg;
		GLuint controlsMapVertexShader;
		GLuint controlsMapFragShader;
		GLuint controlsMapProgVertexPosBufferIdx;
		//GLuint controlsMapProgVertexTransformAttribLoc;
		GLuint controlIdxAttribLoc;

		//bool loadTransformatToBuffer(Rect const& rect, unsigned int bufferIdx);
	};

} // namespace Corium3D

