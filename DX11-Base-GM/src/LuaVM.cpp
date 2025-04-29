#include <pch.h>
#include "LuaVM.h"
#include "Engine.h"
#include "Menu.h"
#include "Bridge.h"
#include <queue>
#include <algorithm>

namespace DX11Base
{
	LuaVM::LuaVM() : L(nullptr), isInitialized(false), 
					 scriptQueue(nullptr), queueMutex(nullptr), 
					 queueCondition(nullptr), threadRunning(nullptr)
	{
		// 移除线程池初始化
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

		try {
			// 收集所有参数
			std::vector<std::string> args;
			size_t totalLength = 0;

			for (int i = 1; i <= nargs; i++) {
				if (lua_isstring(L, i)) {
					size_t len;
					const char* str = lua_tolstring(L, i, &len);
					args.push_back(std::string(str, len));
					totalLength += len;
				}
				else {
					// 非字符串类型，转换为类型描述
					const char* typeName = lua_typename(L, lua_type(L, i));
					std::string typeStr = "<" + std::string(typeName) + ">";
					args.push_back(typeStr);
					totalLength += typeStr.length();
				}

				// 参数间添加分隔符
				if (i < nargs) {
					totalLength += 1; // 为\t预留空间
				}
			}

			// 构建完整字符串
			std::string fullText;
			fullText.reserve(totalLength);

			for (size_t i = 0; i < args.size(); i++) {
				fullText += args[i];
				if (i < args.size() - 1) {
					fullText += '\t';
				}
			}

			// 分片打印，但不显示分片指示
			const size_t CHUNK_SIZE = 4000; // 每次打印的最大字符数
			size_t position = 0;

			// 不再显示总长度提示
			
			// 连续打印，不显示分片指示
			while (position < fullText.length()) {
				// 计算当前片段长度
				size_t chunkLength = (CHUNK_SIZE < fullText.length() - position) ?
					CHUNK_SIZE : (fullText.length() - position);
				std::string chunk = fullText.substr(position, chunkLength);

				// 直接打印内容，不添加任何指示
				g_Console->cLog(chunk.c_str(), Console::EColors::EColor_white);

				position += chunkLength;
			}
			
			// 不再显示完成指示
		}
		catch (const std::exception& e) {
			g_Console->cLog("print 发生异常 %s", Console::EColors::EColor_red, e.what());
		}
		catch (...) {
			g_Console->cLog("print 未知错误", Console::EColors::EColor_red);
		}

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
		
		
		// 设置Lua全局变量中的内存限制参数
		lua_pushinteger(L, 1024 * 1024 * 1024);  // 1GB内存上限
		lua_setglobal(L, "MEMORY_LIMIT");
		lua_gc(L, LUA_GCSETPAUSE, 100);    // 当内存使用增长100%时暂停
		
		// 添加脚本搜索路径（放在 RegisterAPI 之前）
		std::string scriptsPath = CodeEditor::GetScriptsPath();
		
		// 获取程序根目录
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string rootPath = buffer;
		rootPath = rootPath.substr(0, rootPath.find_last_of("\\/"));
		g_Console->cLog(" rootPath :%s", Console::EColor_green, rootPath.c_str());
		// 添加多个搜索路径
		AddSearchPath((rootPath + "\\Scripts\\?.lua").c_str());  // Scripts 目录中的 .lua 文件
		AddSearchPath((rootPath + "\\Scripts\\?\\init.lua").c_str());  // Scripts 目录中的模块
		AddSearchPath((rootPath + "\\Scripts\\libs\\?.lua").c_str());  // 库文件
		AddSearchPath((rootPath + "\\Scripts\\modules\\?.lua").c_str());  // 模块文件


		// 注册 API 函数
		RegisterAPI();

		// 移除线程池创建代码

		isInitialized = true;
		return true;
	}


	// 新增方法：向指定状态添加搜索路径
	void LuaVM::AddSearchPathToState(lua_State* state, const char* path)
	{
		if (!state || !path) return;

		lua_getglobal(state, "package");
		lua_getfield(state, -1, "path");
		std::string cur_path = lua_tostring(state, -1);
		cur_path.append(";");
		cur_path.append(path);
		lua_pop(state, 1);
		lua_pushstring(state, cur_path.c_str());
		lua_setfield(state, -2, "path");
		lua_pop(state, 1);
	}

