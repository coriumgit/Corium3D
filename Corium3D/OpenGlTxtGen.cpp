//
// Created by omer on 16/02/02018.
//

#include "OpenGlTxtGen.h"

#include "GraphicsComponent.h"
#include "lodepng.h"
#include "ServiceLocator.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.inl>
#include <cstring>

static constexpr const char* txtVertexShader =
	"#version 430 core \n"

	"uniform mat4 uMvpMat; \n"
	"in vec4 position; \n"
	"in vec4 color; \n"
	"in vec2 texCoords; \n"

	"out vec4 vs2fsColor; \n"
	"out vec2 vs2fsTexCoords; \n"

	"void main(void) { \n"
	"   vs2fsColor = color; \n"
	"   vs2fsTexCoords = texCoords; \n"
	"   gl_Position = uMvpMat * position; \n"
	"}";

static constexpr const char* txtFragShader =
	"#version 430 core \n"
	"precision mediump float; \n"

	"uniform sampler2D texSampler; \n"
	"in vec4 vs2fsColor; \n"
	"in vec2 vs2fsTexCoords; \n"
	"out vec4 fragColor; \n"

	"void main(void) { \n"
	"   fragColor = texture(texSampler, vs2fsTexCoords)*vs2fsColor; \n"
	"   fragColor.rgb *= vs2fsColor.a; \n"
	"}";

OpenGlTxtGen::OpenGlTxtGen(const char* _txtAtlasPath, unsigned int _atlasHorizonCellsNr, unsigned int _atlasVertCellsNr,
                           unsigned int _spaceSz, unsigned int* _glyphsSzs, GlyphsIdxsMap& _glyphsIdxsMap) :
        txtAtlasPath(_txtAtlasPath), atlasHorizonCellsNr(_atlasHorizonCellsNr), atlasVertCellsNr(_atlasVertCellsNr),
        atlasCellUVWidth(1.0f/_atlasHorizonCellsNr), atlasCellUVHeight(1.0f/_atlasVertCellsNr),
        spaceSz(_spaceSz), glyphsIdxsMap(_glyphsIdxsMap) {
    unsigned int glyphsNr = _atlasHorizonCellsNr*_atlasVertCellsNr;
    glyphsSzs = new unsigned int[glyphsNr];
    memcpy(glyphsSzs, _glyphsSzs, glyphsNr*sizeof(unsigned int));
}

OpenGlTxtGen::~OpenGlTxtGen() {
    delete[] glyphsSzs;
}

bool OpenGlTxtGen::initOpenGlLmnts() {
    unsigned char* txtAtlasData;
    unsigned int texWidth, texHeight;
    unsigned res = lodepng_decode_file(&txtAtlasData, &texWidth, &texHeight, txtAtlasPath.c_str(), LCT_RGBA, 8);
    if (res) {
        ServiceLocator::getLogger().loge("OpenGlTxtGen", lodepng_error_text(res));
        return false;
    }
    atlasCellWidth = (float)texWidth/atlasHorizonCellsNr;
    atlasCellHeight = (float)texHeight/atlasVertCellsNr;

    glGenTextures(1, &atlasTex);
    CHECK_GL_ERROR("glGenBuffers");
    glBindTexture(GL_TEXTURE_2D, atlasTex);
    CHECK_GL_ERROR("glBindTexture");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texWidth, texHeight);
	CHECK_GL_ERROR("glTexStorage2D");
	glTexSubImage2D(GL_TEXTURE_2D, 1, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, txtAtlasData);
    CHECK_GL_ERROR("glTexSubImage2D");
    glBindTexture(GL_TEXTURE_2D, 0);

    delete txtAtlasData;
#if DEBUG
    isInited = true;
#endif
    return true;
}

void OpenGlTxtGen::destroyOpenGlLmnts() {
    glDeleteTextures(1, &atlasTex);
}

