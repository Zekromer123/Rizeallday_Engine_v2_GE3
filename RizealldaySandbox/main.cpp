#include "EditorApp.h"
#include <Windows.h>
#include <memory>

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	auto app = std::make_unique<EditorApp>();
	app->Run();
	return EditorApp::GetExitCode();
}
