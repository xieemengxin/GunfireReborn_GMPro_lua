// DllInjector.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <io.h>

// 初始化控制台以支持 Unicode 输出
void InitConsole() {
    // 设置控制台代码页
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // 设置标准输出为 UTF-8
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
}

// 获取目标进程ID（根据进程名）
DWORD GetProcessIdByName(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::wcout << L"创建进程快照失败! 错误代码: " << GetLastError() << std::endl;
        return 0;
    }

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

// DLL注入核心函数
bool InjectDll(DWORD pid, const wchar_t* dllPath) {
    // 打开目标进程
    HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);

    if (hProcess == NULL) {
        std::wcout << L"打开进程失败! 错误代码: " << GetLastError() << std::endl;
        return false;
    }

    // 在目标进程内存中分配空间
    SIZE_T pathSize = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID remoteMemory = VirtualAllocEx(hProcess, NULL, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (remoteMemory == NULL) {
        std::wcout << L"内存分配失败! 错误代码: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // 写入DLL路径到目标进程内存
    if (!WriteProcessMemory(hProcess, remoteMemory, dllPath, pathSize, NULL)) {
        std::wcout << L"写入内存失败! 错误代码: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 获取LoadLibraryW函数地址
    LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");

    if (loadLibraryAddr == NULL) {
        std::wcout << L"获取函数地址失败! 错误代码: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 创建远程线程执行LoadLibraryW
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        loadLibraryAddr, remoteMemory, 0, NULL);

    if (hThread == NULL) {
        std::wcout << L"创建远程线程失败! 错误代码: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 等待线程执行完成
    WaitForSingleObject(hThread, INFINITE);

    // 清理资源
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    std::wcout << L"注入结果: " << (exitCode != 0 ? L"成功" : L"失败") << std::endl;

    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return exitCode != 0;
}

int main()
{
    // 初始化控制台
    InitConsole();

    const wchar_t* targetProcess = L"Gunfire Reborn.exe";
    wchar_t dllPath[MAX_PATH];

    // 获取DLL路径
    std::wcout << L"请将DLL文件拖拽到此窗口中，然后按回车键" << std::endl;
    std::wcout << L"(直接按回车将使用默认路径: ./GMPro.dll)：" << std::endl;
    std::wcin.getline(dllPath, MAX_PATH);

    // 如果用户直接按回车，使用默认路径
    if (wcslen(dllPath) == 0) {
        // 获取当前目录
        GetCurrentDirectoryW(MAX_PATH, dllPath);
        // 拼接默认DLL名称
        wcscat_s(dllPath, MAX_PATH, L"\\GMPro.dll");
        std::wcout << L"使用默认DLL路径: " << dllPath << std::endl;
    } else {
        // 移除可能的引号
        if (dllPath[0] == L'"') {
            size_t len = wcslen(dllPath);
            if (dllPath[len - 1] == L'"') {
                dllPath[len - 1] = L'\0';
                memmove(dllPath, dllPath + 1, len * sizeof(wchar_t));
            }
        }
    }

    std::wcout << L"\n目标进程: [" << targetProcess << L"]" << std::endl;
    std::wcout << L"DLL路径: [" << dllPath << L"]" << std::endl;

    // 检查DLL文件是否存在
    if (GetFileAttributesW(dllPath) == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"错误: DLL文件不存在! 路径: " << dllPath << std::endl;
        std::wcerr << L"错误代码: " << GetLastError() << std::endl;
        std::wcout << L"\n按回车键退出..." << std::endl;
        std::wcin.get();
        return 1;
    }

    // 等待游戏进程
    DWORD pid = 0;
    std::wcout << L"\n等待游戏进程启动..." << std::endl;
    while ((pid = GetProcessIdByName(targetProcess)) == 0) {
        Sleep(1000);  // 每秒检查一次
        std::wcout << L"." << std::flush;
    }
    std::wcout << std::endl;

    std::wcout << L"找到进程ID: " << pid << std::endl;
    std::wcout << L"\n准备注入，按回车键继续..." << std::endl;
    std::wcin.get();

    // 执行注入
    if (InjectDll(pid, dllPath)) {
        std::wcout << L"成功注入到进程: " << targetProcess << std::endl;
    } else {
        std::wcerr << L"注入失败!" << std::endl;
    }

    std::wcout << L"\n按回车键退出..." << std::endl;
    std::wcin.get();
    return 0;
}

