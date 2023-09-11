#version 430 core

in uint aInstanceDataIdx;
in vec4 aPos; 	
in ivec4 aBonesIDs; 
in vec4 aBonesWeights; 
uniform mat4 uMeshTransform; 

//layout (std430, binding = 3) buffer MmvpsBaseIdxsBuffer { 
//	uint uMmvpsBaseIdx[]; 
//}; 
layout (std430, binding = 4) buffer MvpsBuffer { 
	mat4 uMVPs[]; 
};  
layout (std430, binding = 5) buffer BonesTransformsBaseIdxsBuffer { 
	uint uBonesTransformsBaseIdxs[]; 
}; 
layout (std430, binding = 6) buffer BonesTransformsBuffer { 
	mat4 uBonesTransforms[]; 
}; 	
layout (std430, binding = 7) buffer SelectedColorsIdxsBuffer { 
	uint uSelectedColorsIdxs[]; 
}; 
layout (std430, binding = 8) buffer ColorsBuffer {  
	vec4 uColors[]; 
}; 		

out vec4 passColor; 	

void main(void) { 	
	passColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
   //passColor = vec4(aBonesWeights[0], aBonesWeights[1], aBonesWeights[2], 1.0f);  
	mat4 boneTransform = mat4(1.0f); 
	if (aBonesWeights[0] != 0 || aBonesWeights[1] != 0 || aBonesWeights[2] != 0 || aBonesWeights[3] != 0) { 
		uint bonesTransformsBaseIdx = uBonesTransformsBaseIdxs[aInstanceDataIdx]; 
		boneTransform = uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[0]]*aBonesWeights[0] + 
						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[1]]*aBonesWeights[1] + 
						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[2]]*aBonesWeights[2] + 
						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[3]]*aBonesWeights[3]; 
	} 

   gl_Position = uMVPs[aInstanceDataIdx] * uMeshTransform * boneTransform * aPos; 	
};