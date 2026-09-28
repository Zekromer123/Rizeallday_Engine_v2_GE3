#include "base/WinApp.h"
#include "base/ConvertString.h"

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
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::CreateGameWindow(const char* title, int width, int height)
{
	// COMの初期化
	CoInitializeEx(0,COINIT_MULTITHREADED);

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

	ShowWindow(hwnd_, SW_SHOW);
}

void WinApp::TerminateGameWindow()
{
	CoUninitialize();
}

bool WinApp::ProcessMessage()
{
	MSG msg{};
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message == WM_QUIT)
	{
		return true;  // true = quit
	}
	return false;  // false = keep going
}