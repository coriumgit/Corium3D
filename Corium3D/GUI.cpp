#include "GUI.h"
#include "lodepng.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

namespace Corium3D {

	const GLuint UI_CONTROLS_MAP_EMPTY_PIXEL_VAL = 0xff;
	const Rect UNIT_QUAD = { {0.0f, 0.0f}, {1.0f, 1.0f} };

	const char* renderVertexShaderCode =
		"#version 430 core \n"

		"layout (std430, binding = 0) buffer posesBuffer { \n"
		"	vec2 poses[]; \n"
		"}; \n"
		"layout (std430, binding = 1) buffer texUVsBuffer { \n"
		"	vec2 texUVs[]; \n"
		"}; \n"
		//"layout (std430) buffer emitVertexLogicalsBuffer { \n"
		//"	vec2 emitVertexLogicals[]; \n"
		//"}; \n"
		"in int texIdx; \n"
		"int vertexIdx = 6*gl_InstanceID + gl_VertexID; \n"

		"out vec2 vs2fsTexUV; \n"
		"out flat uint vs2fsTexIdx; \n"
		//"out int vs2gsEmitVertexLogical"

		"void main(void) { \n"
		"	gl_Position = vec4(poses[vertexIdx], 0.0, 1.0f); \n"
		"	vs2fsTexIdx = texIdx; \n"
		"	vs2fsTexUV = texUVs[vertexIdx]; \n"
		//"	vs2gsEmitVertexLogical = emitVertexLogicals[vertexIdx]; \n"
		"}";

	/*const char* renderGeoShaderCode =
		"#version 430 cor \n"
		"precision mediump float; \n"

		"uniform"*/


	const char* renderFragShaderCode =
		"#version 430 core \n"
		"precision mediump float; \n"

		"uniform sampler2DArray sampler; \n"
		"in vec2 vs2fsTexUV; \n"
		"in flat uint vs2fsTexIdx; \n"

		"out vec4 fragColor; \n"

		"void main(void) { \n"
		"	fragColor = texture(sampler, vec3(vs2fsTexUV, vs2fsTexIdx)); \n"
		"}";

	const char* uiControlsMapVertexShaderCode =
		"#version 430 core \n"

		"layout (std430, binding = 0) buffer posesBuffer { \n"
		"	vec2 poses[]; \n"
		"}; \n"
		"in uint uiControlIdx; \n"
		"int vertexIdx = 6*gl_InstanceID + gl_VertexID; \n"

		"out flat uint vs2fsUiControlIdx; \n"

		"void main(void) { \n"
		"	gl_Position = vec4(poses[vertexIdx], 0.0, 1.0f); \n"
		"	vs2fsUiControlIdx = uiControlIdx; \n"
		"}";

	const char* uiControlsMapFragShaderCode =
		"#version 430 core \n"
		"precision mediump float; \n"

		"in flat uint vs2fsUiControlIdx; \n"

		"out uint fragShaderOut; \n"

		"void main(void) { \n"
		"	fragShaderOut = vs2fsUiControlIdx; \n"
		"}";

	/*inline void cpyControlGraphicsDescs(GUI::ControlGraphicsDesc* dest, GUI::ControlGraphicsDesc const* src, unsigned int descsNr) {
		for (unsigned int descIdx = 0; descIdx < descsNr; descIdx++) {
			dest->imgsAtlas = src[descIdx].imgsAtlas->clone();
			dest->instancesNrMax = src[descIdx].instancesNrMax;
			dest->quadsNr = src[descIdx].quadsNr;
		}
	}*/

