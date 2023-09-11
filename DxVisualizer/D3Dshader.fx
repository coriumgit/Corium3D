//--------------------------------------------------------------------------------------
// File: D3DVisualization.fx
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
#define INSTANCES_NR_MAX 100
#define BLUR_SPAN 15
#define HIERS_NR_MAX 100

cbuffer cbViewMat : register(b0)
{
    matrix viewMat;    
}

cbuffer cbProjMat : register(b1)
{
    matrix projMat;
}

cbuffer cbModelID : register(b2)
{
    uint modelID;
}

cbuffer cbGlobalTransformat : register(b3)
{
    matrix globalTransformat;
}

cbuffer cbBlur : register(b4)
{
    float4 offsets[BLUR_SPAN];
    float4 weights[BLUR_SPAN];
}

/*
cbuffer cbParentsData : register(b4)
{
    matrix parentsTransformats[HIERS_NR_MAX];    
}
*/
Texture2D texDiffuse : register(t0);
SamplerState sampState : register(s0);

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    uint2 sceneModelInstanceIdx : INDICES;
};

struct PS_OUTPUT
{
    float4 color0 : SV_Target0;
    uint2 sceneModelInstanceIdx : SV_Target1;
    float4 color2 : SV_Target2;
};

struct VS_OUTPUT_OUTLINE
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// Scene Shaders
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, 
             float4 Color : COLOR,              
             float4 InstanceTransformVec0 : INSTANCE_TRANSFORMAT1,
             float4 InstanceTransformVec1 : INSTANCE_TRANSFORMAT2,
             float4 InstanceTransformVec2 : INSTANCE_TRANSFORMAT3,
             float4 InstanceTransformVec3 : INSTANCE_TRANSFORMAT4,
             //int parentIdx: PARENT_IDX,
             uint instanceIdx : INSTANCE_IDX,
             float4 InstanceColor : INSTANCE_COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    
    matrix modelTransformat = matrix(InstanceTransformVec0, InstanceTransformVec1, InstanceTransformVec2, InstanceTransformVec3);        
    output.Pos = mul(Pos, modelTransformat);
    output.Pos = mul(output.Pos, globalTransformat);
    //if (hierIdx > -1)
    //    output.Pos = mul(output.Pos, parentsTransformats[parentIdx]);
    output.Pos = mul(output.Pos, viewMat);
    output.Pos = mul(output.Pos, projMat);
    //float3 modulatedColor = Color.rgb * (1.0f - InstanceColor.a) + InstanceColor.rgb * InstanceColor.a;
    output.Color = float4(Color.rgb * (1.0f - InstanceColor.a) + InstanceColor.rgb * InstanceColor.a, Color.a);
    output.sceneModelInstanceIdx = uint2(modelID, instanceIdx);

    return output;
}

PS_OUTPUT PS(VS_OUTPUT input) : SV_Target
{
    PS_OUTPUT output;

    output.color0 = input.Color;
    output.color2 = input.Color;
    output.sceneModelInstanceIdx.x = input.sceneModelInstanceIdx.x;
    output.sceneModelInstanceIdx = input.sceneModelInstanceIdx;

    return output;
}

//--------------------------------------------------------------------------------------
// Blur Shaders
//--------------------------------------------------------------------------------------
VS_OUTPUT_OUTLINE VSBlur(float4 pos : POSITION, float2 texCoord : TEXCOORD)
{
    VS_OUTPUT_OUTLINE output;
    output.pos = pos;
    output.texCoord = texCoord;

    return output;
}

float4 PSBlur(VS_OUTPUT_OUTLINE input) : SV_Target
{   
    //float4 vColor = 0.0f;
    float alpha = 0.0;

    //[unroll]
    for (int sampleIdx = 0; sampleIdx < BLUR_SPAN; sampleIdx++) {
        alpha += weights[sampleIdx].a * texDiffuse.Sample(sampState, input.texCoord + offsets[sampleIdx]).a;
        //vColor += weights[sampleIdx] * sampledTexel;
    }   

    return float4(1.0f, 0.65, 0.0f, alpha);
}

//--------------------------------------------------------------------------------------
// Outline Shaders
//--------------------------------------------------------------------------------------
VS_OUTPUT_OUTLINE VSOutline(float4 pos : POSITION, float2 texCoord : TEXCOORD)
{
    VS_OUTPUT_OUTLINE output;
    output.pos = pos;
    output.texCoord = texCoord;

    return output;
}

float4 PSOutline(VS_OUTPUT_OUTLINE input) : SV_Target
{
    float4 output = texDiffuse.Sample(sampState, input.texCoord);

    // alpha test
    //if (output.a == 0.0f)
    //    discard;    

    // Assign Glow Color
    output.r = 1.0f;
    output.g = 0.65f;
    output.b = 0.0;

    return output;
}