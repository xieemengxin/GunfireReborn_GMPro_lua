// DllInjector.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <io.h>
#include <Psapi.h>



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

// 获取目标进程中指定模块的句柄
HMODULE GetRemoteModuleHandle(DWORD pid, const wchar_t* moduleName) {
    HMODULE hResult = NULL;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        std::wcout << L"无法打开进程获取模块信息! 错误代码: " << GetLastError() << std::endl;
        return NULL;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W moduleEntry;
        moduleEntry.dwSize = sizeof(moduleEntry);
        
        std::wcout << L"开始查找模块: " << moduleName << std::endl;
        
        if (Module32FirstW(hSnapshot, &moduleEntry)) {
            do {
                //std::wcout << L"检查模块: " << moduleEntry.szModule << std::endl;
                if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                    hResult = moduleEntry.hModule;
                    std::wcout << L"找到模块! 基址: 0x" << std::hex << (DWORD_PTR)hResult << std::dec << std::endl;
                    break;
                }
            } while (Module32NextW(hSnapshot, &moduleEntry));
        } else {
            std::wcout << L"Module32First失败，错误码: " << GetLastError() << std::endl;
        }
        CloseHandle(hSnapshot);
    } else {
        std::wcout << L"创建模块快照失败，错误码: " << GetLastError() << std::endl;
    }
    
    CloseHandle(hProcess);
    return hResult;
}

