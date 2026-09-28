#pragma once
#include <Windows.h>
#include <dxcapi.h>
#include <cassert>
#include <string>
#include <wrl.h>
#pragma comment(lib, "dxcompiler.lib")
class DXCManager
{
public:
	void Initialize();
	IDxcBlob* CompileShader(
		const std::wstring& filePath,
		const wchar_t* profile,
		IDxcUtils* dxcUtils,
		IDxcCompiler3* dxcCompiler,
		IDxcIncludeHandler* includeHandler);
	IDxcUtils* GetDxcUtils() const { return dxcUtils.Get(); }
	IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() const { return includeHandler.Get(); }



private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
};

