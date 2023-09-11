#include "GameMaster.h"
#include "Corium3D.h"

#include <iostream>
#include <Windowsx.h>
#include <string>

using namespace Corium3D;

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void createGameMaster();
Corium3DEngine::Corium3DEngineOnlineCallback corium3DOnlineCallback = createGameMaster;

const unsigned int WINDOW_WIDTH = 1024;
const unsigned int WINDOW_HEIGHT = 800;

unsigned int clientAreaWidth;
unsigned int clientAreaHeight;
GameMaster* gameMaster;
Corium3DEngine* corium3DEngine;
Corium3D::KeyboardInputID keyboardInputCodesMap[256];
std::function<void(Corium3D::KeyboardInputID inputID)> systemKeyboardInputStartCallback;
std::function<void(Corium3D::KeyboardInputID inputID)> systemKeyboardInputEndCallback;
std::function<void(Corium3D::CursorInputID inputID, glm::vec2 const& cursorPos)> systemCursorInputCallback;
std::function<void(glm::vec2 const& cursorPos)> systemCursorMoveCallbackPtr;
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
	const wchar_t title[] = L"Corium3D";

	WNDCLASS wc = {};

	wc.lpfnWndProc = windowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = title;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	if (!RegisterClass(&wc)) {
		std::cout << "RegisterClass Failed:" << GetLastError() << std::endl;
		return false;
	}
	
	keyboardInputCodesMap[VK_LEFT] = Corium3D::KeyboardInputID::LEFT_ARROW;
	keyboardInputCodesMap[VK_RIGHT] = Corium3D::KeyboardInputID::RIGHT_ARROW;
	keyboardInputCodesMap[VK_UP] = Corium3D::KeyboardInputID::UP_ARROW;
	keyboardInputCodesMap[VK_DOWN] = Corium3D::KeyboardInputID::DOWN_ARROW;	
	keyboardInputCodesMap[0x41] = Corium3D::KeyboardInputID::A;
	keyboardInputCodesMap[0x47] = Corium3D::KeyboardInputID::G;
	keyboardInputCodesMap[0x51] = Corium3D::KeyboardInputID::Q;	
	keyboardInputCodesMap[VK_SPACE] = Corium3D::KeyboardInputID::SPACE;

	//const char* modelsPaths[3] = { "../assets/cube.dae", "../assets/sphere.dae", "../assets/capsule.dae" }; // "../assets/simple_armature.dae"
	Corium3DEngine::CallbackPtrs callbackPtrs = { corium3DOnlineCallback , systemKeyboardInputStartCallback, systemKeyboardInputEndCallback, systemCursorInputCallback };
	const char* shadersFullPaths[2] = {"../assets/bonelessVertexShader.vs", "../assets/fragShader.fs"};
	Corium3DEngine::AssetsFilesFullPaths assetsFilesFullPaths = { "../assets/SceneB.assets", 
																  "../assets/txtTexAtlas.png",																  
															  	  &shadersFullPaths[0], &shadersFullPaths[1], 1};	
	corium3DEngine = new Corium3DEngine(callbackPtrs, assetsFilesFullPaths);
	
	HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, title, title, WS_OVERLAPPEDWINDOW,
							   CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);

	//nCmdShow
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);	

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	delete gameMaster;
	delete corium3DEngine;
	return 0;
}

inline glm::vec2 extractWindowViewportCoords(LPARAM lparam) {
	return glm::vec2(GET_X_LPARAM(lparam), clientAreaHeight - GET_Y_LPARAM(lparam));
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
		case WM_CREATE:
			corium3DEngine->signalSurfaceCreated(hwnd);
			return 0;

		case WM_SHOWWINDOW:
			if (wparam)
				corium3DEngine->signalResume();
			else
				corium3DEngine->signalPause();

			return 0;

		case WM_SETFOCUS:

			corium3DEngine->signalWindowFocusChanged(true);
			return 0;

		case WM_KILLFOCUS:
			corium3DEngine->signalWindowFocusChanged(false);
			return 0;

		case WM_SIZE:
			if (wparam == SIZE_MINIMIZED || wparam == SIZE_MAXHIDE)
				corium3DEngine->signalPause();
			else {
				corium3DEngine->signalResume();
				clientAreaWidth = LOWORD(lparam);
				clientAreaHeight = HIWORD(lparam);
				corium3DEngine->signalSurfaceSzChanged(clientAreaWidth, clientAreaHeight);
				return 0;
			}

		case WM_DESTROY:
			corium3DEngine->signalSurfaceDestroyed();
			corium3DEngine->signalDetachedFromWindow();
			PostQuitMessage(0);
			return 0;

		case WM_LBUTTONDOWN: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(CursorInputID::LEFT_DOWN, cursorPos);
			return 0;
		}

		case WM_LBUTTONUP: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(CursorInputID::LEFT_UP, cursorPos);
			return 0;
		}
		
		case WM_RBUTTONDOWN: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(CursorInputID::RIGHT_DOWN, cursorPos);
			return 0;
		}

		case WM_RBUTTONUP: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(CursorInputID::RIGHT_UP, cursorPos);
			return 0;
		}

		case WM_MBUTTONDOWN: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(Corium3D::CursorInputID::MIDDLE_DOWN, cursorPos);
			return 0;
		}

		case WM_MBUTTONUP: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(Corium3D::CursorInputID::MIDDLE_UP, cursorPos);
			return 0;
		}

		case WM_MOUSEWHEEL: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
			// TODO: Figure why cursorPos comes out too large sometimes for this event
			if (cursorPos.x > clientAreaWidth)
				cursorPos.x = clientAreaWidth;
			if (cursorPos.y > clientAreaHeight)
				cursorPos.y = clientAreaHeight;

			if (wheelDelta > 0)
				systemCursorInputCallback(Corium3D::CursorInputID::WHEEL_ROTATED_FORWARD, cursorPos);
			else
				systemCursorInputCallback(Corium3D::CursorInputID::WHEEL_ROTATED_BACKWARD, cursorPos);
			return 0;

		}	

		case WM_MOUSEMOVE: {
			glm::vec2 cursorPos(extractWindowViewportCoords(lparam));
			systemCursorInputCallback(Corium3D::CursorInputID::MOVE, cursorPos);
			return 0;
		}

		case WM_KEYDOWN :
			systemKeyboardInputStartCallback(keyboardInputCodesMap[wparam]);
			return 0;	

		case WM_KEYUP:
			systemKeyboardInputEndCallback(keyboardInputCodesMap[wparam]);
			return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void createGameMaster() {
	gameMaster = new GameMaster(*corium3DEngine);
}