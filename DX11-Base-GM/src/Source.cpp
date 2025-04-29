#include "Source.h"
#include "LuaVM.h"
#include "Bridge.h"
#include "helper.h"
#include <thread>
#include <chrono>

namespace DX11Base
{
    void LuaExecutionThread()
    {
        auto& luaVM = *g_LuaVM.get();
        
        while (true)
        {
            std::string script;
            {
                std::unique_lock<std::mutex> lock(*luaVM.queueMutex);
                luaVM.queueCondition->wait(lock, [&]() {
                    return !luaVM.scriptQueue->empty() || !luaVM.threadRunning->load();
                });

                if (!luaVM.threadRunning->load() && luaVM.scriptQueue->empty())
                {
                    break;
                }

                if (!luaVM.scriptQueue->empty())
                {
                    script = luaVM.scriptQueue->front();
                    luaVM.scriptQueue->pop();
                }
            }

            if (!script.empty())
            {
                luaVM.ExecuteString(script.c_str());
            }
        }
        
        g_Console->cLog("[+] Lua执行线程已退出", Console::EColors::EColor_green);
    }

    void Initialize()
    {
        g_LuaVM = std::make_unique<LuaVM>();
        if (!g_LuaVM->Initialize())
        {
            g_Console->cLog("[-] 初始化Lua虚拟机失败", Console::EColors::EColor_red);
            return;
        }

        // 创建脚本队列
        g_LuaVM->scriptQueue = new std::queue<std::string>();
        g_LuaVM->queueMutex = new std::mutex();
        g_LuaVM->queueCondition = new std::condition_variable();
        g_LuaVM->threadRunning = new std::atomic<bool>(true);

        // 注册队列访问函数
        g_LuaVM->RegisterQueueAccessFunctions(
            g_LuaVM->scriptQueue,
            g_LuaVM->queueMutex,
            g_LuaVM->queueCondition,
            g_LuaVM->threadRunning
        );

        // 启动脚本处理线程
        g_LuaVM->StartScriptProcessor();
        std::thread(LuaExecutionThread).detach();
        
        g_Console->cLog("[+] Lua系统初始化完成", Console::EColors::EColor_green);
    }

    void Shutdown()
    {
        if (g_LuaVM)
        {
            // 停止脚本处理器
            g_LuaVM->StopScriptProcessor();
            
            // 关闭Lua VM
            g_LuaVM->Shutdown();

            // 清理队列相关资源
            delete g_LuaVM->scriptQueue;
            delete g_LuaVM->queueMutex;
            delete g_LuaVM->queueCondition;
            delete g_LuaVM->threadRunning;
            
            // 重置指针为nullptr
            g_LuaVM->scriptQueue = nullptr;
            g_LuaVM->queueMutex = nullptr;
            g_LuaVM->queueCondition = nullptr;
            g_LuaVM->threadRunning = nullptr;

            // 释放LuaVM实例
            g_LuaVM.reset();
            
            g_Console->cLog("[+] Lua系统已关闭", Console::EColors::EColor_green);
        }
    }
} 