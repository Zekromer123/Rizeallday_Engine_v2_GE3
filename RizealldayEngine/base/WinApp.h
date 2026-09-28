#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#ifdef USE_IMGUI
#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#endif // USE_IMGUI

class WinApp
{
public:
	static WinApp* GetInstance();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/// ゲームウィンドウの作成
	void CreateGameWindow(const char* title = "Rizeallday Engine", int width = 1280, int height = 720);

	/// ゲームウィンドウの破棄
	void TerminateGameWindow();

	/// メッセージ処理 (true = WM_QUIT received)
	bool ProcessMessage();

	HWND GetHWND() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return wc_.hInstance; }

private:
	WinApp() = default;
	~WinApp() = default;
	WinApp(const WinApp&) = delete;
	WinApp& operator=(const WinApp&) = delete;

	HWND hwnd_ = nullptr;
	WNDCLASS wc_ = {};
};
