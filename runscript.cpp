#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <functional>
#include <Windows.h>
#include <mbstring.h>

void exec(const std::wstring & cwd, const std::wstring & cmd, std::function<void(const char *data, DWORD size)> callback) {

	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	HANDLE stdoutPipeRead = NULL;
	HANDLE stdoutPipeWrite = NULL;
	if (!CreatePipe(&stdoutPipeRead, &stdoutPipeWrite, &saAttr, 0)) {
		OutputDebugString(TEXT("StdoutRd CreatePipe"));
		exit(0);
	}

	if (!SetHandleInformation(stdoutPipeRead, HANDLE_FLAG_INHERIT, 0)) {
		OutputDebugString(TEXT("Stdout SetHandleInformation"));
		exit(0);
	}

	HANDLE stdinPipeRead = NULL;
	HANDLE stdinPipeWrite = NULL;
	if (!CreatePipe(&stdinPipeRead, &stdinPipeWrite, &saAttr, 0)) {
		OutputDebugString(TEXT("Stdin CreatePipe"));
		exit(0);
	}

	if (!SetHandleInformation(stdinPipeWrite, HANDLE_FLAG_INHERIT, 0)) {
		OutputDebugString(TEXT("Stdin SetHandleInformation"));
		exit(0);
	}

	TCHAR * szCmdline = const_cast<TCHAR *>(cmd.c_str());

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	STARTUPINFO siStartInfo;
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = stdoutPipeWrite;
	siStartInfo.hStdOutput = stdoutPipeWrite;
	siStartInfo.hStdInput = stdinPipeRead;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = SW_HIDE;

	BOOL bSuccess = CreateProcess(NULL,
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		&cwd[0],       // current working directory
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION

	if (!bSuccess) {
		OutputDebugString(TEXT("CreateProcess"));
		exit(0);
	}

	CloseHandle(stdoutPipeWrite);
	CloseHandle(stdinPipeRead);
	CloseHandle(stdinPipeWrite);

	std::array<char, 128> buffer;

	bool done = false;
	for (;;) {
		DWORD dwRead;
		bSuccess = ReadFile(stdoutPipeRead, buffer.data(), 128, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) {
			if (done) {
				break;
			}
			DWORD res = WaitForSingleObject(piProcInfo.hProcess, 200);
			if (res == WAIT_OBJECT_0) {
				done = true;
				continue;
			}
			if (res != WAIT_TIMEOUT) {
				OutputDebugString(L"WaitForSingleObject FAILED\n");
				exit(0);
			}
			continue;
		}
		if (callback) {
			callback(buffer.data(), dwRead);
		}
	}

	CloseHandle(stdoutPipeRead);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
}
