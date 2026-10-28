#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <functional>

class WinApp
{
public:
	static constexpr int32_t kClientWidth = 1280;
	static constexpr int32_t kClientHeight = 720;

	static WinApp* GetInstance();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	/// Create game window
	void CreateGameWindow(const char* title = "Rizeallday Engine", int width = kClientWidth, int height = kClientHeight);

	/// Terminate game window
	void TerminateGameWindow();

	/// Process Windows messages (returns true when WM_QUIT received)
	bool ProcessMessage();

	/// Called on WM_CLOSE (window X button, Alt+F4). Return true to close now, false to keep the window
	/// open (e.g. to ask about unsaved changes first). Pass nullptr to always close.
	void SetCloseRequestHandler(std::function<bool()> handler) { closeRequestHandler_ = std::move(handler); }
	/// Same decision as WM_CLOSE, usable from menus: quits when the handler allows it
	void RequestClose();

	HWND GetHWND() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return wc_.hInstance; }
	int GetClientWidth() const;
	int GetClientHeight() const;

private:
	WinApp() = default;
	~WinApp() = default;
	WinApp(const WinApp&) = delete;
	WinApp& operator=(const WinApp&) = delete;

	HWND hwnd_ = nullptr;
	WNDCLASS wc_ = {};
	std::function<bool()> closeRequestHandler_;
};