// 远程调用导出函数
bool CallRemoteExportFunction(HANDLE hProcess, HMODULE hRemoteModule, const char* funcName, LPVOID pParam, size_t paramSize) {
    if (!hProcess || !hRemoteModule) {
        std::wcout << L"无效的进程句柄或模块句柄!" << std::endl;
        return false;
    }

    std::wcout << L"调用函数: " << funcName << L" 在模块: 0x" << std::hex << (DWORD_PTR)hRemoteModule << std::dec << std::endl;
    
    // 尝试不同的函数名格式
    const char* possibleNames[] = {
        funcName,                      // 原始名称
        funcName,                      // 原始名称(重复是为了下面的格式化好处理)
        nullptr,                       // 用于存储stdcall格式
        nullptr                        // 用于存储C++格式
    };
    
    // 生成stdcall名称(_Function@N)
    char stdcallName[128];
    sprintf_s(stdcallName, "_%s@%zu", funcName, (paramSize ? sizeof(DWORD_PTR) : 0));
    possibleNames[2] = _strdup(stdcallName);
    
    // 生成C++可能的修饰名(?Function@@...)，只是一个简单的示例，实际C++名称修饰更复杂
    char cppName[128];
    if (strcmp(funcName, "SetUIVisibility") == 0) {
        strcpy_s(cppName, "?SetUIVisibility@@YAXH@Z");  // 假设参数是BOOL(int)
    } else if (strcmp(funcName, "GetUIVisibility") == 0) {
        strcpy_s(cppName, "?GetUIVisibility@@YAHXZ");   // 无参数，返回BOOL
    } else if (strcmp(funcName, "InitializeMod") == 0) {
        strcpy_s(cppName, "?InitializeMod@@YAHPAX@Z");  // 参数是LPVOID
    } else {
        cppName[0] = '\0';
    }
    
    if (cppName[0] != '\0') {
        possibleNames[3] = _strdup(cppName);
    }
    
    // 导出表方式获取函数地址
    DWORD_PTR funcAddress = 0;
    
    // 创建一个内存区域来存储导出表
    BYTE exportBuffer[4096];
    SIZE_T bytesReadSize;
    
    // 获取DOS头
    IMAGE_DOS_HEADER dosHeader;
    if (!ReadProcessMemory(hProcess, hRemoteModule, &dosHeader, sizeof(dosHeader), &bytesReadSize) || bytesReadSize != sizeof(dosHeader)) {
        std::wcout << L"无法读取DOS头! 错误: " << GetLastError() << std::endl;
        goto CLEANUP;
    }
    
    // 获取NT头
    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders), &bytesReadSize) || bytesReadSize != sizeof(ntHeaders)) {
        std::wcout << L"无法读取NT头! 错误: " << GetLastError() << std::endl;
        goto CLEANUP;
    }
    
    // 获取导出表
    if (ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size > 0) {
        DWORD exportRVA = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        IMAGE_EXPORT_DIRECTORY exportDir;
        
        if (!ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + exportRVA, &exportDir, sizeof(exportDir), &bytesReadSize) || bytesReadSize != sizeof(exportDir)) {
            std::wcout << L"无法读取导出表! 错误: " << GetLastError() << std::endl;
            goto CLEANUP;
        }
        
        // 读取名称数组
        DWORD* nameRVAs = new DWORD[exportDir.NumberOfNames];
        if (!ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + exportDir.AddressOfNames, nameRVAs, exportDir.NumberOfNames * sizeof(DWORD), &bytesReadSize)) {
            std::wcout << L"无法读取名称RVA数组! 错误: " << GetLastError() << std::endl;
            delete[] nameRVAs;
            goto CLEANUP;
        }
        
        // 读取序号数组
        WORD* ordinals = new WORD[exportDir.NumberOfNames];
        if (!ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + exportDir.AddressOfNameOrdinals, ordinals, exportDir.NumberOfNames * sizeof(WORD), &bytesReadSize)) {
            std::wcout << L"无法读取序号数组! 错误: " << GetLastError() << std::endl;
            delete[] nameRVAs;
            delete[] ordinals;
            goto CLEANUP;
        }
        
        // 读取函数地址数组
        DWORD* funcRVAs = new DWORD[exportDir.NumberOfFunctions];
        if (!ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + exportDir.AddressOfFunctions, funcRVAs, exportDir.NumberOfFunctions * sizeof(DWORD), &bytesReadSize)) {
            std::wcout << L"无法读取函数RVA数组! 错误: " << GetLastError() << std::endl;
            delete[] nameRVAs;
            delete[] ordinals;
            delete[] funcRVAs;
            goto CLEANUP;
        }
        
        // 打印所有导出函数名称，方便调试
        //std::wcout << L"DLL导出的函数列表:" << std::endl;
        for (DWORD i = 0; i < exportDir.NumberOfNames; i++) {
            char funcNameBuffer[256] = {0};
            if (ReadProcessMemory(hProcess, (BYTE*)hRemoteModule + nameRVAs[i], funcNameBuffer, sizeof(funcNameBuffer) - 1, &bytesReadSize) && bytesReadSize > 0) {
               // std::wcout << L"  " << funcNameBuffer << std::endl;
                
                // 检查是否是我们要找的函数
                for (int j = 0; j < 4; j++) {
                    if (possibleNames[j] && strcmp(funcNameBuffer, possibleNames[j]) == 0) {
                        WORD ordinal = ordinals[i];
                        funcAddress = (DWORD_PTR)hRemoteModule + funcRVAs[ordinal];
                        //std::wcout << L"找到函数 " << possibleNames[j] << L" 地址: 0x" << std::hex << funcAddress << std::dec << std::endl;
                        break;
                    }
                }
            }
        }
        
        delete[] nameRVAs;
        delete[] ordinals;
        delete[] funcRVAs;
        
        if (funcAddress != 0) {
            goto FOUND_FUNCTION;
        }
    }
    


CLEANUP:
    // 释放动态分配的内存
    if (possibleNames[2]) free((void*)possibleNames[2]);
    if (possibleNames[3]) free((void*)possibleNames[3]);
    
    if (!funcAddress) {
        std::wcout << L"未能获取函数地址! " << funcName << std::endl;
        return false;
    }

FOUND_FUNCTION:
    // 准备参数区域（如果需要）
    LPVOID pRemoteData = NULL;
    if (pParam && paramSize > 0) {
        pRemoteData = VirtualAllocEx(hProcess, NULL, paramSize, MEM_COMMIT, PAGE_READWRITE);
        if (!pRemoteData) {
            std::wcout << L"无法分配函数参数内存! 错误: " << GetLastError() << std::endl;
            return false;
        }

        if (!WriteProcessMemory(hProcess, pRemoteData, pParam, paramSize, NULL)) {
            std::wcout << L"无法写入函数参数! 错误: " << GetLastError() << std::endl;
            VirtualFreeEx(hProcess, pRemoteData, 0, MEM_RELEASE);
            return false;
        }
    }

    // 调用导出函数
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        (LPTHREAD_START_ROUTINE)funcAddress, pRemoteData, 0, NULL);
    if (!hThread) {
        std::wcout << L"无法创建函数调用远程线程! 错误: " << GetLastError() << std::endl;
        if (pRemoteData) VirtualFreeEx(hProcess, pRemoteData, 0, MEM_RELEASE);
        return false;
    }

    // 等待函数执行完成
    WaitForSingleObject(hThread, INFINITE);
    
    // 获取函数返回值
    DWORD returnValue = 0;
    GetExitCodeThread(hThread, &returnValue);
    std::wcout << L"函数返回值: " << returnValue << std::endl;
    
    CloseHandle(hThread);
    if (pRemoteData) VirtualFreeEx(hProcess, pRemoteData, 0, MEM_RELEASE);
    
    return true;
}

