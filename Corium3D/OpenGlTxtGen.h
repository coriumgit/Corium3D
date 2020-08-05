//
// Created by omer on 16/02/02018.
//

#pragma once

//TODO: use rvalues (maybe unsigned int (&&glyphsSzs)[] for example)

#include <string.h>

class OpenGlTxtGen {
public:
    typedef struct {
        unsigned int zeroIdx;
        unsigned int captialAIdx;
        unsigned int smallAIdx;
        unsigned int exclamationMarkIdx;
        unsigned int period;
    } GlyphsIdxsMap;

    OpenGlTxtGen(const char* txtAtlasPath, unsigned int atlasHorizonCellsNr, unsigned int atlasVertCellsNr, unsigned int spaceSz,
                 unsigned int* glyphsSzs, GlyphsIdxsMap& glyphsIdxsMap);
    ~OpenGlTxtGen();
    bool initOpenGlLmnts();
    void destroyOpenGlLmnts();    

private:
    std::string txtAtlasPath;
    GLuint atlasTex;
    unsigned int atlasHorizonCellsNr;
    unsigned int atlasVertCellsNr;
    float atlasCellWidth;
    float atlasCellHeight;
    float atlasCellUVWidth;
    float atlasCellUVHeight;
    size_t spaceSz;
    unsigned int* glyphsSzs;
    GlyphsIdxsMap glyphsIdxsMap;
#if DEBUG
    bool isInited = false;
#endif

    unsigned int convertCharToIndex(char c);
};


/*class OpenGlTxtGen::TxtGraphicsComponent : public GraphicsComponent {
public:
    bool initOpenGlLmnts() override;
    void destroyOpenGlLmnts() override;
    bool render(double lag, const glm::mat4& vpMat) override;
    float getWidth();
    float getHeight();

protected:
    friend class OpenGlTxtGen;
    OpenGlTxtGen* txtGen;
    std::string txt;
    float height = 0.0f;
    float glyphResizeFactor;
    float width = 0.0f;
    float color[4];
    unsigned int charsNrToRender;
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

    TxtGraphicsComponent(OpenGlTxtGen* txtGen, const char* txt, float height, float color[4]);
    void fillVerticesArr(float* verticesArr, float currX);
    inline void fillColorsArr(float* colorsArr);
    inline void fillIndicesArr(unsigned int* indicesArr, unsigned int verticesBaseIdx);
    inline void fillUVsArr(float* uvsArr, int glyphIdx);
    inline bool setupOpenGlLmnts(float* verticesArr, float* colorsArr, unsigned int* indicesArr, float* uvsArr, unsigned int txtLen);
};

class OpenGlTxtGen::NumericalTxtGraphicsComponent : public TxtGraphicsComponent {
public:
    bool initOpenGlLmnts() override;
    //NumericalTxtGraphicsComponent&
    bool operator=(unsigned int newNumber);

private:
    friend class OpenGlTxtGen;
    unsigned int digitsNrMax;
    unsigned int number;
    unsigned int* digits;
    float* digtisBotLeftCoords;

    NumericalTxtGraphicsComponent(OpenGlTxtGen* txtGen, unsigned int number, float height, float color[4], unsigned int digitsNrMax);
};*/