unsigned int OpenGlTxtGen::convertCharToIndex(char c) {
    unsigned int idx = 0;

    // Retrieve the index
    if(c>64&&c<91) // A-Z
        idx = c - 65 + glyphsIdxsMap.captialAIdx;
    else if(c>96&&c<123) // a-z
        idx = c - 97 + glyphsIdxsMap.smallAIdx;
    else if(c>47&&c<58) // 0-9
        idx = c - 48 + glyphsIdxsMap.zeroIdx;
    else if(c==43) // +
        idx = 38;
    else if(c==45) // -
        idx = 39;
    else if(c==33) // !
        idx = glyphsIdxsMap.exclamationMarkIdx;
    else if(c==63) // ?
        idx = 37;
    else if(c==61) // =
        idx = 40;
    else if(c==58) // :
        idx = 41;
    else if(c==46) // .
        idx = glyphsIdxsMap.period;
    else if(c==44) // ,
        idx = 43;
    else if(c==42) // *
        idx = 44;
    else if(c==36) // $
        idx = 45;

    return idx;
}

void OpenGlTxtGen::TxtGraphicsComponent::fillVerticesArr(float* verticesArr, float currX){
    verticesArr[0] = currX;
    verticesArr[1] = 0.0f;
    verticesArr[2] = 0.0f;
    verticesArr[3] = currX + txtGen->atlasCellWidth*glyphResizeFactor;
    verticesArr[4] = 0.0f;
    verticesArr[5] = 0.0f;
    verticesArr[6] = currX + txtGen->atlasCellWidth*glyphResizeFactor;
    verticesArr[7] = height;
    verticesArr[8] = 0.0f;
    verticesArr[9] = currX;
    verticesArr[10] = height;
    verticesArr[11] = 0.0f;
}

inline void OpenGlTxtGen::TxtGraphicsComponent::fillColorsArr(float* colorsArr) {
    for (unsigned int vertexIdx = 0; vertexIdx < 4; vertexIdx++) {
        unsigned int baseVertexIdx = vertexIdx * 4;
        colorsArr[baseVertexIdx    ] = color[0];
        colorsArr[baseVertexIdx + 1] = color[1];
        colorsArr[baseVertexIdx + 2] = color[2];
        colorsArr[baseVertexIdx + 3] = color[3];
    }
}

inline void OpenGlTxtGen::TxtGraphicsComponent::fillIndicesArr(unsigned int* indicesArr, unsigned int verticesBaseIdx) {
    //memcpy(indices + charIdx*6, unsigned int[]({0, 1, 2, 1, 2, 0}), 6*sizeof(unsigned int));
    indicesArr[0] = verticesBaseIdx;
    indicesArr[1] = verticesBaseIdx + 1;
    indicesArr[2] = verticesBaseIdx + 2;
    indicesArr[3] = verticesBaseIdx;
    indicesArr[4] = verticesBaseIdx + 2;
    indicesArr[5] = verticesBaseIdx + 3;
}

inline void OpenGlTxtGen::TxtGraphicsComponent::fillUVsArr(float *uvsArr, int glyphIdx) {
    float u = glyphIdx % txtGen->atlasHorizonCellsNr * txtGen->atlasCellUVWidth;
    float v = glyphIdx / txtGen->atlasHorizonCellsNr * txtGen->atlasCellUVHeight;
    float u2 = u + txtGen->atlasCellUVWidth ;
    float v2 = v + txtGen->atlasCellUVHeight;
    uvsArr[0] = u;
    uvsArr[1] = v2;
    uvsArr[2] = u2;
    uvsArr[3] = v2;
    uvsArr[4] = u2;
    uvsArr[5] = v;
    uvsArr[6] = u;
    uvsArr[7] = v;
}

