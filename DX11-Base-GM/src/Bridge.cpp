#include <pch.h>
#include "Bridge.h"
#include <offsets.h>
#include "Engine.h"

namespace DX11Base {

	// 静态成员变量的定义
	TPyEval_CallObjectWithKeywords Bridge::fpPyEval_CallObjectWithKeywords = nullptr;
	TPyObject_GetAttr Bridge::fpPyObject_GetAttr = nullptr;
	TPy_BuildValue Bridge::fpPy_BuildValue = nullptr;
	TPyImport_Import Bridge::fpPyImport_Import = nullptr;
	TPyGILState_Ensure Bridge::fpPyGILState_Ensure = nullptr;
	TPyGILState_Release Bridge::fpPyGILState_Release = nullptr;
	TPyObject_Str Bridge::fpPyObject_Str = nullptr;
	TPyTuple_New Bridge::fpPyTuple_New = nullptr;
	TPyObject_SetAttr Bridge::fpPyObject_SetAttr = nullptr;
	TPyDict_SetItem Bridge::fpPyDict_SetItem = nullptr;

	bool Bridge::isInitialized()
	{
		return isInitionalized;
	}


	//初始化桥
	bool Bridge::InitPyBridge() {
		if (isInitionalized) return isInitionalized;

		// 获取模块基址
		while (!m1logic4) {
			m1logic4 = (UINT64)GetModuleHandleA("m1logic4.dll");
			Sleep(1000);
		}
		if (!m1logic4) {
			g_Console->cLog("Failed to get m1logic4.dll base address\n", Console::EColor_red);
			return isInitionalized;
		}

		// 初始化函数指针
		fpPyEval_CallObjectWithKeywords = (TPyEval_CallObjectWithKeywords)(m1logic4 + Offset::offset_PyEval_CallObjectWithKeywords);
		fpPyObject_GetAttr = (TPyObject_GetAttr)(m1logic4 + Offset::offset_PyObject_GetAttr);
		fpPy_BuildValue = (TPy_BuildValue)(m1logic4 + Offset::offset_Py_BuildValue);
		fpPyImport_Import = (TPyImport_Import)(m1logic4 + Offset::offset_PyImport_Import);
		fpPyGILState_Ensure = (TPyGILState_Ensure)(m1logic4 + Offset::offset_PyGILState_Ensure);
		fpPyGILState_Release = (TPyGILState_Release)(m1logic4 + Offset::offset_PyGILState_Release);
		fpPyObject_Str = (TPyObject_Str)(m1logic4 + Offset::offset_PyObject_Str);
		fpPyTuple_New = (TPyTuple_New)(m1logic4 + Offset::offset_PyTuple_New);
		fpPyObject_SetAttr = (TPyObject_SetAttr)(m1logic4 + Offset::offset_PyObject_SetAttr);
		// 验证函数指针
		if (!fpPyEval_CallObjectWithKeywords || !fpPyObject_GetAttr ||  !fpPy_BuildValue || 
			!fpPyImport_Import || !fpPyGILState_Ensure || 
			!fpPyGILState_Release ) {
			g_Console->cLog("Failed to initialize Python bridge functions\n", Console::EColor_red);
			return isInitionalized;
		}

		
		g_Console->cLog("Python bridge function registering\n", Console::EColor_green);

		RegisterBridgeFunctions();
		RegisterPyObjectMetatables();
		

		isInitionalized = true;
		g_Console->cLog("Python bridge initialized successfully\n", Console::EColor_green);
		return isInitionalized;
	}

	void Bridge::RegisterPyObjectMetatables() {
		// 注册元表到主状态
		RegisterPyObjectMetatable(g_LuaVM->GetState());
		// 注册元表到异步执行状态
		const auto& threadStates = g_LuaVM->GetThreadStates();
		for (auto state : threadStates) {
			if (state) {
				RegisterPyObjectMetatable(state);
			}
		}
	}