// 增加一个函数来解决GetRemoteModuleHandle问题
HMODULE EnsureGetRemoteModuleHandle(DWORD pid, const wchar_t* moduleName, HMODULE fallbackHandle) {
    HMODULE hResult = GetRemoteModuleHandle(pid, moduleName);
    
    // 如果无法获取，尝试枚举所有模块并比较路径
    if (hResult == NULL) {
        std::wcout << L"通过名称无法获取模块句柄，尝试通过枚举所有模块..." << std::endl;
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess != NULL) {
            HMODULE hMods[1024];
            DWORD cbNeeded;
            
            if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
                for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                    wchar_t szModName[MAX_PATH];
                    
                    // 获取模块完整路径
                    if (GetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t))) {
                        // 获取文件名部分
                        wchar_t* fileName = wcsrchr(szModName, L'\\');
                        if (fileName != NULL) {
                            fileName++; // 跳过反斜杠
                            
                           // std::wcout << L"检查模块: " << fileName << L" 基址: 0x" << std::hex << (DWORD_PTR)hMods[i] << std::dec << std::endl;
                            
                            if (_wcsicmp(fileName, moduleName) == 0) {
                                hResult = hMods[i];
                                std::wcout << L"通过枚举找到模块! 基址: 0x" << std::hex << (DWORD_PTR)hResult << std::dec << std::endl;
                                break;
                            }
                        }
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }
    
    // 如果仍然无法获取，使用备用句柄
    if (hResult == NULL) {
        std::wcout << L"无法获取模块句柄，使用LoadLibrary返回的句柄: 0x" << std::hex << (DWORD_PTR)fallbackHandle << std::dec << std::endl;
        hResult = fallbackHandle;
    }
    
    return hResult;
}