inline bool OpenGlTxtGen::TxtGraphicsComponent::setupOpenGlLmnts(float* verticesArr, float* colorsArr, unsigned int* indicesArr, float* uvsArr, unsigned int txtLen) {
    GLuint buffer[4];
    glGenBuffers(4, buffer);
    CHECK_GL_ERROR("glGenBuffers");
    glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
    CHECK_GL_ERROR("glBindBuffer");
    glBufferData(GL_ARRAY_BUFFER, 12*txtLen*sizeof(float), verticesArr, GL_STATIC_DRAW);
    CHECK_GL_ERROR("glBufferData");
    model.vertexBuffer = buffer[0];

    glBindBuffer(GL_ARRAY_BUFFER, buffer[1]);
    CHECK_GL_ERROR("glBindBuffer");
    glBufferData(GL_ARRAY_BUFFER, 16*txtLen*sizeof(float), colorsArr, GL_STATIC_DRAW);
    CHECK_GL_ERROR("glBufferData");
    model.vertexColorBuffer = buffer[1];

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer[2]);
    CHECK_GL_ERROR("glBindBuffer");
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*txtLen*sizeof(unsigned int), indicesArr, GL_STATIC_DRAW);
    CHECK_GL_ERROR("glBufferData");
    model.vertexIndicesBuffer = buffer[2];
    model.facesNr = 2*(GLsizei)txtLen;

    glBindBuffer(GL_ARRAY_BUFFER, buffer[3]);
    CHECK_GL_ERROR("glBindBuffer");
    glBufferData(GL_ARRAY_BUFFER, 8*txtLen*sizeof(float), uvsArr, GL_STATIC_DRAW);
    CHECK_GL_ERROR("glBufferData");
    model.uvsBuffer = buffer[3];

    GLuint prog = createGlProg(vertexShader.c_str(), fragShader.c_str());
    if (!prog) {
        return false;
    }
    model.prog = prog;
    model.vertexPosAttribLoc = (GLuint)glGetAttribLocation(prog, VERTEX_POS_ATTRIB_NAME);
    CHECK_GL_ERROR("glGetAttribLocation");
    model.vertexColorAttribLoc = (GLuint)glGetAttribLocation(prog, VERTEX_COLOR_ATTRIB_NAME);
    CHECK_GL_ERROR("glGetAttribLocation");
    model.uvsAttribLoc = (GLuint)glGetAttribLocation(prog, TEX_COORDS_ATTRIB_NAME);
    CHECK_GL_ERROR("glGetAttribLocation");
    model.mvpMatUnifLoc = (GLuint)glGetUniformLocation(prog, MVP_MAT_UNIF_NAME);
    CHECK_GL_ERROR("glGetAttribLocation");
    model.texSamplerUnifLoc = (GLuint)glGetUniformLocation(prog, "texSampler");
    CHECK_GL_ERROR("glGetAttribLocation");

    glGenVertexArrays(1, &model.vao);
    CHECK_GL_ERROR("glGenVertexArrays");
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glEnableVertexAttribArray(model.vertexPosAttribLoc);
    glVertexAttribPointer(model.vertexPosAttribLoc, 3, GL_FLOAT, 0, 0, 0);
    CHECK_GL_ERROR("glVertexAttribPointer");
    glBindBuffer(GL_ARRAY_BUFFER, model.vertexColorBuffer);
    glEnableVertexAttribArray(model.vertexColorAttribLoc);
    glVertexAttribPointer(model.vertexColorAttribLoc, 4, GL_FLOAT, 0, 0, 0);
    CHECK_GL_ERROR("glVertexAttribPointer");
    glBindBuffer(GL_ARRAY_BUFFER, model.uvsBuffer);
    glEnableVertexAttribArray(model.uvsAttribLoc);
    glVertexAttribPointer(model.uvsAttribLoc, 2, GL_FLOAT, 0, 0, 0);
    CHECK_GL_ERROR("glVertexAttribPointer");
    glEnableVertexAttribArray(model.mvpMatUnifLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    return true;
}

