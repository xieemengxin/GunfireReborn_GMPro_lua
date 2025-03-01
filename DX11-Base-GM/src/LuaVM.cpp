#include <pch.h>
#include "LuaVM.h"
#include "Engine.h"
#include "Menu.h"
#include "Bridge.h"

namespace DX11Base
{
	ThreadPool::ThreadPool(size_t threads) : stop(false)
	{
		for (size_t i = 0; i < threads; ++i)
			workers.emplace_back([this] {
			while (true) {
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(queue_mutex);
					condition.wait(lock, [this] {
						return stop || !tasks.empty();
						});
					if (stop && tasks.empty())
						return;
					task = std::move(tasks.front());
					tasks.pop();
				}
				task();
			}
				});
	}

	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
			worker.join();
	}

	LuaVM::LuaVM() : L(nullptr), isInitialized(false)
	{
		threadPool = std::make_unique<ThreadPool>(4);
	}

	LuaVM::~LuaVM()
	{
		if (L) {
			lua_close(L);
			L = nullptr;
		}
		isInitialized = false;
	}
	// 添加一个错误处理函数
	static int LuaErrorHandler(lua_State* L)
	{
		const char* msg = lua_tostring(L, 1);
		if (!msg) {
			msg = "(error with no message)";
		}

		luaL_traceback(L, L, msg, 1);  // 生成堆栈跟踪
		return 1;
	}

	// 自定义的 print 函数
	static int CustomPrint(lua_State* L) {
		int nargs = lua_gettop(L);
		std::string output;
		
		for (int i = 1; i <= nargs; i++) {
			if (i > 1) output += "\t";
			if (lua_isstring(L, i)) {
				output += lua_tostring(L, i);
			}
		}
		// 移除末尾的换行符，因为 cLog 会自动添加时间戳
		// output += "\n";  // 删除这行
		
		// 使用白色文本颜色，并确保添加到日志缓冲区
		g_Console->cLog(output.c_str(), Console::EColors::EColor_white);
		
		return 0;
	}

	bool LuaVM::Initialize()
	{
		if (isInitialized) return true;

		L = luaL_newstate();
		if (!L) {
			LogError("Failed to create Lua state");
			return false;
		}

		luaL_openlibs(L);
		RegisterAPI();

		// 添加脚本搜索路径
		std::string scriptsPath = CodeEditor::GetScriptsPath();
		AddSearchPath(scriptsPath.c_str());

		// 创建线程池的线程状态
		for (size_t i = 0; i < 4; i++) {  // 创建4个线程状态
			lua_State* threadState = CreateNewState();
			if (threadState) {
				std::lock_guard<std::mutex> lock(luaMutex);
				threadStates.push_back(threadState);
			}
		}

	

		isInitialized = true;
		return true;
	}

	void LuaVM::RegisterAPI(lua_State* state)
	{
		// 如果没有传入状态，使用主状态
		lua_State* target = state ? state : L;

		// 首先替换全局的 print 函数
		lua_pushcfunction(target, CustomPrint);
		lua_setglobal(target, "print");

		// 创建自定义命名空间
		lua_newtable(target);

		// 注册API函数到 GMP 命名空间
		lua_pushcfunction(target, CustomPrint);  // 在 GMP 命名空间也使用相同的 print 函数
		lua_setfield(target, -2, "print");

		lua_pushcfunction(target, LuaAPI::GetWindowSize);
		lua_setfield(target, -2, "getWindowSize");

		lua_pushcfunction(target, LuaAPI::DrawText);
		lua_setfield(target, -2, "drawText");

		lua_pushcfunction(target, LuaAPI::DrawRect);
		lua_setfield(target, -2, "drawRect");

		lua_pushcfunction(target, LuaAPI::GetMousePos);
		lua_setfield(target, -2, "getMousePos");

		lua_pushcfunction(target, LuaAPI::RequireScript);
		lua_setfield(target, -2, "require");

		// 设置命名空间名称为 GMP
		lua_setglobal(target, LuaVM::defaultNameSpace.c_str());
	}

	bool LuaVM::ExecuteFile(const char* filename)
	{
		if (!isInitialized || !filename) return false;

		lua_pushcfunction(L, LuaErrorHandler);
		int errHandlerIndex = lua_gettop(L);

		if (luaL_loadfile(L, filename)) {
			const char* error = lua_tostring(L, -1);
			LogLuaError("File load error:", error);
			lua_pop(L, 2);
			return false;
		}

		if (lua_pcall(L, 0, 0, errHandlerIndex)) {
			const char* stackTrace = lua_tostring(L, -1);
			LogLuaError("Runtime error in file:", stackTrace, true);
			lua_pop(L, 2);
			return false;
		}

		lua_pop(L, 1);
		return true;
	}

	bool LuaVM::ExecuteString(const char* script)
	{
		if (!isInitialized || !script) return false;

		std::lock_guard<std::mutex> lock(luaMutex);
		
		// 将执行任务添加到线程池
		threadPool->enqueue([this, script = std::string(script)]() {
			// 获取一个可用的线程状态
			lua_State* threadState = nullptr;
			{
				std::lock_guard<std::mutex> lock(luaMutex);
				for (auto state : threadStates) {
					if (state) {
						threadState = state;
						break;
					}
				}
			}

			if (!threadState) {
				LogLuaError("Error:", "No available thread state");
				return;
			}

			// 压入错误处理函数
			lua_pushcfunction(threadState, LuaErrorHandler);
			int errHandlerIndex = lua_gettop(threadState);

			// 使用单独的互斥锁保护脚本执行
			std::lock_guard<std::mutex> execLock(luaMutex);
			
			if (luaL_loadstring(threadState, script.c_str())) {
				const char* error = lua_tostring(threadState, -1);
				LogLuaError("Syntax error:", error);
				lua_pop(threadState, 2);
				return;
			}

			if (lua_pcall(threadState, 0, 0, errHandlerIndex)) {
				const char* error = lua_tostring(threadState, -1);
				LogLuaError("Runtime error:", error, true);
				lua_pop(threadState, 2);
				return;
			}

			lua_pop(threadState, 1);  // 弹出错误处理函数
		});
		
		return true;
	}

	void LuaVM::AddSearchPath(const char* path)
	{
		if (!isInitialized || !path) return;

		lua_getglobal(L, "package");
		lua_getfield(L, -1, "path");
		std::string cur_path = lua_tostring(L, -1);
		cur_path.append(";");
		cur_path.append(path);
		cur_path.append("?.lua");
		lua_pop(L, 1);
		lua_pushstring(L, cur_path.c_str());
		lua_setfield(L, -2, "path");
		lua_pop(L, 1);
	}

	void LuaVM::Reset()
	{
		if (L) {
			lua_close(L);
		}
		L = luaL_newstate();
		luaL_openlibs(L);
		RegisterAPI();
		isInitialized = (L != nullptr);
	}

	void LuaVM::LogError(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		g_Console->LogError(fmt, args);
		va_end(args);
	}

	bool LuaVM::IsModuleLoaded(const std::string& modulePath) {
		return loadedModules.find(modulePath) != loadedModules.end();
	}

	void LuaVM::MarkModuleLoaded(const std::string& modulePath) {
		loadedModules[modulePath] = true;
	}

	bool LuaVM::CheckCircularDependency(const std::string& modulePath) {
		return loadingModules.find(modulePath) != loadingModules.end();
	}

	std::string LuaVM::FindModule(const std::string& moduleName) {
		std::vector<std::string> searchPaths = {
			CodeEditor::GetScriptsPath(),
			CodeEditor::GetScriptsPath() + "libs/",
			CodeEditor::GetScriptsPath() + "modules/"
		};

		// 替换模块名中的点为路径分隔符
		std::string modulePath = moduleName;
		std::replace(modulePath.begin(), modulePath.end(), '.', '\\');

		for (const auto& basePath : searchPaths) {
			// 尝试不同的文件扩展名
			std::vector<std::string> attempts = {
				basePath + modulePath + ".lua",
				basePath + modulePath + "/init.lua",
				basePath + modulePath
			};

			for (const auto& path : attempts) {
				if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
					return path;
				}
			}
		}

		return "";
	}

	void LuaVM::ClearModuleCache() {
		loadedModules.clear();
	}

	std::vector<std::string> LuaVM::GetLoadedModules() const {
		std::vector<std::string> modules;
		for (const auto& pair : loadedModules) {
			modules.push_back(pair.first);
		}
		return modules;
	}

	lua_State* LuaVM::CreateNewState()
	{
		lua_State* newState = luaL_newstate();
		if (!newState) return nullptr;

		luaL_openlibs(newState);  // 注册标准库
		RegisterAPI(newState);     // 注册我们的自定义 API

		// 复制主状态的 GMP 表到新状态
		if (L) {
			// 在新状态创建 GMP 表
			lua_newtable(newState);
			lua_setglobal(newState, defaultNameSpace.c_str());

			// 获取主状态的 GMP 表
			lua_getglobal(L, defaultNameSpace.c_str());
			if (!lua_isnil(L, -1)) {
				// 获取新状态的 GMP 表
				lua_getglobal(newState, defaultNameSpace.c_str());

				// 遍历主状态的 GMP 表并复制到新状态
				lua_pushnil(L);  // 第一个键
				while (lua_next(L, -2) != 0) {
					// 复制键
					lua_pushvalue(L, -2);
					const char* key = lua_tostring(L, -1);
					
					// 检查值的类型
					if (lua_iscfunction(L, -2)) {
						// 如果是 C 函数，获取函数指针并在新状态注册
						lua_CFunction func = lua_tocfunction(L, -2);
						lua_pushcfunction(newState, func);
					} else {
						// 其他类型的值，尝试直接复制
						lua_pushvalue(L, -2);
						lua_xmove(L, newState, 1);
					}
					
					// 在新状态设置键值对
					lua_setfield(newState, -2, key);
					
					lua_pop(L, 2);  // 弹出键的副本和值
				}
				
				lua_pop(newState, 1);  // 弹出新状态的 GMP 表
			}
			lua_pop(L, 1);  // 弹出主状态的 GMP 表
		}

		return newState;
	}

	void LuaVM::DestroyState(lua_State* state)
	{
		if (state) {
			lua_close(state);
		}
	}

	void LuaVM::Shutdown()

	{

		// 设置关闭标志

		shuttingDown = true;

		// 等待并关闭线程池
		if (threadPool) {
			threadPool.reset();
		}

		// 清理所有线程状态

		{

			std::lock_guard<std::mutex> lock(luaMutex);

			for (auto state : threadStates) {

				if (state) {

					lua_close(state);

				}

			}

			threadStates.clear();

		}



		// 关闭主 Lua 状态

		if (L) {

			lua_close(L);

			L = nullptr;

		}



		// 清理其他资源

		isInitialized = false;

		loadingModules.clear();

		loadedModules.clear();

	}

	// 添加一个 RAII 包装器来管理 lua_State
	class LuaStateGuard {
	private:
		lua_State* state;
		LuaVM* vm;

	public:
		LuaStateGuard(LuaVM* vm) : vm(vm) {
			state = vm->CreateNewState();
		}

		~LuaStateGuard() {
			if (state) {
				vm->DestroyState(state);
			}
		}

		lua_State* get() { return state; }

		// 禁止拷贝
		LuaStateGuard(const LuaStateGuard&) = delete;
		LuaStateGuard& operator=(const LuaStateGuard&) = delete;
	};

	std::future<bool> LuaVM::ExecuteStringAsync(const char* script)
	{
		return threadPool->enqueue([this, script = std::string(script)]() {
			LuaStateGuard stateGuard(this);
			lua_State* threadState = stateGuard.get();
			bool result = false;

			try {
				if (threadState) {
					lua_pushcfunction(threadState, LuaErrorHandler);
					int errHandlerIndex = lua_gettop(threadState);

					if (luaL_loadstring(threadState, script.c_str())) {
						const char* error = lua_tostring(threadState, -1);
						LogLuaError("Async Syntax Error:", error);
						lua_pop(threadState, 2);
						return false;
					}

					if (lua_pcall(threadState, 0, 0, errHandlerIndex)) {
						const char* stackTrace = lua_tostring(threadState, -1);
						LogLuaError("Async Runtime Error:", stackTrace, true);
						lua_pop(threadState, 2);
						return false;
					}

					lua_pop(threadState, 1);
					result = true;
				}
			}
			catch (const std::exception& e) {
				LogLuaError("C++ Exception:", e.what());
				result = false;
			}
			catch (...) {
				LogLuaError("Unknown Error:", "An unexpected error occurred");
				result = false;
			}

			return result;
			});
	}

	std::future<bool> LuaVM::ExecuteFileAsync(const char* filename)
	{
		return threadPool->enqueue([this, filename = std::string(filename)]() {
			LuaStateGuard stateGuard(this);
			lua_State* threadState = stateGuard.get();
			bool result = false;

			try {
				if (threadState) {
					lua_pushcfunction(threadState, LuaErrorHandler);
					int errHandlerIndex = lua_gettop(threadState);

					if (luaL_loadfile(threadState, filename.c_str())) {
						const char* error = lua_tostring(threadState, -1);
						LogLuaError("Async file load error:", error);
						lua_pop(threadState, 2);
						return false;
					}

					if (lua_pcall(threadState, 0, 0, errHandlerIndex)) {
						const char* stackTrace = lua_tostring(threadState, -1);
						LogLuaError("Async runtime error in file:", stackTrace, true);
						lua_pop(threadState, 2);
						return false;
					}

					lua_pop(threadState, 1);
					result = true;
				}
			}
			catch (const std::exception& e) {
				LogLuaError("C++ exception in async file execution:", e.what());
				result = false;
			}
			catch (...) {
				LogLuaError("Unknown error in async file execution:", "");
				result = false;
			}

			return result;
			});
	}

	// API 函数实现
	namespace LuaAPI
	{
		

		int GetWindowSize(lua_State* L)
		{
			lua_pushnumber(L, g_Engine->mGameWidth);
			lua_pushnumber(L, g_Engine->mGameHeight);
			return 2;
		}

		int DrawText(lua_State* L)
		{
			const char* text = luaL_checkstring(L, 1);
			float x = (float)luaL_checknumber(L, 2);
			float y = (float)luaL_checknumber(L, 3);
			ImGui::GetForegroundDrawList()->AddText(
				ImVec2(x, y),
				IM_COL32_WHITE,
				text
			);
			return 0;
		}

		int DrawRect(lua_State* L)
		{
			float x = (float)luaL_checknumber(L, 1);
			float y = (float)luaL_checknumber(L, 2);
			float w = (float)luaL_checknumber(L, 3);
			float h = (float)luaL_checknumber(L, 4);
			ImGui::GetForegroundDrawList()->AddRect(
				ImVec2(x, y),
				ImVec2(x + w, y + h),
				IM_COL32_WHITE
			);
			return 0;
		}

		int GetMousePos(lua_State* L)
		{
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(g_Engine->pGameWindow, &p);
			lua_pushnumber(L, p.x);
			lua_pushnumber(L, p.y);
			return 2;
		}

		int RequireScript(lua_State* L)
		{
			LuaVM* vm = g_LuaVM.get();
			const char* moduleName = luaL_checkstring(L, 1);
			if (!moduleName) {
				vm->LogLuaError("Require Error:", "Module name expected");
				return luaL_error(L, "require: module name expected");
			}

			std::string modulePath = vm->FindModule(moduleName);
			if (modulePath.empty()) {
				vm->LogLuaError("Require Error:", "Module not found");
				return luaL_error(L, "require: module '%s' not found", moduleName);
			}

			if (vm->CheckCircularDependency(modulePath)) {
				vm->LogLuaError("Require Error:", "Circular dependency detected");
				return luaL_error(L, "require: circular dependency detected for module '%s'", moduleName);
			}

			// 检查是否已加载
			if (vm->IsModuleLoaded(modulePath)) {
				return 0;  // 已加载，直接返回
			}

			// 标记正在加载
			vm->loadingModules.insert(modulePath);

			// 压入错误处理函数
			lua_pushcfunction(L, LuaErrorHandler);
			int errHandlerIndex = lua_gettop(L);

			// 加载并执行文件
			if (luaL_loadfile(L, modulePath.c_str())) {
				const char* error = lua_tostring(L, -1);
				vm->LogLuaError("Module load error:", error);
				lua_pop(L, 2);
				vm->loadingModules.erase(modulePath);
				return luaL_error(L, "require: error loading module '%s'", moduleName);
			}

			if (lua_pcall(L, 0, LUA_MULTRET, errHandlerIndex)) {
				const char* stackTrace = lua_tostring(L, -1);
				vm->LogLuaError("Module runtime error:", stackTrace, true);
				lua_pop(L, 2);
				vm->loadingModules.erase(modulePath);
				return luaL_error(L, "require: error executing module '%s'", moduleName);
			}

			lua_pop(L, 1);

			// 清理并标记为已加载
			vm->loadingModules.erase(modulePath);
			vm->MarkModuleLoaded(modulePath);

			g_Console->cLog("\nModule loaded: %s\n", Console::EColor_green, moduleName);
			return 0;
		}
	}

	void LuaVM::LogLuaError(const char* prefix, const char* error, bool isStackTrace)
	{
		if (!error) return;

		// 使用一致的格式输出错误信息
		if (isStackTrace) {
			g_Console->cLog("\n[ERROR] %s\n", Console::EColor_red, prefix);
			g_Console->cLog("%s\n", Console::EColor_red, error);
		}
		else {
			g_Console->cLog("\n[ERROR] %s %s\n", Console::EColor_red, prefix, error);
		}
	}

	void LuaVM::RegisterFunctionToState(lua_State* state, const char* name, lua_CFunction func, const char* namespace_)
	{
		if (namespace_) {
			// 检查命名空间表是否存在
			lua_getglobal(state, namespace_);
			if (lua_isnil(state, -1)) {
				lua_pop(state, 1);
				lua_newtable(state);
				lua_setglobal(state, namespace_);
				lua_getglobal(state, namespace_);
			}
			
			// 将函数注册到命名空间表中
			lua_pushcfunction(state, func);
			lua_setfield(state, -2, name);
			lua_pop(state, 1);
		} else {
			// 直接注册到全局环境
			lua_pushcfunction(state, func);
			lua_setglobal(state, name);
		}
	}

	void LuaVM::RegisterFunction(const char* name, lua_CFunction func, const char* namespace_) {
		if (!isInitialized || !L) return;

		std::lock_guard<std::mutex> lock(luaMutex);
		
		// 在主状态中注册
		RegisterFunctionToState(L, name, func, namespace_);

		// 在所有线程状态中注册
		for (auto state : threadStates) {
			if (state) {
				RegisterFunctionToState(state, name, func, namespace_);
			}
		}
	}

}