	GUI::GUI(ImgsAtlas* imgsAtlases, unsigned int imgsAtlasesNr, ImgsControlParams const* imgsControlsParams, unsigned int _imgsControlsNr,
		GlyphsAtlas* glyphsAtlases, unsigned int glyphsAtlasesNr, TxtControlParams const* txtControlsParams, unsigned int _txtControlsNr) :
		atlasesNr(imgsAtlasesNr + glyphsAtlasesNr), imgsControlsNr(_imgsControlsNr), txtControlsNr(_txtControlsNr) {
		atlases = new ImgsAtlas * [atlasesNr];
		for (unsigned int imgsAtlasIdx = 0; imgsAtlasIdx < imgsAtlasesNr; imgsAtlasIdx++)
			atlases[imgsAtlasIdx] = imgsAtlases[imgsAtlasIdx].clone();
		for (unsigned int glyphsAtlasIdx = 0; glyphsAtlasIdx < glyphsAtlasesNr; glyphsAtlasIdx++)
			atlases[imgsAtlasesNr + glyphsAtlasIdx] = glyphsAtlases[glyphsAtlasIdx].clone();

		controlsIdxsInTex = new unsigned int[imgsControlsNr + txtControlsNr];
		controls = new Control * [imgsControlsNr + txtControlsNr];
		for (unsigned int imgsControlIdx = 0; imgsControlIdx < imgsControlsNr; imgsControlIdx++) {
			ImgsControlParams const& controlParams = imgsControlsParams[imgsControlIdx];
			controls[imgsControlIdx] = new Control(*this, (*atlases[controlParams.atlasIdx]), controlParams.quadsNr,
				controlParams.lowerLeftVertex, quadsNrDemanded);
			quadsNrDemanded += controlParams.quadsNr;
			controlsIdxsInTex[imgsControlIdx] = controlParams.atlasIdx;
		}

		for (unsigned int txtControlIdx = 0; txtControlIdx < txtControlsNr; txtControlIdx++) {
			TxtControlParams const& controlParams = txtControlsParams[txtControlIdx];
			unsigned int controlIdxOverall = imgsControlsNr + txtControlIdx;
			controls[controlIdxOverall] = new TxtControl(*this, (GlyphsAtlas&)(*atlases[imgsAtlasesNr + controlParams.imgsControlParams.atlasIdx]), controlParams.imgsControlParams.quadsNr,
				controlParams.imgsControlParams.lowerLeftVertex, quadsNrDemanded,
				controlParams.spaceSz, controlParams.txtHeight, controlParams.txtColor);
			quadsNrDemanded += controlParams.imgsControlParams.quadsNr;
			controlsIdxsInTex[controlIdxOverall] = controlParams.imgsControlParams.atlasIdx;
		}
	}

	GUI::~GUI() {
		for (unsigned int controlIdx = 0; controlIdx < imgsControlsNr + txtControlsNr; controlIdx++)
			delete controls[controlIdx];
		delete[] controls;
		delete[] controlsIdxsInTex;
		for (unsigned int atlasIdx = 0; atlasIdx < atlasesNr; atlasIdx++)
			delete atlases[atlasIdx];
		delete[] atlases;
	}

	bool GUI::initOpenGlLmnts(unsigned int winWidth, unsigned int winHeight) {
		glGenVertexArrays(2, vaos);
		CHECK_GL_ERROR("glGenVertexArrays");
		glGenTextures(1, &controlsTex);
		CHECK_GL_ERROR("glGenTextures");
		glGenBuffers(1, &quadsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &uvsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &texIdxsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		//glGenBuffers(1, &transformatsBuffer);
		//CHECK_GL_ERROR("glGenBuffers");				
		glGenBuffers(1, &controlsIdxsBuffer);
		CHECK_GL_ERROR("glGenBuffers");

		unsigned int controlsNrTotal = imgsControlsNr + txtControlsNr;
		glBindTexture(GL_TEXTURE_2D_ARRAY, controlsTex);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, TEXES_WIDTH, TEXES_HEIGHT, controlsNrTotal);
		CHECK_GL_ERROR("glTexStorage3D");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, quadsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 12 * quadsNrDemanded * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uvsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 12 * quadsNrDemanded * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ARRAY_BUFFER, texIdxsBuffer);
		glBufferData(GL_ARRAY_BUFFER, controlsNrTotal * sizeof(int), NULL, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		//glBindBuffer(GL_ARRAY_BUFFER, transformatsBuffer);
		//glBufferData(GL_ARRAY_BUFFER, quadsNrDemanded * sizeof(mat3), NULL, GL_STATIC_DRAW);
		//CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ARRAY_BUFFER, controlsIdxsBuffer);
		glBufferData(GL_ARRAY_BUFFER, quadsNrDemanded * sizeof(unsigned int), NULL, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");

		unsigned int texWidth, texHeight;
		unsigned char* texData;
		for (unsigned int atlasIdx = 0; atlasIdx < atlasesNr; atlasIdx++) {
			unsigned int res = lodepng_decode_file(&texData, &texWidth, &texHeight, atlases[atlasIdx]->getAtlasPath().c_str(), LCT_RGBA, 8);
			if (res) {
				ServiceLocator::getLogger().loge("GUI", lodepng_error_text(res));
				return false;
			}
			else if (texWidth != TEXES_WIDTH || texHeight != TEXES_HEIGHT) {
				ServiceLocator::getLogger().loge("GUI", "texture %s is of an invalid size.", atlases[atlasIdx]->getAtlasPath().c_str());
				return false;
			}
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, atlasIdx, TEXES_WIDTH, TEXES_HEIGHT, 1, GL_RGBA, GL_UNSIGNED_BYTE, texData);
		}

		unsigned int quadsIt = 0;
		for (unsigned int controlIdx = 0; controlIdx < controlsNrTotal; controlIdx++) {
			glBindBuffer(GL_ARRAY_BUFFER, texIdxsBuffer);
			glBufferSubData(GL_ARRAY_BUFFER, controlIdx * sizeof(unsigned int), sizeof(unsigned int), controlsIdxsInTex + controlIdx);
			CHECK_GL_ERROR("glBufferSubData");
			for (unsigned int quadIdx = 0; quadIdx < controls[controlIdx]->quadsNr; quadIdx++) {
				glBindBuffer(GL_ARRAY_BUFFER, controlsIdxsBuffer);
				//unsigned int temp = 1;
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(unsigned int) * quadsIt, sizeof(unsigned int), &controlIdx);// &controlIdx);
				CHECK_GL_ERROR("glBufferSubData");
				quadsIt++;
			}
		}