bool OpenGlTxtGen::TxtGraphicsComponent::initOpenGlLmnts() {
#if DEBUG
    if (!txtGen->isInited) {
        ServiceLocator::getLogger().loge("TxtGraphicsComponentImpl", "Tried to initOpenGlLmnts while TxtGen was not inited.");
        return false;
    }
#endif
    glyphResizeFactor = height / txtGen->atlasCellHeight;
    unsigned int txtLen = (unsigned int)txt.length();
    float* vertices = new float[12*txtLen];
    float* colors = new float[16*txtLen];
    unsigned int* indices= new unsigned int[6*txtLen];
    float* uvs = new float[8*txtLen];
    float currX = 0;
    for (unsigned int charIdx = 0; charIdx < txtLen; charIdx++) {
        int glyphIdx = txtGen->convertCharToIndex(txt[charIdx]);
        if (!glyphIdx) {
            currX += txtGen->spaceSz*glyphResizeFactor;
            continue;
        }
        fillVerticesArr(vertices + charIdx*12, currX);
        fillColorsArr(colors + charIdx*16);
        fillIndicesArr(indices + charIdx*6, charIdx*4);
        fillUVsArr(uvs + charIdx*8, glyphIdx);
        currX += txtGen->glyphsSzs[glyphIdx]*glyphResizeFactor;
    }
    width = currX;
    charsNrToRender = (unsigned int)txt.length();

    bool res = setupOpenGlLmnts(vertices, colors, indices, uvs, txtLen);
	
	delete[] vertices;
	delete[] colors;
	delete[] indices;
	delete[] uvs;	
	return res;
}

void OpenGlTxtGen::TxtGraphicsComponent::destroyOpenGlLmnts() {
    glDeleteBuffers(1, &model.uvsBuffer);
    glDeleteBuffers(1, &model.vertexIndicesBuffer);
    glDeleteBuffers(1, &model.vertexColorBuffer);
    glDeleteBuffers(1, &model.vertexBuffer);
    glDeleteVertexArrays(1, &model.vao);
}

bool OpenGlTxtGen::TxtGraphicsComponent::render(double lag, const glm::mat4& vpMat) {
    glUseProgram(model.prog);
    glBindVertexArray(model.vao);
    glUniformMatrix4fv(model.mvpMatUnifLoc, 1, GL_FALSE, glm::value_ptr(vpMat * transformMat));
    CHECK_GL_ERROR("glUniformMatrix4fv");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, txtGen->atlasTex);
    CHECK_GL_ERROR("glBindTexture");
    glUniform1i(model.texSamplerUnifLoc, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.vertexIndicesBuffer);
    glDrawElements(GL_TRIANGLES, (GLsizei)charsNrToRender*6, GL_UNSIGNED_INT, 0);
    CHECK_GL_ERROR("glDrawElements");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    return true;
}

OpenGlTxtGen::NumericalTxtGraphicsComponent::NumericalTxtGraphicsComponent(OpenGlTxtGen* _txtGen, unsigned int _number, float height, float color[4], unsigned int _digitsNrMax) :
        TxtGraphicsComponent(_txtGen, "", height, color), digitsNrMax(_digitsNrMax), number(_number), digits(new unsigned int[_digitsNrMax]), digtisBotLeftCoords(new float[_digitsNrMax])  {}