// DLL注入核心函数
bool InjectDll(DWORD pid, const wchar_t* dllPath, BOOL showUI) {
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
        DWORD error = GetLastError();
        std::wcout << L"创建远程线程失败! 错误代码: " << error << std::endl;
        
        // 显示更多有关此错误的信息
        wchar_t* errorMsg = NULL;
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0, (LPWSTR)&errorMsg, 0, NULL);
        if (errorMsg) {
            std::wcout << L"错误信息: " << errorMsg << std::endl;
            LocalFree(errorMsg);
        }
        
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 等待线程执行完成，设置更长的超时时间
    DWORD waitResult = WaitForSingleObject(hThread, 10000); // 等待10秒
    if (waitResult != WAIT_OBJECT_0) {
        std::wcout << L"等待远程线程超时或失败! 错误码: " << waitResult << std::endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // 获取线程退出码（即DLL模块的句柄）
    DWORD dllHandle = 0;
    if (!GetExitCodeThread(hThread, &dllHandle)) {
        std::wcout << L"获取线程退出码失败! 错误代码: " << GetLastError() << std::endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    CloseHandle(hThread);

    if (dllHandle == 0) {
        std::wcout << L"DLL加载失败!" << std::endl;
        
        // 尝试在目标进程中获取最后的错误码
        LPTHREAD_START_ROUTINE getLastErrorAddr = (LPTHREAD_START_ROUTINE)
            GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetLastError");
        
        if (getLastErrorAddr != NULL) {
            HANDLE errorThread = CreateRemoteThread(hProcess, NULL, 0,
                getLastErrorAddr, NULL, 0, NULL);
            if (errorThread != NULL) {
                WaitForSingleObject(errorThread, INFINITE);
                DWORD lastError = 0;
                GetExitCodeThread(errorThread, &lastError);
                CloseHandle(errorThread);
                
                std::wcout << L"目标进程中的最后错误码: " << lastError << std::endl;
                
                // 显示更多有关此错误的信息
                wchar_t* errorMsg = NULL;
                FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, lastError, 0, (LPWSTR)&errorMsg, 0, NULL);
                if (errorMsg) {
                    std::wcout << L"错误信息: " << errorMsg << std::endl;
                    LocalFree(errorMsg);
                }
            }
        }
        
        // 检查DLL文件是否为有效的PE文件
        std::wcout << L"检查DLL文件是否为有效的PE文件..." << std::endl;
        HANDLE hFile = CreateFileW(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize > 0) {
                HANDLE hFileMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
                if (hFileMapping != NULL) {
                    LPVOID fileData = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
                    if (fileData != NULL) {
                        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)fileData;
                        if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                            PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)fileData + dosHeader->e_lfanew);
                            if (ntHeaders->Signature == IMAGE_NT_SIGNATURE) {
                                std::wcout << L"DLL文件是有效的PE文件" << std::endl;
                                
                                // 检查是否为32位或64位DLL
                                BOOL is64Bit = (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
                                std::wcout << L"DLL架构: " << (is64Bit ? L"64位" : L"32位") << std::endl;
                                
                                // 检查当前进程架构
                                BOOL isTarget64Bit = FALSE;
                                IsWow64Process(hProcess, &isTarget64Bit);
                                isTarget64Bit = !isTarget64Bit; // IsWow64Process返回TRUE表示32位进程在64位系统上运行
                                
                                std::wcout << L"目标进程架构: " << (isTarget64Bit ? L"64位" : L"32位") << std::endl;
                                
                                // 检查架构匹配
                                if (is64Bit != isTarget64Bit) {
                                    std::wcout << L"错误: DLL架构与目标进程不匹配!" << std::endl;
                                }
                            } else {
                                std::wcout << L"无效的NT头" << std::endl;
                            }
                        } else {
                            std::wcout << L"无效的DOS头" << std::endl;
                        }
                    }
                    UnmapViewOfFile(fileData);
                }
            }
            CloseHandle(hFile);
        } else {
            std::wcout << L"无法打开DLL文件进行验证" << std::endl;
        }
        
        // 检查是否有依赖DLL问题
        std::wcout << L"可能的原因:" << std::endl;
        std::wcout << L"1. DLL可能依赖其他DLL，但目标进程无法找到它们" << std::endl;
        std::wcout << L"2. DLL可能需要特定权限才能加载" << std::endl;
        std::wcout << L"3. DLL可能与目标进程的架构不匹配(32位 vs 64位)" << std::endl;
        
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    std::wcout << L"DLL加载成功，地址: 0x" << std::hex << dllHandle << std::dec << std::endl;

    // 获取DLL文件名(不含路径)
    wchar_t dllName[MAX_PATH];
    const wchar_t* lastSlash = wcsrchr(dllPath, L'\\');
    if (lastSlash) {
        wcscpy_s(dllName, MAX_PATH, lastSlash + 1);
    } else {
        wcscpy_s(dllName, MAX_PATH, dllPath);
    }
    
    std::wcout << L"DLL文件名: " << dllName << std::endl;
    
    // 等待一下确保DLL完全加载
    Sleep(1000);

    // 在目标进程中获取模块句柄
    HMODULE hRemoteModule = EnsureGetRemoteModuleHandle(pid, dllName, (HMODULE)dllHandle);
    std::wcout << L"使用模块句柄: 0x" << std::hex << (DWORD_PTR)hRemoteModule << std::dec << std::endl;

    // 调用导出函数
    bool success = true;
    
    // 设置UI可见性
    if (!CallRemoteExportFunction(hProcess, hRemoteModule, "SetUIVisibility", (LPVOID)showUI, sizeof(showUI))) {
        std::wcout << L"设置UI可见性失败!" << std::endl;
        success = false;
    } else {
        std::wcout << L"成功设置UI可见性!"<< showUI << std::endl;
    }

    // 初始化模块
    if (!CallRemoteExportFunction(hProcess, hRemoteModule, "InitializeMod", &hRemoteModule, sizeof(hRemoteModule))) {
        std::wcout << L"初始化模块失败!" << std::endl;
        success = false;
    } else {
        std::wcout << L"成功初始化模块!" << std::endl;
    }

    //测试脚本执行
    Sleep(3000);
    const char* test = "print(\"Lua Script Remote Execute Success\")";
    if (!CallRemoteExportFunction(hProcess, hRemoteModule, "RemoteRunScript", (LPVOID)test, strlen(test) + 1)) {
        std::wcout << L"脚本执行测试失败!" << std::endl;
        success = false;
    }
    else {
        std::wcout << L"脚本执行测试成功!" << std::endl;
    }

    // 清理资源
    VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return success;
}

