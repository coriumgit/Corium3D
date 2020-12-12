#version 430 core

in uint aInstanceDataIdx;
uniform mat4 uVpMat;
in vec4 aPos;
uniform uint uBaseVertex;  // TODO: simplify this shit

layout (std430, binding = 4) buffer TransformatsBuffer { 
	mat4 uTransformats[]; 
};  
layout (std430, binding = 7) buffer SelectedColorsIdxsBuffer { 
	uint uSelectedColorsIdxs[]; 
}; 
layout (std430, binding = 8) buffer ColorsBuffer {
	vec4 uColors[]; 
}; 

out vec4 passColor; 

void main(void) { 
//	passColor = uColors[uSelectedColorsIdxs[aInstanceDataIdx] + gl_VertexID - uBaseVertex]; //	passColor = uColors[gl_VertexID]; 
//  passColor = vec4(aInstanceDataIdx/2.0f, aInstanceDataIdx/2.0f, aInstanceDataIdx/2.0f, 1.0f);
	passColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	gl_Position = uVpMat * uTransformats[aInstanceDataIdx] * aPos;  //*** uVpMat 
};