bool OpenGlTxtGen::NumericalTxtGraphicsComponent::initOpenGlLmnts() {
#if DEBUG
    if (!txtGen->isInited) {
        ServiceLocator::getLogger().loge("NumericalTxtGraphicsComponentImpl", "Tried to initOpenGlLmnts while TxtGen was not initiated.");
        return false;
    }
#endif
    glyphResizeFactor = height / txtGen->atlasCellHeight;
    unsigned int remainder = number;
    unsigned int digitIdx = 0;
    do {
        digits[digitIdx++] = remainder % 10;
        remainder /= 10;
    } while (remainder > 0 && digitIdx < digitsNrMax);
    charsNrToRender = digitIdx;

    float* vertices = new float[12*charsNrToRender];
    float* colors = new float[16*digitsNrMax];
    unsigned int* indices = new unsigned int[6*digitsNrMax];
    float* uvs = new float[8*charsNrToRender];
    float currX = 0;
    for (digitIdx = 0; digitIdx < charsNrToRender; digitIdx++) {
        fillVerticesArr(vertices + digitIdx*12, currX);
        fillColorsArr(colors + digitIdx*16);
        fillIndicesArr(indices + digitIdx*6, digitIdx*4);
        unsigned int digitUVIdx = txtGen->glyphsIdxsMap.zeroIdx + digits[charsNrToRender - digitIdx - 1];
        fillUVsArr(uvs + digitIdx*8, digitUVIdx);
        digtisBotLeftCoords[digitIdx] = currX;
        currX += txtGen->glyphsSzs[digitUVIdx]*glyphResizeFactor;
    }
    for (; digitIdx < digitsNrMax; digitIdx++) {
        fillColorsArr(colors + digitIdx*16);
        fillIndicesArr(indices + digitIdx*6, digitIdx*4);
    }

    bool res = setupOpenGlLmnts(vertices, colors, indices, uvs, digitsNrMax);

	delete[] vertices;
	delete[] colors;
	delete[] indices;
	delete[] uvs;
	return res;
}

/*void OpenGlTxtGen::NumericalTxtGraphicsComponent::destroyOpenGlLmnts() {
    glDeleteBuffers(1, &model.uvsBuffer);
    glDeleteBuffers(1, &model.vertexIndicesBuffer);
    glDeleteBuffers(1, &model.vertexColorBuffer);
    glDeleteBuffers(1, &model.vertexBuffer);
}*/
//OpenGlTxtGen::NumericalTxtGraphicsComponent&
bool OpenGlTxtGen::NumericalTxtGraphicsComponent::operator=(unsigned int newNumber) {	
    unsigned int remainder = newNumber;
    unsigned int digitIdx = 0;
    unsigned int digitsNrToUpdate = 0;
    do {
        unsigned int digit = remainder % 10;
        if (digitIdx >= charsNrToRender || digits[digitIdx] != digit) {
            digits[digitIdx] = digit;
            digitsNrToUpdate = digitIdx + 1;
        }
        digitIdx++;
        remainder /= 10;
    } while (remainder > 0 && digitIdx < digitsNrMax);
    number = newNumber;
    charsNrToRender = digitIdx;
	
    float* vertices = new float[12*digitsNrToUpdate];
    float* uvs = new float[8*digitsNrToUpdate];
    unsigned int digitsNrUnchanged = charsNrToRender - digitsNrToUpdate;
    float currX = digtisBotLeftCoords[digitsNrUnchanged];
    for (digitIdx = 0; digitIdx < digitsNrToUpdate; digitIdx++) {
        fillVerticesArr(vertices + digitIdx*12, currX);
        unsigned int digitUVIdx = txtGen->glyphsIdxsMap.zeroIdx + digits[charsNrToRender - digitIdx - digitsNrUnchanged - 1];
        fillUVsArr(uvs + digitIdx*8, digitUVIdx);
        digtisBotLeftCoords[digitsNrUnchanged + digitIdx] = currX;
        currX += txtGen->glyphsSzs[digitUVIdx]*glyphResizeFactor;
    }
    width = currX;

    glBindBuffer(GL_ARRAY_BUFFER, model.vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, digitsNrUnchanged*12*sizeof(float), digitsNrToUpdate*12*sizeof(float), vertices);
    CHECK_GL_ERROR("glBufferSubData");
    glBindBuffer(GL_ARRAY_BUFFER, model.uvsBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, digitsNrUnchanged*8*sizeof(float), digitsNrToUpdate*8*sizeof(float), uvs);
    CHECK_GL_ERROR("glBufferSubData");
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //return *this;
	delete[] vertices;
	delete[] uvs;
    return true;
}