	void Bridge::RegisterBridgeFunctions() {
		// 注册到主状态和所有线程状态
		g_LuaVM->RegisterFunction("buildValue", Bridge::BuildValue, "GMP");
		g_LuaVM->RegisterFunction("callObject", Bridge::CallObject, "GMP");
		g_LuaVM->RegisterFunction("getAttr", Bridge::GetAttr, "GMP");
		g_LuaVM->RegisterFunction("fromString", Bridge::FromString, "GMP");
		g_LuaVM->RegisterFunction("importModule", Bridge::ImportModule, "GMP");
		g_LuaVM->RegisterFunction("toString", Bridge::ToString, "GMP");

		// 注册新函数
		g_LuaVM->RegisterFunction("toLua", Bridge::ToLua, "GMP");
		g_LuaVM->RegisterFunction("toPython", Bridge::ToPython, "GMP");
	}

	// 辅助函数：检查数字是否为整数
	static bool isInteger(lua_Number n) {
		return n == floor(n);
	}


	// 辅助函数：将 Lua 值转换为 Python 对象
	PyObject* Bridge::LuaToPython(lua_State* L, int index) {
		switch (lua_type(L, index)) {
		case LUA_TNIL:
			Py_INCREF(Py_None);
			return Py_None;

		case LUA_TBOOLEAN:
			return fpPy_BuildValue("i", lua_toboolean(L, index));

		case LUA_TNUMBER: {
			lua_Number num = lua_tonumber(L, index);
			return isInteger(num) ? 
				fpPy_BuildValue("i", (long long)num) : 
				fpPy_BuildValue("d", (double)num);
		}

		case LUA_TSTRING: {
			const char* str = lua_tostring(L, index);
			return fpPy_BuildValue("s", str);
		}

		case LUA_TUSERDATA: {
			if (PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_testudata(L, index, "PyObject")) {
				if (wrapper->obj) {
					Py_INCREF(wrapper->obj);
					return wrapper->obj;
				}
			}
			return nullptr;
		}

		default:
			return nullptr;
		}
	}

	// 辅助函数：将 Python 对象转换为 Lua 值
	static void PythonToLua(lua_State* L, PyObject* obj) {
		if (!obj || obj == Py_None) {
			lua_pushnil(L);
		}
		else if (PyBool_Check(obj)) {
			lua_pushboolean(L, obj == Py_True);
		}
		else if (PyLong_Check(obj)) {
			lua_pushnumber(L, (lua_Number)PyLong_AsLongLong(obj));
		}
		else if (PyFloat_Check(obj)) {
			lua_pushnumber(L, (lua_Number)PyFloat_AsDouble(obj));
		}
		else if (PyUnicode_Check(obj)) {
			PyObject* bytes = PyUnicode_AsUTF8String(obj);
			if (bytes) {
				char* str;
				Py_ssize_t len;
				PyBytes_AsStringAndSize(bytes, &str, &len);
				lua_pushlstring(L, str, len);
				Py_DECREF(bytes);
			}
			else {
				lua_pushnil(L);
			}
		}
		else if (PyBytes_Check(obj)) {
			char* str;
			Py_ssize_t len;
			PyBytes_AsStringAndSize(obj, &str, &len);
			lua_pushlstring(L, str, len);
		}
		else {
			// 创建 Python 对象的包装
			PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
			wrapper->obj = obj;
			Py_INCREF(obj);  // 增加引用计数

			// 设置元表
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		}
	}

	// 元表方法实现
	int Bridge::py_object_gc(lua_State* L) {
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (wrapper->obj) {
			wrapper->obj = nullptr;
		}
		return 0;
	}

