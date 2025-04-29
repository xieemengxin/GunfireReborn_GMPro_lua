#pragma once

#include <pch.h>
#include <Engine.h>
#include <Menu.h>
#include <queue>
#include <Bridge.h>
using namespace DX11Base;

extern "C" {
    __declspec(dllexport) void SetUIVisibility(BOOL show);
    __declspec(dllexport) BOOL GetUIVisibility();
    __declspec(dllexport) BOOL InitializeMod(LPVOID dwModule);
    __declspec(dllexport) void RemoteRunScript(const char* script);
}
void ClientBGThread()
{
    while (g_Running)
    {
        Menu::Loops();

        if (g_KillSwitch)
        {
            g_D3D11Window->UnhookD3D();
            g_Hooking->Shutdown();
            g_Engine.release();     //  releases all created class instances
            g_Running = false;
        }

        std::this_thread::sleep_for(1ms);
        std::this_thread::yield();
    }
}

void LuaExecutionThread()
{
    g_Console->cLog("[+] Lua执行线程启动", Console::EColors::EColor_green);


    std::chrono::steady_clock::time_point nextExecutionTime = 
        std::chrono::steady_clock::now();
    const int cooldownSeconds = 10;

    while (g_LuaVM->threadRunning->load() && g_Running)
    {
        std::string scriptToExecute;
        bool hasScript = false;

      
        {
            std::unique_lock<std::mutex> lock(*g_LuaVM->queueMutex);
            if (g_LuaVM->scriptQueue->empty()) {
                g_LuaVM->queueCondition->wait(lock, [&]() {
                    return !g_LuaVM->scriptQueue->empty() || !g_LuaVM->threadRunning->load() || !g_Running;
                });
            }

            if (!g_LuaVM->threadRunning->load() || !g_Running) break;

            if (!g_LuaVM->scriptQueue->empty()) {
                scriptToExecute = std::move(g_LuaVM->scriptQueue->front());
                g_LuaVM->scriptQueue->pop();

      
                nextExecutionTime = std::chrono::steady_clock::now() +
                    std::chrono::seconds(cooldownSeconds);
            }
        }

  
        if (!scriptToExecute.empty()) {
            try {
                g_Console->cLog("[+] 开始执行脚本...", Console::EColors::EColor_green);
                auto startTime = std::chrono::steady_clock::now();
                
                bool result = g_LuaVM->ExecuteString(scriptToExecute.c_str());

                auto endTime = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    endTime - startTime).count();

                if (result) {
                    g_Console->cLog("[+] 脚本执行成功 (耗时: %lld 毫秒)",
                        Console::EColors::EColor_green, duration);
                } else {
                    g_Console->cLog("[-] 脚本执行失败 (耗时: %lld 毫秒)",
                        Console::EColors::EColor_red, duration);
                }
            }
            catch (const std::exception& e) {
                g_Console->cLog("[-] 执行脚本时发生异常: %s", 
                    Console::EColors::EColor_red, e.what());
            }
        }
    }

    g_Console->cLog("[+] Lua执行线程正常退出", Console::EColors::EColor_yellow);
}

DWORD WINAPI MainThread_Initialize(LPVOID dwModule)
{
    UNREFERENCED_PARAMETER(dwModule);

    g_Engine = std::make_unique<Engine>();
    g_Console->InitializeConsole("DEBUG CONSOLE", true);
    g_Engine->Init();
    g_Console->cLog("[+] 是否初始化UI:%s", Console::EColors::EColor_green,  (g_HookD3DSwitch ? "是" : "否"));
    if (g_HookD3DSwitch)
    {
        g_D3D11Window->HookD3D();
    }
    g_Hooking->Initialize();

    // 初始化 Lua VM
    if (!g_LuaVM->Initialize()) {
        g_Console->cLog("[-] Failed to initialize Lua VM", Console::EColor_red);
        return FALSE;
    }
    // 创建脚本队列
    g_LuaVM->scriptQueue = new std::queue<std::string>();
    g_LuaVM->queueMutex = new std::mutex();
    g_LuaVM->queueCondition = new std::condition_variable();
    g_LuaVM->threadRunning = new std::atomic<bool>(true);
    
    // 启动脚本处理器
    g_LuaVM->StartScriptProcessor();

    // 启动Lua执行线程
    std::thread(LuaExecutionThread).detach();

    // INITIALIZE BACKGROUND THREAD
    std::thread WCMUpdate(ClientBGThread);

    // RENDER LOOP
    g_Running = true;
    static int LastTick = 0;
    while (g_Running)
    {
        if ((Engine::GamePadGetKeyState(XINPUT_GAMEPAD_RIGHT_THUMB | XINPUT_GAMEPAD_LEFT_THUMB) || Engine::GetKeyState(VK_INSERT, 0)) && ((GetTickCount64() - LastTick) > 500))
        {
            g_Engine->bShowMenu = !g_Engine->bShowMenu;           //  Main Menu Window
            g_Engine->bShowHud = !g_Engine->bShowMenu;            //  Render Window
            LastTick = GetTickCount64();
        }

        // 其他代码...
        std::this_thread::sleep_for(1ms);
        std::this_thread::yield();
    }

    // EXIT
    WCMUpdate.join();
    FreeLibraryAndExitThread(g_hModule, EXIT_SUCCESS);
    return EXIT_SUCCESS;

}

extern "C" {
    // 设置UI可见性
    __declspec(dllexport) void SetUIVisibility(BOOL show) {
        bool wasVisible = DX11Base::g_HookD3DSwitch;
        DX11Base::g_HookD3DSwitch = show;
        // 确保调用此函数的线程有权限访问控制台
        try {
            // 获取标准输出句柄
            HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hStdout == INVALID_HANDLE_VALUE || hStdout == NULL) {
                // 如果无效，则分配控制台
                AllocConsole();
                hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

                // 设置控制台输出
                FILE* fp;
                freopen_s(&fp, "CONOUT$", "w", stdout);
            }

            // 使用Windows API直接写入控制台
            const char* message = show ? "UI open\n" : "UI close\n";
            DWORD written;
            WriteConsoleA(hStdout, message, (DWORD)strlen(message), &written, NULL);
        }
        catch (...) {
            // 处理任何异常
        }
    }

    // 获取UI可见性状态
    __declspec(dllexport) BOOL GetUIVisibility() {
        return DX11Base::g_HookD3DSwitch ? TRUE : FALSE;
    }

    // 初始化Mod的入口函数，通过CreateThread调用
    __declspec(dllexport) BOOL InitializeMod(LPVOID dwModule) {
        HANDLE hThread = CreateThread(0, 0, MainThread_Initialize, dwModule, 0, 0);
        if (hThread) {
            CloseHandle(hThread);
            return TRUE;
        }
        return FALSE;
    }

    //远程执行脚本
    __declspec(dllexport) void RemoteRunScript(const char* script) {
        if (!g_LuaVM || !g_LuaVM->IsInitialized()) return;

        std::lock_guard<std::mutex> lock(CodeEditor::scriptMutex);

        // 使用新的队列访问函数
        if (!g_LuaVM->QueueScriptForExecution(script)) {
            g_Console->cLog("[-] 无法添加脚本到执行队列", Console::EColors::EColor_red);
        }
        else {
            g_Console->cLog("[+] 脚本已加入执行队列", Console::EColors::EColor_green);
        }
    }


    


}

