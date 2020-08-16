#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <array>
#include <functional>
#include <vector>
#include <Windows.h>
#include <mbstring.h>

static void split(const std::wstring & str, std::vector<std::wstring> & split) {
	std::size_t current = 0;
	std::size_t previous = 0;
	while ((current = str.find(L"\n", previous)) != std::string::npos) {
		split.push_back(str.substr(previous, current - previous));
		previous = current + 1;
	}
	std::wstring last = str.substr(previous, std::string::npos);
	if (!last.empty()) {
		split.push_back(last);
	}
}

static std::wstring utf8_to_wstr(const std::string & str) {
	size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
	if (charsNeeded == 0)
		return std::wstring();

	std::vector<wchar_t> buffer(charsNeeded);
	size_t charsConverted = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &buffer[0], (int)buffer.size());
	if (charsConverted == 0)
		return std::wstring();

	return std::wstring(&buffer[0], charsConverted);
}

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

std::vector<std::wstring> exec(const std::wstring & cwd, const std::wstring & cmd ) {
	std::vector<std::wstring> lines;
	std::string result;
	exec(cwd, cmd, [&result](const char * data, DWORD size) { result.append(data, size); });

	std::wstring wresult = utf8_to_wstr(result);
	split(wresult, lines);
	return lines;
}
