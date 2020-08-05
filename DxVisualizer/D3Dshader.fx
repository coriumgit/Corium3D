//--------------------------------------------------------------------------------------
// File: D3DVisualization.fx
// Originally from DirectX SDK - Tutorial 4 sample
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
#define INSTANCES_NR_MAX 100

cbuffer cbPerFrame : register(b0)
{
    matrix viewMat;
    matrix projMat;
}

//cbuffer cbPerObj : register(b1)
//{    
//    matrix instancesTransformats[INSTANCES_NR_MAX];
//}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, 
             float4 Color : COLOR)//, 
             //uint instanceIdx : SV_InstanceID)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = Pos;
    
    //output.Pos = mul(Pos, instancesTransformats[instanceIdx]);
    output.Pos = mul(output.Pos, viewMat);
    output.Pos = mul(output.Pos, projMat);
    output.Color = Color;

    return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    return input.Color;
}