	// 处理属性访问（.操作符）
	int Bridge::py_object_index(lua_State* L) {
		g_Console->cLog("py_object_index: 开始属性访问", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			g_Console->cLog("py_object_index: 无效的Python对象", Console::EColor_red);
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 只处理字符串键的属性访问
		const char* key = luaL_checkstring(L, 2);
		g_Console->cLog("py_object_index: 处理属性访问: %s", Console::EColor_green, key);

		// 获取属性
		PyObject* attr_name = fpPy_BuildValue("s", key);
		if (!attr_name) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to create Python string");
		}

		PyObject* result = fpPyObject_GetAttr(wrapper->obj, attr_name);
		Py_DECREF(attr_name);

		if (result) {
			if (PyCallable_Check(result)) {
				g_Console->cLog("py_object_index: 返回可调用对象", Console::EColor_green);
				// 如果是方法，创建闭包
				PyObjectWrapper* methodWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
				methodWrapper->obj = result;
				luaL_getmetatable(L, "PyObject");
				lua_setmetatable(L, -2);
				lua_pushcclosure(L, py_object_call, 1);  // 创建闭包
			} else {
				g_Console->cLog("py_object_index: 返回普通值", Console::EColor_green);
				// 如果是普通值，直接返回
				PyObjectWrapper* newWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
				newWrapper->obj = result;
				luaL_getmetatable(L, "PyObject");
				lua_setmetatable(L, -2);
			}
		} else {
			PyErr_Clear();
			lua_pushnil(L);
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 处理下标访问（[]操作符）
	int Bridge::py_object_getitem(lua_State* L) {
		g_Console->cLog("py_object_getitem: 开始下标访问", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* result = nullptr;

		// 将键转换为Python对象
		PyObject* key = LuaToPython(L, 2);
		if (!key) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Invalid key type");
		}

		// 尝试序列访问
		if (PySequence_Check(wrapper->obj)) {
			if (PyLong_Check(key)) {
				Py_ssize_t index = PyLong_AsSsize_t(key) - 1;  // Lua 索引从 1 开始
				if (index >= 0) {
					result = PySequence_GetItem(wrapper->obj, index);
				}
			}
		}
		// 尝试字典访问
		else if (PyDict_Check(wrapper->obj)) {
			result = PyDict_GetItem(wrapper->obj, key);
			if (result) {
				Py_INCREF(result);  // GetItem 返回借用的引用
			}
		}

		Py_DECREF(key);

		if (result) {
			PyObjectWrapper* newWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
			newWrapper->obj = result;
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		} else {
			PyErr_Clear();
			lua_pushnil(L);
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

	int Bridge::py_object_call(lua_State* L) {
		g_Console->cLog("py_object_call: 开始执行", Console::EColor_green);
		
		PyObjectWrapper* methodWrapper = (PyObjectWrapper*)lua_touserdata(L, lua_upvalueindex(1));
		
		if (!methodWrapper || !methodWrapper->obj) {
			g_Console->cLog("py_object_call: 无效的Python对象", Console::EColor_red);
			return luaL_error(L, "Invalid Python method object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 创建参数元组
		int nargs = lua_gettop(L);
		PyObject* args = fpPyTuple_New(nargs);
		
		// 转换参数
		for (int i = 0; i < nargs; i++) {
			PyObject* arg;
			if (PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_testudata(L, i + 1, "PyObject")) {
				// 如果是 Python 对象，直接使用并增加引用计数
				arg = wrapper->obj;
				Py_INCREF(arg);
			} else {
				// 否则转换为 Python 对象
				arg = LuaToPython(L, i + 1);
			}

			if (!arg) {
				Py_DECREF(args);
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to convert argument %d", i + 1);
			}
			PyTuple_SET_ITEM(args, i, arg);
		}

		// 调用方法
		PyObject* result = fpPyEval_CallObjectWithKeywords(methodWrapper->obj, args, NULL);
		Py_DECREF(args);

		if (!result) {
			PyErr_Print();
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to call Python method");
		}

		// 返回结果并确保设置了正确的元表
		PyObjectWrapper* resultWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		resultWrapper->obj = result;
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);

		g_Console->cLog("py_object_call: 调用完成，返回结果", Console::EColor_green);
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 注册元表
	void Bridge::RegisterPyObjectMetatable(lua_State* L) {
		luaL_newmetatable(L, "PyObject");
		
		// 创建一个新的 C 闭包来处理 __index
		lua_pushcfunction(L, [](lua_State* L) -> int {
			// 检查是否是字符串键
			if (lua_type(L, 2) == LUA_TSTRING) {
				// 如果是字符串键，先尝试查找方法
				luaL_getmetatable(L, "PyObject");
				lua_getfield(L, -1, "__methods");
				lua_pushvalue(L, 2);  // 复制键名
				lua_gettable(L, -2);  // 在方法表中查找
				
				if (!lua_isnil(L, -1)) {
					return 1;  // 如果找到了方法，直接返回
				}
				
				// 如果不是方法，清理栈并进行属性访问
				lua_pop(L, 3);
				return py_object_index(L);
			} else {
				// 如果不是字符串键，使用 getitem 处理
				return py_object_getitem(L);
			}
		});
		lua_setfield(L, -2, "__index");
		
		// 设置其他元方法
		luaL_Reg methods[] = {
			{"__call", py_object_call},
			{"__gc", py_object_gc},
			{"__newindex", py_object_newindex},
			{"__len", py_object_len},
			{"__pairs", py_object_pairs},
			{NULL, NULL}
		};
		
		// 注册元方法
		luaL_setfuncs(L, methods, 0);
		
		// 创建方法表
		lua_newtable(L);
		
		// 在方法表中设置 getitem 方法（作为备用）
		lua_pushcfunction(L, py_object_getitem);
		lua_setfield(L, -2, "getitem");
		
		// 将方法表设置为 PyObject 的方法表
		lua_setfield(L, -2, "__methods");
		
		lua_pop(L, 1);  // 弹出元表
	}



	int Bridge::BuildValue(lua_State* L) {
		int nargs = lua_gettop(L);
		if (nargs < 1) {
			return luaL_error(L, "At least 1 argument required");
		}

		const char* format = luaL_checkstring(L, 1);
		if (!format) {
			return luaL_error(L, "Format string required as first argument");
		}

		// 处理格式字符串，将所有格式说明符替换为 O
		std::string newFormat = format;
		const char* formatChars = "diufFeEgGxXoscbaLvp";
		size_t formatCharsLen = strlen(formatChars);
		for (size_t i = 0; i < formatCharsLen; i++) {
			size_t pos = 0;
			while ((pos = newFormat.find(formatChars[i], pos)) != std::string::npos) {
				newFormat[pos] = 'O';
				pos++;
			}
		}
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* result = nullptr;

		try {
			// 收集参数
			std::vector<PyObject*> args;
			args.reserve(nargs - 1);

			// 将所有参数转换为 PyObject*
			for (int i = 2; i <= nargs; i++) {
				PyObject* arg = LuaToPython(L, i);
				if (!arg) {
					// 清理已收集的参数
					for (PyObject* obj : args) {
						Py_DECREF(obj);
					}
					fpPyGILState_Release(gstate);
					return luaL_error(L, "Failed to convert argument %d", i);
				}
				args.push_back(arg);
			}

			// 使用新的格式字符串
			switch (args.size()) {
				case 0:
					result = fpPy_BuildValue(newFormat.c_str());
					break;
				case 1:
					result = fpPy_BuildValue(newFormat.c_str(), args[0]);
					break;
				case 2:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1]);
					break;
				case 3:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2]);
					break;
				case 4:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3]);
					break;
				case 5:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4]);
					break;
				case 6:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5]);
					break;
				case 7:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
					break;
				case 8:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
					break;
				case 9:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
					break;
				case 10:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
					break;
				case 11:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10]);
					break;
				case 12:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11]);
					break;
				case 13:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12]);
					break;
				case 14:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13]);
					break;
				case 15:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14]);
					break;
				case 16:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14], args[15]);
					break;
				case 17:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14], args[15], args[16]);
					break;
				case 18:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17]);
					break;
				case 19:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18]);
					break;
				case 20:
					result = fpPy_BuildValue(newFormat.c_str(), args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9],
						args[10], args[11], args[12], args[13], args[14], args[15], args[16], args[17], args[18], args[19]);
					break;
				default:
					for (PyObject* obj : args) {
						Py_DECREF(obj);
					}
					fpPyGILState_Release(gstate);
					return luaL_error(L, "Too many arguments (maximum 20 supported)");
			}

			// 清理参数
			for (PyObject* obj : args) {
				Py_DECREF(obj);
			}

			if (!result) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to build Python value");
			}

			// 将结果转换为 Lua 值
			PythonToLua(L, result);
			Py_DECREF(result);
		}
		catch (...) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Error building Python value");
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 实现 PyEval_CallObjectWithKeywords
	int Bridge::CallObject(lua_State* L) {
		if (lua_gettop(L) < 2) {
			return luaL_error(L, "At least 2 arguments required (callable, args)");
		}

		// 获取可调用对象
		PyObjectWrapper* callable = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!callable || !callable->obj || !PyCallable_Check(callable->obj)) {
			return luaL_error(L, "First argument must be a callable Python object");
		}

		// 获取参数元组
		PyObjectWrapper* args = (PyObjectWrapper*)luaL_checkudata(L, 2, "PyObject");
		if (!args || !args->obj || !PyTuple_Check(args->obj)) {
			return luaL_error(L, "Second argument must be a Python tuple");
		}

		// 调用函数
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* result = nullptr;

		try {
			result = fpPyEval_CallObjectWithKeywords(callable->obj, args->obj, nullptr);
		}
		catch (...) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Error calling Python function");
		}

		if (!result) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Python function call failed");
		}

		// 转换返回值
		PythonToLua(L, result);

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 实现 PyObject_GetAttr
	int Bridge::GetAttr(lua_State* L) {
		if (lua_gettop(L) != 2) {
			return luaL_error(L, "Exactly 2 arguments required (object, attr_name)");
		}

		// 获取 Python 对象
		PyObjectWrapper* obj = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!obj || !obj->obj) {
			return luaL_error(L, "First argument must be a Python object");
		}

		// 获取属性名
		const char* attrName = luaL_checkstring(L, 2);
		if (!attrName) {
			return luaL_error(L, "Second argument must be a string");
		}

		// 获取属性
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* attr = nullptr;

		try {
			// 使用 fpPy_BuildValue 创建 Python 字符串
			PyObject* nameObj = fpPy_BuildValue("s", attrName);
			if (!nameObj) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to convert attribute name to Python string");
			}

			attr = fpPyObject_GetAttr(obj->obj, nameObj);
		}
		catch (...) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Error getting Python attribute");
		}

		if (!attr) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Attribute not found: %s", attrName);
		}

		// 转换属性值
		PythonToLua(L, attr);

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 实现 PyUnicode_FromString
	int Bridge::FromString(lua_State* L) {
		if (lua_gettop(L) != 1) {
			return luaL_error(L, "Exactly 1 argument required (string)");
		}

		const char* str = luaL_checkstring(L, 1);
		if (!str) {
			return luaL_error(L, "Argument must be a string");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* pyStr = nullptr;

		try {
			pyStr = fpPy_BuildValue("s", str);
		}
		catch (...) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Error converting string to Python unicode");
		}

		if (!pyStr) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to create Python string");
		}

		// 转换为 Lua 值
		PythonToLua(L, pyStr);

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 实现 PyImport_Import
	int Bridge::ImportModule(lua_State* L) {
		const char* name = luaL_checkstring(L, 1);
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 创建模块名称字符串对象
		PyObject* module_name = fpPy_BuildValue("s", name);
		if (!module_name) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to create module name string");
		}
		
		// 导入模块
		PyObject* module = fpPyImport_Import(module_name);
		Py_DECREF(module_name);
		
		if (!module) {
			PyErr_Print();
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to import module: %s", name);
		}
		
		// 创建模块包装器并设置元表
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = module;
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 实现 toString 函数
	int Bridge::ToString(lua_State* L) {
		if (lua_gettop(L) != 1) {
			return luaL_error(L, "Exactly 1 argument required");
		}

		// 如果是 Lua 原生类型，直接转换
		switch (lua_type(L, 1)) {
			case LUA_TNIL:
				lua_pushstring(L, "nil");
				return 1;
			case LUA_TBOOLEAN:
				lua_pushstring(L, lua_toboolean(L, 1) ? "true" : "false");
				return 1;
			case LUA_TNUMBER:
				if (isInteger(lua_tonumber(L, 1))) {
					lua_pushfstring(L, "%d", (int)lua_tonumber(L, 1));
				} else {
					lua_pushfstring(L, "%f", lua_tonumber(L, 1));
				}
				return 1;
			case LUA_TSTRING:
				lua_pushvalue(L, 1);  // 字符串直接返回
				return 1;
			case LUA_TTABLE: {
				lua_pushstring(L, "table");  // 简单返回类型名
				return 1;
			}
			case LUA_TFUNCTION: {
				lua_pushstring(L, "function");  // 简单返回类型名
				return 1;
			}
			case LUA_TUSERDATA: {
				// 检查是否是 Python 对象
				if (PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_testudata(L, 1, "PyObject")) {
					if (!wrapper->obj) {
						return luaL_error(L, "Invalid Python object");
					}

					PyGILState_STATE gstate = fpPyGILState_Ensure();
					try {
						// 直接使用 fpPyObject_Str
						PyObject* strResult = fpPyObject_Str(wrapper->obj);
						if (!strResult) {
							fpPyGILState_Release(gstate);
							return luaL_error(L, "Failed to convert Python object to string");
						}

						// 将 Python 字符串转换为 C 字符串
						const char* cstr = PyUnicode_AsUTF8(strResult);
						if (!cstr) {
							Py_DECREF(strResult);
							fpPyGILState_Release(gstate);
							return luaL_error(L, "Failed to convert Python string to C string");
						}

						// 将结果推入 Lua 栈
						lua_pushstring(L, cstr);
						Py_DECREF(strResult);

						fpPyGILState_Release(gstate);
						return 1;
					}
					catch (...) {
						fpPyGILState_Release(gstate);
						return luaL_error(L, "Error converting Python object to string");
					}
				}
				lua_pushstring(L, "userdata");  // 其他 userdata 类型
				return 1;
			}
			default:
				lua_pushfstring(L, "unknown type: %s", lua_typename(L, lua_type(L, 1)));
				return 1;
		}
	}
	// 将 Python 对象转换为 Lua 值

	int Bridge::ToLua(lua_State* L) {
		if (lua_gettop(L) != 1) {
			return luaL_error(L, "Exactly 1 argument required (python_object)");
		}

		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 根据 Python 对象类型进行转换
		if (PyBool_Check(wrapper->obj)) {
			lua_pushboolean(L, wrapper->obj == Py_True);
		}
		else if (PyLong_Check(wrapper->obj)) {
			lua_pushnumber(L, (lua_Number)PyLong_AsLongLong(wrapper->obj));
		}
		else if (PyFloat_Check(wrapper->obj)) {
			lua_pushnumber(L, (lua_Number)PyFloat_AsDouble(wrapper->obj));
		}
		else if (PyUnicode_Check(wrapper->obj)) {
			const char* str = PyUnicode_AsUTF8(wrapper->obj);
			if (str) {;
				lua_pushlstring(L, str, strlen(str));
			}
			else {
				lua_pushnil(L);
			}
		}
		else if (PyBytes_Check(wrapper->obj)) {
			char* str;
			Py_ssize_t len;
			PyBytes_AsStringAndSize(wrapper->obj, &str, &len);
			lua_pushlstring(L, str, len);
		}
		else if (PyList_Check(wrapper->obj)) {
			// 转换 Python 列表为 Lua 表
			lua_newtable(L);
			Py_ssize_t len = PyList_Size(wrapper->obj);
			for (Py_ssize_t i = 0; i < len; i++) {
				PyObject* item = PyList_GetItem(wrapper->obj, i);
				lua_pushinteger(L, i + 1);  // Lua 表索引从 1 开始
				PythonToLua(L, item);
				lua_settable(L, -3);
			}
		}
		else if (PyTuple_Check(wrapper->obj)) {
			// 转换 Python 元组为 Lua 表
			lua_newtable(L);
			Py_ssize_t len = PyTuple_Size(wrapper->obj);
			for (Py_ssize_t i = 0; i < len; i++) {
				PyObject* item = PyTuple_GetItem(wrapper->obj, i);
				lua_pushinteger(L, i + 1);
				PythonToLua(L, item);
				lua_settable(L, -3);
			}
		}
		else if (PyDict_Check(wrapper->obj)) {
			// 转换 Python 字典为 Lua 表
			lua_newtable(L);
			PyObject* key, * value;
			Py_ssize_t pos = 0;
			while (PyDict_Next(wrapper->obj, &pos, &key, &value)) {
				PythonToLua(L, key);    // 转换键
				PythonToLua(L, value);  // 转换值
				lua_settable(L, -3);
			}
		}
		else {
			// 保持为 Python 对象
			lua_pushvalue(L, 1);
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 将 Lua 值转换为 Python 对象
	int Bridge::ToPython(lua_State* L) {
		if (lua_gettop(L) != 1) {
			return luaL_error(L, "Exactly 1 argument required (lua_value)");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* result = nullptr;

		switch (lua_type(L, 1)) {
		case LUA_TNIL:
			result = Py_None;
			Py_INCREF(result);
			break;

		case LUA_TBOOLEAN:
			result = lua_toboolean(L, 1) ? Py_True : Py_False;
			Py_INCREF(result);
			break;

		case LUA_TNUMBER: {
			lua_Number num = lua_tonumber(L, 1);
			if (isInteger(num)) {
				result = PyLong_FromLongLong((long long)num);
			}
			else {
				result = PyFloat_FromDouble((double)num);
			}
			break;
		}

		case LUA_TSTRING: {
			size_t len;
			const char* str = lua_tolstring(L, 1, &len);
			result = PyUnicode_FromStringAndSize(str, len);
			break;
		}

		case LUA_TTABLE: {
			// 检查是否是数组形式的表
			bool isArray = true;
			size_t len = 0;
			lua_pushnil(L);
			while (lua_next(L, 1) != 0) {
				if (lua_type(L, -2) != LUA_TNUMBER ||
					!isInteger(lua_tonumber(L, -2)) ||
					lua_tonumber(L, -2) <= 0) {
					isArray = false;
				}
				len = max(len, (size_t)lua_tonumber(L, -2));
				lua_pop(L, 1);
			}

			if (isArray) {
				// 转换为 Python 列表
				result = PyList_New(len);
				for (size_t i = 1; i <= len; i++) {
					lua_rawgeti(L, 1, i);
					PyObject* item = LuaToPython(L, -1);
					if (!item) {
						Py_DECREF(result);
						lua_pop(L, 1);
						fpPyGILState_Release(gstate);
						return luaL_error(L, "Failed to convert list item at index %d", i);
					}
					PyList_SET_ITEM(result, i - 1, item);
					lua_pop(L, 1);
				}
			}
			else {
				// 转换为 Python 字典
				result = PyDict_New();
				lua_pushnil(L);
				while (lua_next(L, 1) != 0) {
					PyObject* key = LuaToPython(L, -2);
					PyObject* value = LuaToPython(L, -1);
					if (!key || !value) {
						Py_XDECREF(key);
						Py_XDECREF(value);
						Py_DECREF(result);
						lua_pop(L, 2);
						fpPyGILState_Release(gstate);
						return luaL_error(L, "Failed to convert table key or value");
					}
					fpPyDict_SetItem(result, key, value);
					Py_DECREF(key);
					Py_DECREF(value);
					lua_pop(L, 1);
				}
			}
			break;
		}

		case LUA_TUSERDATA: {
			// 如果已经是 Python 对象，直接返回
			if (PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_testudata(L, 1, "PyObject")) {
				if (wrapper->obj) {
					result = wrapper->obj;
					Py_INCREF(result);  // 增加引用计数但不输出日志
				}
			}
			break;
		}

		default:
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Unsupported Lua type: %s", lua_typename(L, lua_type(L, 1)));
		}

		if (!result) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to convert to Python object");
		}

		// 创建新的 Python 对象包装
		PyObjectWrapper* newWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		newWrapper->obj = result;

		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);

		fpPyGILState_Release(gstate);
		return 1;
	}

	// 处理属性设置（赋值操作）
	int Bridge::py_object_newindex(lua_State* L) {
		g_Console->cLog("py_object_newindex: 开始属性设置", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		int result = -1;
		if (lua_type(L, 2) == LUA_TSTRING) {
			// 处理属性赋值 (obj.attr = value)
			const char* key = lua_tostring(L, 2);
			PyObject* attr_name = fpPy_BuildValue("s", key);
			if (!attr_name) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to create Python string for attribute name");
			}

			PyObject* value = LuaToPython(L, 3);
			if (!value) {
				Py_DECREF(attr_name);
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to convert value to Python");
			}

			result = fpPyObject_SetAttr(wrapper->obj, attr_name, value);
			Py_DECREF(attr_name);
			Py_DECREF(value);
		} else {
			// 处理下标赋值 (obj[key] = value)
			PyObject* key = LuaToPython(L, 2);
			PyObject* value = LuaToPython(L, 3);

			if (!key || !value) {
				Py_XDECREF(key);
				Py_XDECREF(value);
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to convert key or value to Python");
			}

			if (PyDict_Check(wrapper->obj)) {
				// 字典赋值
				result = fpPyDict_SetItem(wrapper->obj, key, value);
			} else if (PySequence_Check(wrapper->obj) && PyLong_Check(key)) {
				// 序列赋值
				Py_ssize_t index = PyLong_AsSsize_t(key) - 1;  // Lua 索引从 1 开始
				if (index >= 0) {
					result = PySequence_SetItem(wrapper->obj, index, value);
				}
			}

			Py_DECREF(key);
			Py_DECREF(value);
		}

		if (result == -1) {
			PyErr_Clear();
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Failed to set Python object attribute/item");
		}

		fpPyGILState_Release(gstate);
		return 0;
	}

	// 处理长度操作（#操作符）
	int Bridge::py_object_len(lua_State* L) {
		g_Console->cLog("py_object_len: 开始获取长度", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		Py_ssize_t length = -1;
		if (PySequence_Check(wrapper->obj)) {
			length = PySequence_Length(wrapper->obj);
		} else if (PyDict_Check(wrapper->obj)) {
			length = PyDict_Size(wrapper->obj);
		}

		if (length == -1) {
			PyErr_Clear();
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Object does not support length operation");
		}

		lua_pushinteger(L, length);
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 处理迭代操作（pairs和ipairs函数）
	int Bridge::py_object_pairs(lua_State* L) {
		g_Console->cLog("py_object_pairs: 开始迭代", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* iter = nullptr;

		if (PyDict_Check(wrapper->obj)) {
			// 对于字典，使用 items() 方法
			g_Console->cLog("py_object_pairs: 迭代字典", Console::EColor_green);
			PyObject* items = PyDict_Items(wrapper->obj);
			if (!items) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to get dictionary items");
			}
			iter = PyObject_GetIter(items);
			Py_DECREF(items);
		} 
		else if (PyList_Check(wrapper->obj) || PyTuple_Check(wrapper->obj)) {
			// 对于列表和元组，创建枚举迭代器
			g_Console->cLog("py_object_pairs: 迭代序列", Console::EColor_green);
			iter = PyObject_GetIter(wrapper->obj);
			
			// 创建一个新表来存储当前索引
			lua_newtable(L);
			lua_pushinteger(L, 0);  // 初始索引
			lua_setfield(L, -2, "index");
			
			// 将这个表设置为迭代器的环境
			lua_setfenv(L, -2);
		}
		else {
			// 对于其他可迭代对象
			g_Console->cLog("py_object_pairs: 迭代其他对象", Console::EColor_green);
			iter = PyObject_GetIter(wrapper->obj);
		}

		if (!iter) {
			PyErr_Clear();
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Object is not iterable");
		}

		// 创建迭代器包装器
		PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		iterWrapper->obj = iter;
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);

		// 创建迭代函数闭包
		lua_pushcclosure(L, py_object_next, 1);
		lua_pushnil(L);  // 初始状态
		lua_pushnil(L);  // 初始值

		fpPyGILState_Release(gstate);
		return 3;
	}

	// 迭代器的 next 函数
	int Bridge::py_object_next(lua_State* L) {
		PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_touserdata(L, lua_upvalueindex(1));
		if (!iterWrapper || !iterWrapper->obj) {
			return luaL_error(L, "Invalid iterator");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();

		PyObject* item = PyIter_Next(iterWrapper->obj);
		if (!item) {
			if (PyErr_Occurred()) {
				PyErr_Clear();
			}
			fpPyGILState_Release(gstate);
			return 0;  // 迭代结束
		}

		if (PyTuple_Check(item) && PyTuple_Size(item) == 2) {
			// 对于字典的 items()，返回键值对
			PyObject* key = PyTuple_GetItem(item, 0);
			PyObject* value = PyTuple_GetItem(item, 1);
			PythonToLua(L, key);   // 转换键
			PythonToLua(L, value); // 转换值
			Py_DECREF(item);
		} else {
			// 对于列表和元组，返回索引和值
			lua_getfenv(L, 1);  // 获取环境表
			lua_getfield(L, -1, "index");
			lua_Integer index = lua_tointeger(L, -1) + 1;
			lua_pop(L, 1);  // 弹出旧索引
			
			lua_pushinteger(L, index);  // 设置新索引
			lua_setfield(L, -2, "index");
			lua_pop(L, 1);  // 弹出环境表
			
			lua_pushinteger(L, index);  // 返回索引（从1开始）
			PythonToLua(L, item);  // 返回值
			Py_DECREF(item);
		}

		fpPyGILState_Release(gstate);
		return 2;
	}
}