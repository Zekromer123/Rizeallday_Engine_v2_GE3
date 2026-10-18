#include "base/WinApp.h"
#include "base/ConvertString.h"
#include "base/DirectXCommon.h"

WinApp* WinApp::GetInstance()
{
	static WinApp instance;
	return &instance;
}

//Window procedure function declaration
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam))
	{
		return true;
	}
#endif

	switch (msg) {
	case WM_SIZE:
		if (wparam != SIZE_MINIMIZED) {
			uint32_t w = LOWORD(lparam);
			uint32_t h = HIWORD(lparam);
			if (w > 0 && h > 0) {
				DirectXCommon* dx = DirectXCommon::GetInstance();
				if (dx && dx->GetDevice()) {
					dx->ResizeBuffers(w, h);
				}
			}
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::CreateGameWindow(const char* title, int width, int height)
{
	// Set Per-Monitor DPI Awareness v2 so Windows scaling does not blur or distort mouse hit-testing
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Initialize COM
	CoInitializeEx(0, COINIT_MULTITHREADED);

	wc_.lpfnWndProc = WindowProc;
	wc_.lpszClassName = L"RizealldayEngine";
	wc_.hInstance = GetModuleHandle(nullptr);
	wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc_);

	RECT wrc = { 0, 0, width, height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

	ConvertStrings converter;
	std::string titleName = title;
	std::wstring convertTitle = converter.ConvertString(titleName);
	hwnd_ = CreateWindow(
		wc_.lpszClassName,
		convertTitle.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc_.hInstance,
		nullptr
	);

	// Show window in maximized state for full editor workspace
	ShowWindow(hwnd_, SW_MAXIMIZE);
	UpdateWindow(hwnd_);
}

void WinApp::TerminateGameWindow()
{
	CoUninitialize();
}

bool WinApp::ProcessMessage()
{
	MSG msg{};
	// Process all queued messages every frame for smooth input and resizing
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
		{
			return true;  // true = quit
		}
	}
	return false;  // false = keep going
}

int WinApp::GetClientWidth() const
{
	if (!hwnd_) return 1280;
	RECT rc{};
	GetClientRect(hwnd_, &rc);
	int w = rc.right - rc.left;
	return w > 0 ? w : 1280;
}

int WinApp::GetClientHeight() const
{
	if (!hwnd_) return 720;
	RECT rc{};
	GetClientRect(hwnd_, &rc);
	int h = rc.bottom - rc.top;
	return h > 0 ? h : 720;
}