#include "DxUtils.h"

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <debugapi.h>

HRESULT CompileShaderFromFile(const WCHAR* fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3D10Blob** blobOut) {
	ID3D10Blob* errBlob;
	HRESULT hr = D3DCompileFromFile(fileName, NULL, NULL, entryPoint, shaderModel, D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, NULL, blobOut, &errBlob);
	if (FAILED(hr) && errBlob != NULL)		
		OutputDebugStringA((char*)errBlob->GetBufferPointer());					
		
	SAFE_RELEASE(errBlob);
	return hr;
}