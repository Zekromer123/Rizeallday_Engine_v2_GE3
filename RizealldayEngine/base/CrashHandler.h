#pragma once

#include <windows.h>

class CrashHandler {
public:
	static CrashHandler* GetInstance() { static CrashHandler instance; return &instance; }
public:
	static void Initialize();
	~CrashHandler();

private:
	static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

};