		if (!createGlProg(renderVertexShaderCode, renderFragShaderCode, &renderProg, &renderProgVertexShader, &renderProgFragShader))
			return false;

		texSamplerUnifLoc = glGetUniformLocation(renderProg, "sampler");
		//renderProgVertexPosBufferIdx = glGetUniformBlockIndex(renderProg, "posesBuffer");	
		//renderProgUvsBufferIdx = glGetUniformBlockIndex(renderProg, "texUVsBuffer");	
		renderProgTexIdxAttribLoc = glGetAttribLocation(renderProg, "texIdx");
		//renderProgTransformatsAttribLoc = glGetAttribLocation(renderProg, "transformat");

		glBindVertexArray(vaos[0]);
		glUseProgram(renderProg);
		glUniform1i(texSamplerUnifLoc, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, quadsBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, quadsBuffer);
		CHECK_GL_ERROR("glBindBufferBase");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, uvsBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, uvsBuffer);
		CHECK_GL_ERROR("glBindBufferBase");
		glBindBuffer(GL_ARRAY_BUFFER, texIdxsBuffer);
		glVertexAttribPointer(renderProgTexIdxAttribLoc, 1, GL_UNSIGNED_INT, GL_FALSE, 0, (void*)0);
		CHECK_GL_ERROR("glVertexAttribPointer");
		glEnableVertexAttribArray(renderProgTexIdxAttribLoc);
		CHECK_GL_ERROR("glEnableVertexAttribArray");
		glVertexAttribDivisor(renderProgTexIdxAttribLoc, 1);
		CHECK_GL_ERROR("glVertexAttribDivisor");
		/*glBindBuffer(GL_ARRAY_BUFFER, transformatsBuffer);
		for (unsigned transformatColIdx = 0; transformatColIdx < 3; transformatColIdx++) {
			glVertexAttribPointer(renderProgTransformatsAttribLoc + transformatColIdx, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (void*)(sizeof(vec3)*transformatColIdx));
			CHECK_GL_ERROR("glVertexAttribPointer");
			glEnableVertexAttribArray(renderProgTransformatsAttribLoc + transformatColIdx);
			CHECK_GL_ERROR("glEnableVertexAttribArray");
			glVertexAttribDivisor(renderProgTransformatsAttribLoc + transformatColIdx, 1);
			CHECK_GL_ERROR("glVertexAttribDivisor");
		}*/

		glGenFramebuffers(1, &controlsMapFramebuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, controlsMapFramebuffer);
		CHECK_GL_ERROR("glBindFramebuffer");
		glGenRenderbuffers(1, &controlsMapRenderbuffer);
		CHECK_GL_ERROR("glGenRenderbuffers");
		glBindRenderbuffer(GL_RENDERBUFFER, controlsMapRenderbuffer);
		CHECK_GL_ERROR("glBindRenderbuffer");
		// REMINDER: doesnt work in opengl es 3 (can read only rgb/rgba)
		glRenderbufferStorage(GL_RENDERBUFFER, GL_R32UI, winWidth + 1, winHeight + 1);
		CHECK_GL_ERROR("glRenderbufferStorage");
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, controlsMapRenderbuffer);
		CHECK_GL_ERROR("glFramebufferRenderbuffer");
		CHECK_FRAMEBUFFER_STATUS(GL_DRAW_FRAMEBUFFER);

		if (!createGlProg(uiControlsMapVertexShaderCode, uiControlsMapFragShaderCode, &controlsMapProg, &controlsMapVertexShader, &controlsMapFragShader))
			return false;

		//controlsMapProgVertexPosBufferIdx = glGetAttribLocation(controlsMapProg, "posesBuffer");
		//controlsMapProgVertexTransformAttribLoc = glGetAttribLocation(controlsMapProg, "transformat");	
		controlIdxAttribLoc = glGetAttribLocation(controlsMapProg, "uiControlIdx");
		CHECK_GL_ERROR("glGetAttribLocation");
		glBindVertexArray(vaos[1]);
		glUseProgram(controlsMapProg);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, quadsBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, quadsBuffer);
		CHECK_GL_ERROR("glBindBufferBase");

		/*glBindBuffer(GL_VERTEX_ARRAY, transformatsBuffer);
		for (unsigned int transformatColIdx = 0; transformatColIdx < 3; transformatColIdx++) {
			glVertexAttribPointer(controlsMapProgVertexTransformAttribLoc + transformatColIdx, 3, GL_FLOAT, GL_FALSE, sizeof(mat3), (void*)(sizeof(vec3) * transformatColIdx));
			CHECK_GL_ERROR("glVertexAttribPointer");
			glEnableVertexAttribArray(controlsMapProgVertexTransformAttribLoc + transformatColIdx);
			CHECK_GL_ERROR("glEnableVertexAttribArray");
			glVertexAttribDivisor(controlsMapProgVertexTransformAttribLoc + transformatColIdx, 1);
			CHECK_GL_ERROR("glEnableVertexAttribArray");
		}*/

		glBindBuffer(GL_ARRAY_BUFFER, controlsIdxsBuffer);
		glVertexAttribIPointer(controlIdxAttribLoc, 1, GL_UNSIGNED_INT, 0, (void*)0);
		CHECK_GL_ERROR("glVertexAttribIPointer");
		glEnableVertexAttribArray(controlIdxAttribLoc);
		CHECK_GL_ERROR("glEnableVertexAttribArray");
		glVertexAttribDivisor(controlIdxAttribLoc, 1);
		CHECK_GL_ERROR("glVertexAttribDivisor");

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glUseProgram(0);
		glBindVertexArray(0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

#if DEBUG
		isInited = true;
#endif
		return true;
	}

	void GUI::destroyOpenGlLmnts() {
		glDeleteShader(controlsMapVertexShader);
		glDeleteShader(controlsMapFragShader);
		glDeleteProgram(controlsMapProg);
		glDeleteShader(renderProgVertexShader);
		glDeleteShader(renderProgFragShader);
		glDeleteProgram(renderProg);

		glDeleteFramebuffers(1, &controlsMapFramebuffer);
		glDeleteBuffers(1, &quadsBuffer);
		glDeleteBuffers(1, &uvsBuffer);
		//glDeleteBuffers(1, &transformatsBuffer);
		glDeleteBuffers(1, &controlsIdxsBuffer);
		glDeleteTextures(1, &controlsTex);
		glDeleteVertexArrays(2, vaos);
#if DEBUG
		isInited = false;
#endif
	}

	bool GUI::render() {
		if (isActive) {
			glDisable(GL_DEPTH_TEST);
			glUseProgram(renderProg);
			glBindVertexArray(vaos[0]);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quadsNrDemanded);
			CHECK_GL_ERROR("glDrawArraysInstanced");
			glBindVertexArray(0);
			glUseProgram(0);
			glEnable(GL_DEPTH_TEST);
		}

		return true;
	}

	bool GUI::select(float x, float y) {
		if (isActive) {
			glUseProgram(controlsMapProg);
			glBindVertexArray(vaos[1]);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, controlsMapFramebuffer);
			GLuint uiControlsMapInitVals = UI_CONTROLS_MAP_EMPTY_PIXEL_VAL;
			glClearBufferuiv(GL_COLOR, 0, &uiControlsMapInitVals);
			CHECK_GL_ERROR("glClearBufferuiv");
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quadsNrDemanded);
			CHECK_GL_ERROR("glDrawArraysInstanced");
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			GLuint controlIdx;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, controlsMapFramebuffer);
			// TODO: Check what is the range of the viewport coordinates on android
			glReadPixels((GLint)round(x), (GLint)round(y), 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &controlIdx);
			CHECK_GL_ERROR("glReadPixels");
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			if (controlIdx != UI_CONTROLS_MAP_EMPTY_PIXEL_VAL && controls[controlIdx]->onSelectCallback) {
				Rect const& containingQuad = controls[controlIdx]->containingQuad;
				controls[controlIdx]->onSelectCallback((x - containingQuad.lowerLeft.x) / (containingQuad.upperRight.x - containingQuad.lowerLeft.x),
					(y - containingQuad.lowerLeft.y) / (containingQuad.upperRight.y - containingQuad.lowerLeft.y));
				return true;
			}
		}

		return false;
	}

	/*
	bool GUI::hover(float x, float y) {
		unsigned char controlIdx;
		glBindBuffer(GL_READ_FRAMEBUFFER, controlsMapFramebuffer);
		glReadPixels((GLint)round(x), (GLint)round(y), 1, 1, GL_RED, GL_UNSIGNED_INT, (void*)&controlIdx);
		if (controlIdx != UI_CONTROLS_MAP_EMPTY_PIXEL_VAL) {
			controls[controlIdx]->onHoverOver();
			return true;
		}
		else
			return false;
	}
	*/

	/*bool GUI::loadTransformatToBuffer(Rect const& rect, unsigned int bufferIdx) {
		mat3 transformat = translate(vec3((rect.lowerRightX + rect.upperLeftX - 1) / 2, (rect.lowerRightY + rect.upperLeftY - 1) / 2, 0.0f)) *
						   scale(vec3(rect.lowerRightX - rect.upperLeftX, rect.upperLeftY - rect.lowerRightY, 1.0f));
		glBindBuffer(GL_ARRAY_BUFFER, transformatsBuffer);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(mat3) * bufferIdx, sizeof(mat3), value_ptr(transformat));
		CHECK_GL_ERROR("glBufferSubData");

		return true;
	}*/

	GUI::Control::Control(GUI& _gui, ImgsAtlas const& _imgsAtlas, unsigned int _quadsNr, glm::vec2 _lowerLeftVertex, unsigned int _quadsRelatedBuffersIdx) :
		gui(_gui), imgsAtlas(_imgsAtlas), quadsNr(_quadsNr), lowerLeftVertex(_lowerLeftVertex), quadsRelatedBuffersIdx(_quadsRelatedBuffersIdx) {
		quads = new Rect[quadsNr];
		uvs = new Rect[quadsNr];
	}

	GUI::Control::~Control() {
		delete[] quads;
		delete[] uvs;
	}

	void GUI::Control::assignSelectCallback(OnSelectCallback _onSelectCallback) {
		onSelectCallback = _onSelectCallback;
	}

	void GUI::Control::updateGraphics(unsigned int* quadsIdxs, unsigned int quadsNrToUpdate, Rect* _quads, Rect* _uvs) {
#if DEBUG
		if (!gui.isInited)
			throw std::exception("Control's graphics were updated, but the GUI had not been initialized!");
#endif	
		// TODO: use also GL_MAP_FLUSH_EXPLICIT_BIT 
		glBindBuffer(GL_ARRAY_BUFFER, gui.quadsBuffer);
		float* quadsBuffer = (float*)glMapBufferRange(GL_ARRAY_BUFFER, quadsRelatedBuffersIdx * 12 * sizeof(float), quadsNr * 12 * sizeof(float), GL_MAP_WRITE_BIT);
		glBindBuffer(GL_ARRAY_BUFFER, gui.uvsBuffer);
		float* uvsBuffer = (float*)glMapBufferRange(GL_ARRAY_BUFFER, quadsRelatedBuffersIdx * 12 * sizeof(float), quadsNr * 12 * sizeof(float), GL_MAP_WRITE_BIT);
		for (unsigned int quadIdxIdx = 0; quadIdxIdx < quadsNrToUpdate; quadIdxIdx++) {
			unsigned int quadIdx = quadsIdxs[quadIdxIdx];
			unsigned int const quadsRelatedBuffersOffset = quadIdx * 12;
			Rect quad = { {2 * _quads[quadIdx].lowerLeft.x - 1, 2 * _quads[quadIdx].lowerLeft.y - 1},
									{2 * _quads[quadIdx].upperRight.x - 1, 2 * _quads[quadIdx].upperRight.y - 1} };
			quadsBuffer[quadsRelatedBuffersOffset + 0] = quad.lowerLeft.x; quadsBuffer[quadsRelatedBuffersOffset + 1] = quad.lowerLeft.y;
			quadsBuffer[quadsRelatedBuffersOffset + 2] = quad.upperRight.x; quadsBuffer[quadsRelatedBuffersOffset + 3] = quad.lowerLeft.y;
			quadsBuffer[quadsRelatedBuffersOffset + 4] = quad.lowerLeft.x; quadsBuffer[quadsRelatedBuffersOffset + 5] = quad.upperRight.y;
			quadsBuffer[quadsRelatedBuffersOffset + 6] = quad.upperRight.x; quadsBuffer[quadsRelatedBuffersOffset + 7] = quad.lowerLeft.y;
			quadsBuffer[quadsRelatedBuffersOffset + 8] = quad.upperRight.x; quadsBuffer[quadsRelatedBuffersOffset + 9] = quad.upperRight.y;
			quadsBuffer[quadsRelatedBuffersOffset + 10] = quad.lowerLeft.x; quadsBuffer[quadsRelatedBuffersOffset + 11] = quad.upperRight.y;
			quads[quadIdx] = _quads[quadIdx];
			/*vec2 translate = { (quad.upperLeftX + quad.lowerRightX - UNIT_QUAD.upperLeftX - UNIT_QUAD.lowerRightX) / 2,
			(quad.upperLeftY + quad.lowerRightY - UNIT_QUAD.upperLeftY - UNIT_QUAD.lowerRightY) / 2 };
			vec2 scale = { (quad.lowerRightX - quad.upperLeftX) / (UNIT_QUAD.lowerRightX - UNIT_QUAD.upperLeftX),
			(quad.lowerRightY - quad.upperLeftY) / (UNIT_QUAD.lowerRightY - UNIT_QUAD.upperLeftY) };
			transformatsBuffer[quadIdx] = mat3({ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translate.x, translate.y, 1.0f }) *
			mat3({ scale.x, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 1.0f });*/

			Rect quadUVs = _uvs[quadIdx];
			uvsBuffer[quadsRelatedBuffersOffset + 0] = quadUVs.lowerLeft.x; uvsBuffer[quadsRelatedBuffersOffset + 1] = quadUVs.lowerLeft.y;
			uvsBuffer[quadsRelatedBuffersOffset + 2] = quadUVs.upperRight.x; uvsBuffer[quadsRelatedBuffersOffset + 3] = quadUVs.lowerLeft.y;
			uvsBuffer[quadsRelatedBuffersOffset + 4] = quadUVs.lowerLeft.x; uvsBuffer[quadsRelatedBuffersOffset + 5] = quadUVs.upperRight.y;
			uvsBuffer[quadsRelatedBuffersOffset + 6] = quadUVs.upperRight.x; uvsBuffer[quadsRelatedBuffersOffset + 7] = quadUVs.lowerLeft.y;
			uvsBuffer[quadsRelatedBuffersOffset + 8] = quadUVs.upperRight.x; uvsBuffer[quadsRelatedBuffersOffset + 9] = quadUVs.upperRight.y;
			uvsBuffer[quadsRelatedBuffersOffset + 10] = quadUVs.lowerLeft.x; uvsBuffer[quadsRelatedBuffersOffset + 11] = quadUVs.upperRight.y;
			uvs[quadIdx] = quadUVs;
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, gui.quadsBuffer);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		containingQuad = calcContainingQuad(quads, quadsNr);
	}

	Rect GUI::Control::calcContainingQuad(Rect* quads, unsigned int quadsNr) {
		Rect containingQuad = quads[0];
		for (unsigned int quadIdx = 1; quadIdx < quadsNr; quadIdx++) {
			Rect const& quad = quads[quadIdx];

			float leftX, rightX;
			if (containingQuad.lowerLeft.x < quad.upperRight.x) {
				leftX = containingQuad.lowerLeft.x;
				rightX = quad.upperRight.x;
			}
			else {
				leftX = quad.upperRight.x;
				rightX = containingQuad.lowerLeft.x;
			}

			float lowerY, upperY;
			if (containingQuad.lowerLeft.y < quad.upperRight.y) {
				lowerY = containingQuad.lowerLeft.y;
				upperY = quad.upperRight.y;
			}
			else {
				lowerY = quad.upperRight.y;
				upperY = containingQuad.lowerLeft.y;
			}

			containingQuad = { {leftX, lowerY}, {rightX, upperY} };
		}

		return containingQuad;
	}

	GUI::TxtControl::TxtControl(GUI& gui, GlyphsAtlas const& glyphsAtlas, unsigned int quadsNr,
		glm::vec2 lowerLeftVertex, unsigned int _quadsRelatedBuffersIdx,
		float _spaceSz, float _txtHeight, vec4 _txtColor) :
		Control(gui, glyphsAtlas, quadsNr, lowerLeftVertex, _quadsRelatedBuffersIdx),
		spaceSz(_spaceSz), txtHeight(_txtHeight), txtColor(_txtColor) {
		quadsIdxsForUpdate = new unsigned int[quadsNr];
		for (unsigned int quadIdx = 0; quadIdx < quadsNr; quadIdx++)
			quadsIdxsForUpdate[quadIdx] = quadIdx;
	}

	GUI::TxtControl::~TxtControl() {
		delete[] quadsIdxsForUpdate;
	}

	inline unsigned long pown(unsigned long base, unsigned int exp) {
		unsigned long res;
		for (res = 1; exp > 0; exp--)
			res *= base;

		return res;
	}

	GUI::TxtControl& GUI::TxtControl::setTxt(unsigned int number) {
#if DEBUG
		if (number / pown(10, quadsNr) > 0)
			throw std::overflow_error("number's digits number exceeded maximum string length.");
#endif	
		updateTxtControlGraphics(std::to_string(number));
		return *this;
	}

	GUI::TxtControl& GUI::TxtControl::setTxt(std::string const& txt) {
#if DEBUG
		if (txt.length() > quadsNr)
			throw std::overflow_error("txt length maximum string length.");
#endif	
		updateTxtControlGraphics(txt);
		return *this;
	}

	void GUI::TxtControl::updateTxtControlGraphics(std::string const& txt) {
		Rect* quads = new Rect[quadsNr];
		Rect* uvs = new Rect[quadsNr];
		float currX = lowerLeftVertex.x, upperRightY = lowerLeftVertex.y + txtHeight;
		unsigned int charIdx;
		// TODO: Fix this design, coz it will fail if graphicsDesc.imgsAtlas is not a GlyphsAtlas
		GlyphsAtlas& glyphsAtlas = (GlyphsAtlas&)imgsAtlas;
		for (charIdx = 0; charIdx < txt.length(); charIdx++) {
			uvs[charIdx] = glyphsAtlas.getImgUVs(glyphsAtlas.convertCharToIndex(txt[charIdx]));
			quads[charIdx].lowerLeft.x = currX;
			quads[charIdx].lowerLeft.y = lowerLeftVertex.y;
			quads[charIdx].upperRight.x = currX + txtHeight * (uvs[charIdx].upperRight.x - uvs[charIdx].lowerLeft.x) / ((uvs[charIdx].lowerLeft.y - uvs[charIdx].upperRight.y) * glyphsAtlas.getAtlasAspectRatio());
			quads[charIdx].upperRight.y = upperRightY;
			currX = quads[charIdx].upperRight.x + spaceSz;
		}
		for (unsigned int quadIdx = charIdx; charIdx < quadsNr; charIdx++) {
			quads[charIdx] = { {0.0f, 0.0f}, {0.0f, 0.0f} };
			uvs[charIdx] = { {0.0f, 0.0f}, {0.0f, 0.0f} };
		}

		updateGraphics(quadsIdxsForUpdate, quadsNr, quads, uvs);

		delete[] quads;
		delete[] uvs;
	}

} // namespace Corium3D