	// 修改添加搜索路径方法，移除多线程相关代码
	void LuaVM::AddSearchPath(const char* path)
	{
		if (isInitialized || !path) return;

		// 添加到主状态
		AddSearchPathToState(L, path);
		
		// 移除线程状态相关代码
		
		// 输出日志以便调试
		g_Console->cLog("Lua Search %s", Console::EColor_green, path);
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

	// 修改ExecuteString方法，移除多线程相关代码
	bool LuaVM::ExecuteString(const char* script)
	{
		if (!isInitialized || !script) return false;
		
		// 直接在主线程执行脚本
		lua_pushcfunction(L, LuaErrorHandler);
		int errHandlerIndex = lua_gettop(L);
		
		if (luaL_loadstring(L, script)) {
			const char* error = lua_tostring(L, -1);
			LogLuaError("Syntax error:", error);
			lua_pop(L, 2);
			return false;
		}

		if (lua_pcall(L, 0, 0, errHandlerIndex)) {
			const char* error = lua_tostring(L, -1);
			LogLuaError("Runtime error:", error, true);
			lua_pop(L, 2);
			return false;
		}

		lua_pop(L, 1);  // 弹出错误处理函数
		return true;
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
		// 获取当前工作目录
		char currentDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, currentDir);
		
		// 获取可执行文件目录
		char exeDir[MAX_PATH];
		GetModuleFileNameA(NULL, exeDir, MAX_PATH);
		std::string exePath = exeDir;
		exePath = exePath.substr(0, exePath.find_last_of("\\/"));
		
		// 设置搜索路径，优先搜索当前目录
		std::vector<std::string> searchPaths = {
			"./",  // 当前目录
			currentDir + std::string("\\"),  // 使用绝对路径
			exePath + "\\",  // 可执行文件目录
			CodeEditor::GetScriptsPath(),
			CodeEditor::GetScriptsPath() + "libs/",
			CodeEditor::GetScriptsPath() + "modules/"
		};

		// 替换模块名中的点为路径分隔符
		std::string modulePath = moduleName;
		std::replace(modulePath.begin(), modulePath.end(), '.', '\\');

		// 记录尝试过的路径，用于调试
		std::vector<std::string> triedPaths;

		for (const auto& basePath : searchPaths) {
			// 尝试不同的文件扩展名
			std::vector<std::string> attempts = {
				basePath + modulePath + ".lua",
				basePath + modulePath + "/init.lua",
				basePath + modulePath
			};

			for (const auto& path : attempts) {
				triedPaths.push_back(path);
				g_Console->cLog("Searching module at: %s", Console::EColor_yellow, path.c_str());
				if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
					g_Console->cLog("Module found: %s", Console::EColor_green, path.c_str());
					return path;
				}
			}
		}

		// 如果没找到，打印所有尝试过的路径
		g_Console->cLog("Module not found: %s", Console::EColor_red, moduleName.c_str());
		g_Console->cLog("Tried following paths:", Console::EColor_red);
		for (const auto& path : triedPaths) {
			g_Console->cLog("  %s", Console::EColor_red, path.c_str());
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

	void LuaVM::Shutdown()
	{
		// 停止Lua线程
		if (threadRunning) {
			threadRunning->store(false);
			if (queueCondition) queueCondition->notify_all();
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待线程退出
		}

		// 关闭Lua状态
		if (L) {
			lua_close(L);
			L = nullptr;
		}

		// 清理其他资源
		isInitialized = false;
		loadingModules.clear();
		loadedModules.clear();
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
		
		// 仅在主状态中注册
		RegisterFunctionToState(L, name, func, namespace_);
	}

	// 修改RegisterQueueAccessFunctions方法
	void LuaVM::RegisterQueueAccessFunctions(std::queue<std::string>* queue,
		std::mutex* mutex,
		std::condition_variable* condition,
		std::atomic<bool>* running)
	{
		scriptQueue = queue;
		queueMutex = mutex;
		queueCondition = condition;
		threadRunning = running;
	}

	bool LuaVM::QueueScriptForExecution(const char* script)
	{
		if (!script || !scriptQueue || !queueMutex || !queueCondition) return false;

		int queueSize = 0;
		{
			std::lock_guard<std::mutex> lock(*queueMutex);

			// 检查队列大小，避免堆积太多任务
			queueSize = scriptQueue->size();
			if (queueSize >= 5) { // 最多允许5个脚本在队列中
				g_Console->cLog("[-] 脚本队列已满 (%d 个脚本等待执行)，请稍后再试",
					Console::EColors::EColor_red, queueSize);
				return false;
			}

			scriptQueue->push(std::string(script));
			queueSize = scriptQueue->size();
		}

		g_Console->cLog("[+] 脚本已加入执行队列 (队列中还有 %d 个脚本)",
			Console::EColors::EColor_green, queueSize);

		queueCondition->notify_one();
		return true;
	}

	// 添加StartScriptProcessor方法实现
	void LuaVM::StartScriptProcessor()
	{
		if (!threadRunning) return;
		threadRunning->store(true);
		g_Console->cLog("[+] 脚本处理器已启动", Console::EColors::EColor_green);
	}

	// 添加StopScriptProcessor方法实现
	void LuaVM::StopScriptProcessor()
	{
		if (!threadRunning) return;
		threadRunning->store(false);
		if (queueCondition) {
			queueCondition->notify_all(); // 唤醒所有等待的线程
		}
		g_Console->cLog("[+] 脚本处理器已停止", Console::EColors::EColor_green);
	}
}