#pragma once

#include "helper.h"
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

// 静态链接的配置
EXTERN_C{
    #include "..\libs\lua\luajit.h"
    #include "..\libs\lua\lua.hpp"
    #include "..\libs\lua\lualib.h"
    #include "..\libs\lua\lauxlib.h"
}

namespace DX11Base 
{
    // API函数声明
    namespace LuaAPI
    {
        // 获取游戏窗口大小
        int GetWindowSize(lua_State* L);

        // 绘制文本
        int DrawText(lua_State* L);

        // 绘制矩形
        int DrawRect(lua_State* L);

        // 获取鼠标位置
        int GetMousePos(lua_State* L);

        // 导入其他脚本
        int RequireScript(lua_State* L);
    }

    class LuaVM 
    {
    private:
        lua_State* L;
        bool isInitialized;
        std::unordered_set<std::string> loadingModules;  // 用于检测循环导入
        std::unordered_map<std::string, bool> loadedModules;  // 缓存已加载的模块
        
        // 修改 RegisterAPI 声明
        void RegisterAPI(lua_State* state = nullptr);
        
        // 错误处理
        static void LogError(const char* fmt, ...);
        // 专门用于 Lua 堆栈错误的日志函数
        static void LogLuaError(const char* prefix, const char* error, bool isStackTrace = false);

        // 检查模块是否已加载
        bool IsModuleLoaded(const std::string& modulePath);
        
        // 标记模块为已加载
        void MarkModuleLoaded(const std::string& modulePath);
        
        // 检查循环导入
        bool CheckCircularDependency(const std::string& modulePath);

        // 查找模块文件
        std::string FindModule(const std::string& moduleName);

        // 向指定状态注册函数
        void RegisterFunctionToState(lua_State* state, const char* name, lua_CFunction func, const char* namespace_);

        // 向指定状态添加搜索路径
        void AddSearchPathToState(lua_State* state, const char* path);

    public:
        LuaVM();
        ~LuaVM();

        // 初始化Lua虚拟机
        bool Initialize();

        // 执行Lua脚本文件
        bool ExecuteFile(const char* filename);

        // 执行Lua脚本字符串
        bool ExecuteString(const char* script);

        // 添加搜索路径
        void AddSearchPath(const char* path);

        // 获取Lua状态
        lua_State* GetState() const { return L; }

        // 检查是否初始化
        bool IsInitialized() { return isInitialized; }

        // 重置Lua状态
        void Reset();

        // 清除模块缓存
        void ClearModuleCache();

        // 获取已加载模块列表
        std::vector<std::string> GetLoadedModules() const;

        // 关闭方法
        void Shutdown();

        // 注册 C++ 函数到 Lua
        void RegisterFunction(const char* name, lua_CFunction func, const char* namespace_ = nullptr);

        // 队列相关方法
        void RegisterQueueAccessFunctions(std::queue<std::string>* queue, 
            std::mutex* mutex, 
            std::condition_variable* condition, 
            std::atomic<bool>* running);

        bool QueueScriptForExecution(const char* script);

        // 启动和停止脚本处理器
        void StartScriptProcessor();
        void StopScriptProcessor();

        // 队列相关成员
        std::queue<std::string>* scriptQueue;
        std::mutex* queueMutex;
        std::condition_variable* queueCondition;
        std::atomic<bool>* threadRunning;

        std::string defaultNameSpace{ "GMP" };

        friend int LuaAPI::RequireScript(lua_State* L);
    };

    inline std::unique_ptr<LuaVM> g_LuaVM;
} 