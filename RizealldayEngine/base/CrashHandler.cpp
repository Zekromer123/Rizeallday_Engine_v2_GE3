#include "base/CrashHandler.h"
#include <dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "Dbghelp.lib")

void CrashHandler::Initialize()
{
	SetUnhandledExceptionFilter(CrashHandler::ExportDump);
}

CrashHandler::~CrashHandler()
{
}

LONG __stdcall CrashHandler::ExportDump(EXCEPTION_POINTERS* exception)
{
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);//File Name
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	DWORD proccessId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	MiniDumpWriteDump(GetCurrentProcess(), proccessId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);


	return EXCEPTION_EXECUTE_HANDLER;
}