int main()
{
    // 初始化控制台
    InitConsole();

    const wchar_t* targetProcess = L"Gunfire Reborn.exe";
    wchar_t dllPath[MAX_PATH];
    BOOL showUI = TRUE;

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

    // 验证DLL文件
    std::wcout << L"验证DLL文件..." << std::endl;
    HANDLE hFile = CreateFileW(dllPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::wcerr << L"错误: 无法打开DLL文件! 路径: " << dllPath << std::endl;
        std::wcerr << L"错误代码: " << GetLastError() << std::endl;
        std::wcout << L"\n按回车键退出..." << std::endl;
        std::wcin.get();
        return 1;
    }
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0) {
        std::wcerr << L"错误: DLL文件大小为零!" << std::endl;
        CloseHandle(hFile);
        std::wcout << L"\n按回车键退出..." << std::endl;
        std::wcin.get();
        return 1;
    }
    
    // 简单验证是否为PE文件
    BYTE dosHeader[2];
    DWORD bytesRead;
    ReadFile(hFile, dosHeader, 2, &bytesRead, NULL);
    if (bytesRead != 2 || dosHeader[0] != 'M' || dosHeader[1] != 'Z') {
        std::wcerr << L"错误: 不是有效的DLL文件! (无效的MZ头)" << std::endl;
        CloseHandle(hFile);
        std::wcout << L"\n按回车键退出..." << std::endl;
        std::wcin.get();
        return 1;
    }
    
    CloseHandle(hFile);
    std::wcout << L"DLL文件验证成功" << std::endl;

    // 询问是否显示UI
    wchar_t showUIOption;
    std::wcout << L"\n是否显示UI界面? (Y/n): ";
    std::wcin >> showUIOption;
    std::wcin.ignore(1000, L'\n'); // 清除输入缓冲区
    showUI = (showUIOption != L'n' && showUIOption != L'N');

    std::wcout << L"\n目标进程: [" << targetProcess << L"]" << std::endl;
    std::wcout << L"DLL路径: [" << dllPath << L"]" << std::endl;
    std::wcout << L"UI设置: [" << (showUI ? L"显示" : L"隐藏") << L"]" << std::endl;

 
    BOOL needAdmin = true;
    if (needAdmin) {
        // 检查当前是否已以管理员身份运行
        BOOL isElevated = FALSE;
        HANDLE hToken = NULL;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
            TOKEN_ELEVATION elevation;
            DWORD cbSize = sizeof(TOKEN_ELEVATION);
            if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
                isElevated = elevation.TokenIsElevated;
            }
            CloseHandle(hToken);
        }
        
        if (!isElevated) {
            std::wcout << L"需要以管理员权限运行，请重启程序并以管理员身份运行..." << std::endl;
            std::wcout << L"\n按回车键退出..." << std::endl;
            std::wcin.get();
            return 1;
        } else {
            std::wcout << L"已以管理员权限运行" << std::endl;
        }
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
    
    // 检查进程的位数
    BOOL isTarget64Bit = FALSE;
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess != NULL) {
        IsWow64Process(hProcess, &isTarget64Bit);
        isTarget64Bit = !isTarget64Bit; // IsWow64Process返回TRUE表示32位进程在64位系统上运行
        std::wcout << L"目标进程架构: " << (isTarget64Bit ? L"64位" : L"32位") << std::endl;
        CloseHandle(hProcess);
    }
    
    std::wcout << L"\n准备注入，按回车键继续..." << std::endl;
    std::wcin.get();

    // 执行注入
    if (InjectDll(pid, dllPath, showUI)) {
        std::wcout << L"成功注入到进程: " << targetProcess << std::endl;
    } else {
        std::wcerr << L"注入过程发生错误!" << std::endl;
    }

    std::wcout << L"\n按回车键退出..." << std::endl;
    std::wcin.get();
    return 0;
}

