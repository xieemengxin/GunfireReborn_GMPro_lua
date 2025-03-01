#pragma once

#include "helper.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

// 静态链接的配置

EXTERN_C{
    #include "..\libs\lua\luajit.h"
    #include "..\libs\lua\lua.hpp"
    #include "..\libs\lua\lualib.h"
    #include "..\libs\lua\lauxlib.h"
#include <unordered_set>
#include <unordered_map>
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

    // 线程池类
    class ThreadPool {
    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

    public:
        ThreadPool(size_t threads);
        ~ThreadPool();

        template<class F>
        auto enqueue(F&& f) -> std::future<decltype(f())> {
            using return_type = decltype(f());
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace([task](){ (*task)(); });
            }
            condition.notify_one();
            return res;
        }
    };

    class LuaVM 
    {
    private:
        lua_State* L;
        bool isInitialized;
        std::unordered_set<std::string> loadingModules;  // 用于检测循环导入
        std::unordered_map<std::string, bool> loadedModules;  // 缓存已加载的模块
        
        // 线程池
        std::unique_ptr<ThreadPool> threadPool;
        std::mutex luaMutex;
        std::vector<lua_State*> threadStates;  // 每个线程的Lua状态
        bool shuttingDown;  // 添加关闭标志

        // 修改 RegisterAPI 声明
        void RegisterAPI(lua_State* state = nullptr);
        
        // 错误处理
        static void LogError(const char* fmt, ...);
        // 新增：专门用于 Lua 堆栈错误的日志函数
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

        // 异步执行脚本
        std::future<bool> ExecuteStringAsync(const char* script);
        std::future<bool> ExecuteFileAsync(const char* filename);
        
        // 创建新的Lua状态
        lua_State* CreateNewState();
        
        // 销毁Lua状态
        void DestroyState(lua_State* state);

        // 添加关闭方法
        void Shutdown();

        // 注册 C++ 函数到 Lua
        void RegisterFunction(const char* name, lua_CFunction func, const char* namespace_ = nullptr);

        void Update();

        // 获取线程状态列表
        const std::vector<lua_State*>& GetThreadStates() const { return threadStates; }

        std::string defaultNameSpace{ "GMP" };

        friend int LuaAPI::RequireScript(lua_State* L);
    };

   

    inline std::unique_ptr<LuaVM> g_LuaVM;
} 