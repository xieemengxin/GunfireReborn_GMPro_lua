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
	TPyList_New Bridge::fpPyList_New = nullptr;
	TPyObject_SetAttr Bridge::fpPyObject_SetAttr = nullptr;
	TPyDict_SetItem Bridge::fpPyDict_SetItem = nullptr;
	Tnew_dict Bridge::fpnew_dict = nullptr;
	Tnew_keys_object Bridge::fpnew_keys_object = nullptr;
	TPyObject_SetItem Bridge::fpPyObject_SetItem = nullptr;
	TPyTuple_SetItem Bridge::fpPyTuple_SetItem = nullptr;


	


	bool Bridge::isInitialized()
	{
		return isInitionalized;
	}

	//构造一个自己的PyDict_New
	 #define PyDict_MINSIZE 8
	 PyObject* Bridge::fpPyDict_New() {
		PyDictKeysObject* keys = fpnew_keys_object(PyDict_MINSIZE);
		if (keys == NULL)
			return NULL;
		return fpnew_dict(keys, NULL);
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
		fpPyList_New = (TPyList_New)(m1logic4 + Offset::offset_PyList_New);
		fpnew_dict = (Tnew_dict)(m1logic4 + Offset::offset_new_dict);
		fpnew_keys_object = (Tnew_keys_object)(m1logic4 + Offset::offset_new_keys_object);
		fpPyObject_SetAttr = (TPyObject_SetAttr)(m1logic4 + Offset::offset_PyObject_SetAttr);
		fpPyDict_SetItem = (TPyDict_SetItem)(m1logic4 + Offset::offset_PyDict_SetItem);
		fpPyObject_SetItem = (TPyObject_SetItem)(m1logic4 + Offset::offset_PyObject_SetItem);
		fpPyTuple_SetItem = (TPyTuple_SetItem)(m1logic4 + Offset::offset_PyTuple_SetItem);

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
		// 移除对线程状态的引用
	}

	void Bridge::RegisterBridgeFunctions() {
		// 只在主状态中注册函数
		g_LuaVM->RegisterFunction("buildValue", Bridge::BuildValue, "GMP");
		g_LuaVM->RegisterFunction("callObject", Bridge::CallObject, "GMP");
		g_LuaVM->RegisterFunction("getAttr", Bridge::GetAttr, "GMP");
		g_LuaVM->RegisterFunction("fromString", Bridge::FromString, "GMP");
		g_LuaVM->RegisterFunction("importModule", Bridge::ImportModule, "GMP");
		g_LuaVM->RegisterFunction("toString", Bridge::ToString, "GMP");

		// 添加新的函数注册
		g_LuaVM->RegisterFunction("toPyBytes", toPyBytes, "GMP");
		g_LuaVM->RegisterFunction("toPyAuto", toPyAuto, "GMP");

		// 添加Python风格的元组和列表构造函数
		g_LuaVM->RegisterFunction("tuple", createPyTuple, "GMP");
		g_LuaVM->RegisterFunction("list", createPyList, "GMP");

		// 添加Python风格的字典构造函数
		g_LuaVM->RegisterFunction("dict", createPyDict, "GMP");

		// 注册setattr函数
		g_LuaVM->RegisterFunction("setattr", py_object_set_attr, "GMP");



		g_LuaVM->RegisterFunction("dictItems", py_dict_iterate, "GMP");
		g_LuaVM->RegisterFunction("listItems", py_list_iterate, "GMP");
		g_LuaVM->RegisterFunction("tupleItems", py_tuple_iterate, "GMP");
		g_LuaVM->RegisterFunction("items", py_iterate, "GMP");
		g_LuaVM->RegisterFunction("pyType", py_get_type, "GMP");
	}

	// 辅助函数：检查数字是否为整数
	static bool isInteger(lua_Number n) {
		return n == floor(n);
	}


	// 辅助函数：将 Lua 值转换为 Python 对象
	PyObject* Bridge::LuaToPython(lua_State* L, int index) {
		// 获取 Gil 锁
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* result = nullptr;

		// 首先检查是否已经是 PyObject
		if (lua_isuserdata(L, index)) {
			if (lua_getmetatable(L, index)) {  // 有元表
				luaL_getmetatable(L, "PyObject");  // 获取 PyObject 元表
				if (lua_rawequal(L, -1, -2)) {  // 比较两个元表
					// 是 PyObject，直接获取并增加引用计数
					PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, index);
					if (wrapper && wrapper->obj) {

						result = wrapper->obj;
					}
				}
				lua_pop(L, 2);  // 弹出两个元表
				
				if (result) {  // 如果找到了 PyObject，直接返回
					fpPyGILState_Release(gstate);
					return result;
				}
			}
		}

		int type = lua_type(L, index);
		switch (type) {
		case LUA_TNIL:

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
			size_t len;
			const char* str = lua_tolstring(L, index, &len);
			
			// 检查是否应该转换为 bytes
			bool isBinary = false;
			for (size_t i = 0; i < len; i++) {
				if (str[i] == 0 || (unsigned char)str[i] > 127) {
					isBinary = true;
					break;
				}
			}
			
			if (isBinary) {
				// 如果包含 null 字节或非 ASCII 字符，视为二进制数据
				result = PyBytes_FromStringAndSize(str, len);
			} else {
				// 普通文本字符串
				result = fpPy_BuildValue("s", str);
				if (!result) {
					// 如果 UTF-8 解码失败，回退到 bytes
					
					result = PyBytes_FromStringAndSize(str, len);
				}
			}
			break;
		}

		case LUA_TUSERDATA: {
			if (PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_testudata(L, index, "PyObject")) {
				if (wrapper->obj) {
					return wrapper->obj;
				}
			}
			return nullptr;
		}

		case LUA_TTABLE: {
			// 检查是否是数组
			size_t len = lua_objlen(L, index);
			bool isArray = true;
			
			// 检查是否所有键都是连续的数字
			for (size_t i = 1; i <= len; i++) {
				lua_rawgeti(L, index, i);
				if (lua_isnil(L, -1)) {
					isArray = false;
					lua_pop(L, 1);
					break;
				}
				lua_pop(L, 1);
			}
			
			if (isArray && len > 0) {
				// 转换为 Python 列表
				result = fpPyList_New(len);
				if (!result) {
					fpPyGILState_Release(gstate);
					return nullptr;
				}
				
				for (size_t i = 1; i <= len; i++) {
					lua_rawgeti(L, index, i);
					PyObject* item = LuaToPython(L, -1);
					lua_pop(L, 1);
					
					if (!item) {
						//Py_DECREF(result);
						fpPyGILState_Release(gstate);
						return nullptr;
					}
					
					PyList_SetItem(result, i-1, item);  // 这会偷取 item 的引用
				}
			}
			else {
				// 转换为 Python 字典
				result = fpPyDict_New();
				if (!result) {
					fpPyGILState_Release(gstate);
					return nullptr;
				}
				
				lua_pushnil(L);  // 第一个键
				while (lua_next(L, index) != 0) {
					// 键在索引 -2，值在索引 -1
					
					PyObject* pyKey = LuaToPython(L, -2);
					if (!pyKey) {
						//Py_DECREF(result);
						lua_pop(L, 1);  // 弹出值
						fpPyGILState_Release(gstate);
						return nullptr;
					}
					
					PyObject* pyValue = LuaToPython(L, -1);
					if (!pyValue) {
						//Py_DECREF(pyKey);
						//Py_DECREF(result);
						lua_pop(L, 1);  // 弹出值
						fpPyGILState_Release(gstate);
						return nullptr;
					}
					
					if (fpPyDict_SetItem(result, pyKey, pyValue) < 0) {
						//Py_DECREF(pyKey);
						//Py_DECREF(pyValue);
						//Py_DECREF(result);
						lua_pop(L, 1);  // 弹出值
						fpPyGILState_Release(gstate);
						return nullptr;
					}
					
					//Py_DECREF(pyKey);
					//Py_DECREF(pyValue);
					lua_pop(L, 1);  // 弹出值，保留键用于下一次迭代
				}
			}
			break;
		}

		default:
			return nullptr;
		}

		fpPyGILState_Release(gstate);
		return result;
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
				//Py_DECREF(bytes);
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


			// 设置元表
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		}
	}

	// 元表方法实现
	int Bridge::py_object_gc(lua_State* L) {
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (wrapper) {
			PyGILState_STATE gstate = fpPyGILState_Ensure();
			
			// 释放Python对象
			if (wrapper->obj) {
				//Py_DECREF(wrapper->obj);
				wrapper->obj = NULL;
			}
			
			// 释放parent对象
			if (wrapper->parent) {
				//Py_DECREF(wrapper->parent);
				wrapper->parent = NULL;
			}
			
			fpPyGILState_Release(gstate);
		}
		return 0;
	}

	// 处理属性访问（.操作符）
	int Bridge::py_object_index(lua_State* L) {
		//g_Console->cLog("py_object_index: 开始属性访问", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "无效的Python对象");
		}
		
		// 获取属性名
		const char* key = luaL_checkstring(L, 2);
		//g_Console->cLog("py_object_index: 处理属性访问: %s", Console::EColor_green, key);
		
		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 构建属性名的Python字符串
		PyObject* pyKey = fpPy_BuildValue("s", key);
		if (!pyKey) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "创建属性名失败");
		}
		
		// 获取属性
		PyObject* result = nullptr;
		if (PyDict_Check(wrapper->obj))
		{
			result = PyDict_GetItem(wrapper->obj, pyKey);
		}
		else 
		{
			result = fpPyObject_GetAttr(wrapper->obj, pyKey);
		}

		
		//Py_DECREF(pyKey);
		
		if (!result) {
			PrintPythonError("获取属性");
				fpPyGILState_Release(gstate);
			return luaL_error(L, "获取属性 '%s' 失败", key);
		}
		
		// 检查是否是可调用对象
		int isCallable = PyCallable_Check(result);
		
		if (isCallable) {
			//g_Console->cLog("py_object_index: 返回可调用对象", Console::EColor_green);
			
			// 创建一个用户数据用于存储方法
			PyObjectWrapper* methodWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
			
			// 重要：设置parent字段为对象本身
			methodWrapper->parent = wrapper->obj;
			////Py_INCREF(wrapper->obj);  // 增加引用计数，因为我们保存了引用
			
			methodWrapper->obj = result;  // 不需要增加引用计数，因为已经是新引用
			
			// 设置元表
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
			
			// 打印调试信息
			//g_Console->cLog("py_object_index: 设置parent=%p, obj=%p", Console::EColor_green, methodWrapper->parent, methodWrapper->obj);
		} else {
			//g_Console->cLog("py_object_index: 返回普通值", Console::EColor_green);
			
			// 创建一个用户数据用于存储结果
			PyObjectWrapper* resultWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
			resultWrapper->parent = NULL;  // 非方法没有parent
			resultWrapper->obj = result;   // 不需要增加引用计数，因为已经是新引用
			
			// 设置元表
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		}
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 专门处理下标访问的函数
	int Bridge::py_object_getitem(lua_State* L) {
		//g_Console->cLog("py_object_getitem: 开始下标访问", Console::EColor_green);

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
				////Py_INCREF(result);  // GetItem 返回借用的引用
			}
		}

		//Py_DECREF(key);

		if (result) {
			PyObjectWrapper* newWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
			newWrapper->obj = result;
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		}
		else {
			
			lua_pushnil(L);
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

	int Bridge::py_object_call(lua_State* L) {
		//g_Console->cLog("py_object_call: 开始执行 堆栈大小：%d", Console::EColor_green, lua_gettop(L));
		
		// 检查对象是否有效
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "调用的对象无效");
		}
		//g_Console->cLog("py_object_call: parent %s , obj :%s,objtype %s", Console::EColor_green, PyUnicode_AsUTF8(fpPyObject_Str(wrapper->parent)), PyUnicode_AsUTF8(fpPyObject_Str(wrapper->obj)), wrapper->obj->ob_type->tp_name);
		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 获取用户参数数量（减去函数对象本身）
		int nargs = lua_gettop(L) - 1 ;


		// 创建参数元组
		PyObject* args =  fpPyTuple_New(nargs);
	
		
			// 填充参数元组
		for (int i = 0; i < nargs; i++) {
				// Lua栈索引从2开始（1是函数对象）
				PyObject* arg = LuaToPython(L, i + 2);
				if (!arg) {
					//Py_DECREF(args);
					fpPyGILState_Release(gstate);
					return luaL_error(L, "参数 #%d 转换失败", i + 1);
				}

				// 设置到元组中
				fpPyTuple_SetItem(args, i, arg);
		}
		
			
		// 打印调用信息
		PyObject* strArgs = fpPyObject_Str(args);
		if (strArgs) {
			//g_Console->cLog("py_object_call: 调用入参 %s", Console::EColor_green, PyUnicode_AsUTF8(strArgs));
			//Py_DECREF(strArgs);
		}
		
		// 调用函数
		PyObject* result = fpPyEval_CallObjectWithKeywords(wrapper->obj, args, NULL);
		
		// 检查调用结果
			if (!result) {
			PrintPythonError("函数调用");
				fpPyGILState_Release(gstate);
			return luaL_error(L, "函数调用失败");
		}
		
		// 打印结果信息
		PyObject* strResult = fpPyObject_Str(result);
		if (strResult) {
			const char* resultStr = PyUnicode_AsUTF8(strResult);
			//g_Console->cLog("py_object_call: 调用完成，返回结果 %s", Console::EColor_green, resultStr ? resultStr : "无法显示");
		}
		
		// 将Python结果转换回Lua
		lua_settop(L, 0);  // 清空栈
		PyObjectWrapper* resultWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		resultWrapper->obj = result;  // 不增加引用计数，因为result已经是新引用
		
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;  // 返回结果
	}

	// 注册元表
	void Bridge::RegisterPyObjectMetatable(lua_State* L) {
		//// 创建元表
		//luaL_newmetatable(L, "PyObject");
		//
		//// 设置元方法
		//
		//// 为属性访问和下标访问设置不同的处理函数
		//lua_pushcfunction(L, [](lua_State* L) -> int {
		//	// 检查是否是数字键
		//	if (lua_type(L, 2) == LUA_TNUMBER) {
		//		// 如果是数字键，使用getitem处理
		//		return py_object_getitem(L);
		//	} else {
		//		// 如果是属性访问，使用index处理
		//		return py_object_index(L);
		//	}
		//});
		//lua_setfield(L, -2, "__index");
		//
		//// 其他元方法保持不变
		//lua_pushcfunction(L, py_object_newindex);
		//lua_setfield(L, -2, "__newindex");
		//
		//lua_pushcfunction(L, py_object_gc);
		//lua_setfield(L, -2, "__gc");
		//
		//lua_pushcfunction(L, py_object_call);
		//lua_setfield(L, -2, "__call");
		//
		//lua_pushcfunction(L, py_object_len);
		//lua_setfield(L, -2, "__len");
		//
		//lua_pushcfunction(L, py_object_pairs);
		//lua_setfield(L, -2, "__pairs");
		//
		//// 弹出元表
		//lua_pop(L, 1);
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
			}
			else {
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
						//Py_DECREF(obj);
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
						//Py_DECREF(obj);
					}
					fpPyGILState_Release(gstate);
					return luaL_error(L, "Too many arguments (maximum 20 supported)");
			}

			// 清理参数
			for (PyObject* obj : args) {
				//Py_DECREF(obj);
			}

			if (!result) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to build Python value");
			}

			// 将结果转换为 Lua 值
			PythonToLua(L, result);
			//Py_DECREF(result);
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
		//Py_DECREF(module_name);
		
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
	// 安全的字符串缓冲区实现
	class SafeStringBuffer {
	private:
		char* buffer;
		size_t capacity;
		size_t size;

		static const size_t INITIAL_SIZE = 1024;
		static const size_t MAX_SIZE = 1024 * 1024 * 10; // 10MB上限

	public:
		SafeStringBuffer() : buffer(nullptr), capacity(0), size(0) {
			// 初始分配
			buffer = (char*)malloc(INITIAL_SIZE);
			if (buffer) {
				capacity = INITIAL_SIZE;
				buffer[0] = '\0';
			}
		}

		~SafeStringBuffer() {
			if (buffer) {
				free(buffer);
				buffer = nullptr;
			}
		}

		// 禁用拷贝构造和赋值
		SafeStringBuffer(const SafeStringBuffer&) = delete;
		SafeStringBuffer& operator=(const SafeStringBuffer&) = delete;

		// 追加字符串，返回是否成功
		bool append(const char* str, size_t len) {
			if (!buffer || !str || len == 0) return false;

			// 检查是否需要扩展缓冲区
			if (size + len + 1 > capacity) {
				size_t newCapacity = capacity * 2;
				while (size + len + 1 > newCapacity) {
					newCapacity *= 2;
				}

				// 防止超过最大限制
				if (newCapacity > MAX_SIZE) {
					if (size + len + 1 > MAX_SIZE) {
						// 追加指示字符串已被截断
						const char* truncMsg = "...(截断)";
						size_t truncLen = strlen(truncMsg);

						// 确保有足够空间放截断信息
						if (MAX_SIZE > size + truncLen + 1) {
							char* newBuffer = (char*)realloc(buffer, MAX_SIZE);
							if (!newBuffer) return false;

							buffer = newBuffer;
							capacity = MAX_SIZE;

							// 计算可以复制的原字符串部分长度
							size_t copyLen = MAX_SIZE - size - truncLen - 1;
							memcpy(buffer + size, str, copyLen);
							size += copyLen;

							// 添加截断信息
							memcpy(buffer + size, truncMsg, truncLen);
							size += truncLen;
							buffer[size] = '\0';
						}
						return false;
					}
					newCapacity = MAX_SIZE;
				}

				// 重新分配内存
				char* newBuffer = (char*)realloc(buffer, newCapacity);
				if (!newBuffer) return false;

				buffer = newBuffer;
				capacity = newCapacity;
			}

			// 追加字符串
			memcpy(buffer + size, str, len);
			size += len;
			buffer[size] = '\0';

			return true;
		}

		// 获取字符串
		const char* str() const {
			return buffer ? buffer : "";
		}

		// 获取长度
		size_t length() const {
			return size;
		}

		// 清空缓冲区
		void clear() {
			if (buffer) {
				buffer[0] = '\0';
				size = 0;
			}
		}
	};

	// 安全的toString实现
	int Bridge::ToString(lua_State* L) {
		if (lua_gettop(L) != 1) {
			lua_pushstring(L, "toString需要一个参数");
			return 1;
		}

		// 创建安全字符串缓冲区
		SafeStringBuffer buffer;

		// 使用try-catch包装所有操作，确保不崩溃
		try {
			// 获取参数类型
			int type = lua_type(L, 1);

			// 处理基本类型
			switch (type) {
			case LUA_TNIL:
				buffer.append("nil", 3);
				break;

			case LUA_TBOOLEAN:
				if (lua_toboolean(L, 1)) {
					buffer.append("true", 4);
				}
				else {
					buffer.append("false", 5);
				}
				break;

			case LUA_TNUMBER: {
				char numStr[64];
				lua_Number num = lua_tonumber(L, 1);
				snprintf(numStr, sizeof(numStr), "%g", num);
				buffer.append(numStr, strlen(numStr));
				break;
			}

			case LUA_TSTRING: {
				
				size_t len;
				const char* str = lua_tolstring(L, 1, &len);
				buffer.append(str, len);
				break;
			}

			case LUA_TTABLE: {
				// 简单处理表，避免过于复杂的操作
				buffer.append("{表对象}", 9);
				break;
			}

			case LUA_TUSERDATA: {
				// 尝试识别PyObject类型
				if (luaL_testudata(L, 1, "PyObject")) {
					PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, 1);
					if (!wrapper || !wrapper->obj) {
						buffer.append("{无效的Python对象}", 22);
						break;
					}

					PyGILState_STATE gstate = fpPyGILState_Ensure();
						// 对于其他类型，尝试获取字符串表示
						PyObject* str = NULL;

						str = fpPyObject_Str(wrapper->obj);

						if (str) {
							const char* cstr = PyUnicode_AsUTF8(str);
							if (cstr) {
								// 获取完整字符串长度
								size_t len = strlen(cstr);
								
								// 不限制长度，直接附加完整字符串
								// 由 SafeStringBuffer 的内部逻辑自动处理大字符串
								buffer.append(cstr, len);
							}
							else {
								buffer.append("{无法转换为UTF-8字符串}", 27);
							}
							//Py_DECREF(str);
						}
						else {
							
							// 如果无法获取字符串表示，使用类型名
							const char* typeName = wrapper->obj->ob_type->tp_name;
							buffer.append("{Python对象: ", 14);
							buffer.append(typeName, strlen(typeName));
							buffer.append("}", 1);
						}
					

					fpPyGILState_Release(gstate);
				}
				else {
					// 其他userdata类型
					buffer.append("<非Python userdata>", 19);
				}
				break;
			}

			case LUA_TFUNCTION:
				buffer.append("{函数}", 7);
				break;

			case LUA_TTHREAD:
				buffer.append("{线程}", 7);
				break;

			default: {
				const char* typeName = lua_typename(L, type);
				buffer.append("<", 1);
				buffer.append(typeName, strlen(typeName));
				buffer.append(">", 1);
				break;
			}
			}

			// 返回结果
			lua_pushstring(L, buffer.str());
			return 1;
		}
		catch (const std::exception& e) {
			// 捕获C++异常
			lua_pushfstring(L, "<异常: %s>", e.what());
			return 1;
		}
		catch (...) {
			// 捕获所有其他异常
			lua_pushstring(L, "<未知异常>");
			return 1;
		}
	}


	// 处理属性设置（赋值操作）
	int Bridge::py_object_newindex(lua_State* L) {
		//g_Console->cLog("py_object_newindex: 开始属性设置", Console::EColor_green);
		
		// 检查并获取 Python 对象（第一个参数）
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			//g_Console->cLog("错误: 无效的 PyObject", Console::EColor_red);
			return luaL_error(L, "参数 #1 必须是有效的 PyObject");
		}
		
		// 检查属性名（第二个参数）
		if (!lua_isstring(L, 2)) {
			//g_Console->cLog("错误: 属性名必须是字符串", Console::EColor_red);
			return luaL_error(L, "参数 #2 必须是字符串属性名");
		}
		
			const char* key = lua_tostring(L, 2);
		//g_Console->cLog("py_object_newindex: 属性名 '%s'", Console::EColor_green, key);
		
		// 首先确定值的类型（第三个参数）
		int valueType = lua_type(L, 3);
		//g_Console->cLog("py_object_newindex: 值类型 %s", Console::EColor_green, lua_typename(L, valueType));
		
		// 获取 GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 创建属性名 PyObject
		PyObject* pyKey = fpPy_BuildValue("s", key);
		if (!pyKey) {
			//g_Console->cLog("错误: 创建属性名失败", Console::EColor_red);
				fpPyGILState_Release(gstate);
			return luaL_error(L, "创建 Python 属性名失败");
		}
		
		// 根据值类型选择处理方式
		PyObject* pyValue = nullptr;
		
		switch (valueType) {
		case LUA_TNIL:
			// nil 值转换为 Python None
			////Py_INCREF(Py_None);
			pyValue = Py_None;
			//g_Console->cLog("py_object_newindex: nil -> None", Console::EColor_green);
			break;

		case LUA_TBOOLEAN:
			// 布尔值转换为 Python True/False
			if (lua_toboolean(L, 3)) {
				////Py_INCREF(Py_True);
				pyValue = Py_True;
			}
			else {
				////Py_INCREF(Py_False);
				pyValue = Py_False;
			}
			//g_Console->cLog("py_object_newindex: boolean -> %s", Console::EColor_green,lua_toboolean(L, 3) ? "True" : "False");
			break;

		case LUA_TNUMBER:
			// 数字类型 - 不使用 lua_isinteger，直接转换为双精度
		{
			lua_Number n = lua_tonumber(L, 3);
			pyValue = fpPy_BuildValue("d", n);
			//g_Console->cLog("py_object_newindex: number %f", Console::EColor_green, n);
		}
		break;

		case LUA_TSTRING:
			// 字符串类型
		{
			size_t len;
			const char* s = lua_tolstring(L, 3, &len);
			pyValue = fpPy_BuildValue("s", s);
			//g_Console->cLog("py_object_newindex: string \"%s\"", Console::EColor_green, s);
		}
		break;

		case LUA_TUSERDATA:
			// 检查是否是 PyObject
		{
			//g_Console->cLog("检查是否是PyObject...", Console::EColor_green);

			if (!luaL_testudata(L, 3, "PyObject")) {
				//g_Console->cLog("值不是PyObject，尝试通用转换", Console::EColor_yellow);
				pyValue = LuaToPython(L, 3);
				break;
			}

			PyObjectWrapper* valuewrapper = (PyObjectWrapper*)lua_touserdata(L, 3);
			if (!valuewrapper) {
				//g_Console->cLog("获取userdata失败", Console::EColor_red);
				break;
			}

			if (!valuewrapper->obj) {
				//g_Console->cLog("PyObject为空", Console::EColor_red);
				break;
			}

			pyValue = valuewrapper->obj;

			// 检查对象类型，确保它是有效的
			//g_Console->cLog("获取对象类型信息...", Console::EColor_green);
			PyObject* objType = PyObject_Type(pyValue);
			if (objType) {
				PyObject* typeName = fpPyObject_GetAttr(objType, fpPy_BuildValue("s", "__name__"));
				if (typeName) {
					const char* typeStr = PyUnicode_AsUTF8(typeName);
					//g_Console->cLog("PyObject类型: %s", Console::EColor_green, typeStr ? typeStr : "未知");
					//Py_DECREF(typeName);
				}
				//Py_DECREF(objType);
			}

			// 尝试打印对象内容
			PyObject* strRep = fpPyObject_Str(pyValue);
			if (strRep) {
				const char* strValue = PyUnicode_AsUTF8(strRep);
				//g_Console->cLog("PyObject值: %s", Console::EColor_green, strValue ? strValue : "无法显示");
				//Py_DECREF(strRep);
			}
			else {
				//g_Console->cLog("无法获取PyObject字符串表示", Console::EColor_yellow);
				
			}

			break;
		}

		case LUA_TTABLE:
			// 表类型，使用通用转换
		{	//g_Console->cLog("py_object_newindex: 表类型，使用通用转换", Console::EColor_green);
			pyValue = LuaToPython(L, 3);
			break;
		}
				
		default:
			// 其他类型，尝试通用转换
		{
			//g_Console->cLog("py_object_newindex: 其他类型 %s，尝试通用转换", Console::EColor_yellow,lua_typename(L, valueType));
			pyValue = LuaToPython(L, 3);
			break;
		}
		}
		
		// 检查值是否转换成功
		if (!pyValue) {
			//g_Console->cLog("错误: 值转换失败", Console::EColor_red);
			//Py_DECREF(pyKey);
			fpPyGILState_Release(gstate);
			return luaL_error(L, "无法将值转换为 Python 对象");
		}
		
		// 设置属性
		//g_Console->cLog("py_object_newindex: 执行 SetAttr", Console::EColor_green);
		int result = fpPyObject_SetAttr(wrapper->obj, pyKey, pyValue);
		
		// 清理引用
		//Py_DECREF(pyKey);
		//Py_DECREF(pyValue);
		
		if (result < 0) {
			//g_Console->cLog("错误: SetAttr 失败", Console::EColor_red);
			PrintPythonError("SetAttr");
			
				fpPyGILState_Release(gstate);
			return luaL_error(L, "设置 Python 对象属性 '%s' 失败", key);
		}
		
		fpPyGILState_Release(gstate);
		//g_Console->cLog("py_object_newindex: 属性设置成功", Console::EColor_green);
		return 0;
	}

	// 处理长度操作（#操作符）
	int Bridge::py_object_len(lua_State* L) {
		//g_Console->cLog("py_object_len: 开始获取长度", Console::EColor_green);
		
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
			
			fpPyGILState_Release(gstate);
			return luaL_error(L, "Object does not support length operation");
		}

		lua_pushinteger(L, length);
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 处理迭代操作（pairs和ipairs函数）
	int Bridge::py_object_pairs(lua_State* L) {
		//g_Console->cLog("py_object_pairs: 开始迭代", Console::EColor_green);
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "Invalid Python object");
		}

		PyGILState_STATE gstate = fpPyGILState_Ensure();
		PyObject* iter = nullptr;

		if (PyDict_Check(wrapper->obj)) {
			// 对于字典，使用 items() 方法
			//g_Console->cLog("py_object_pairs: 迭代字典", Console::EColor_green);
			PyObject* items = PyDict_Items(wrapper->obj);
			if (!items) {
				fpPyGILState_Release(gstate);
				return luaL_error(L, "Failed to get dictionary items");
			}
			iter = PyObject_GetIter(items);
			//Py_DECREF(items);
		} 
		else if (PyList_Check(wrapper->obj) || PyTuple_Check(wrapper->obj)) {
			// 对于列表和元组，创建枚举迭代器
			//g_Console->cLog("py_object_pairs: 迭代序列", Console::EColor_green);
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
			//g_Console->cLog("py_object_pairs: 迭代其他对象", Console::EColor_green);
			iter = PyObject_GetIter(wrapper->obj);
		}

		if (!iter) {
			
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
			//Py_DECREF(item);
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
			//Py_DECREF(item);
		}

		fpPyGILState_Release(gstate);
		return 2;
	}

	// 添加一个新函数用于创建 Python bytes 对象

	 int Bridge::toPyBytes(lua_State* L) {
		size_t len;
		const char* data = luaL_checklstring(L, 1, &len);
		if (!data) {
			luaL_error(L, "需要提供字符串作为参数");
			return 0;
		}

		PyGILState_STATE gstate = Bridge::fpPyGILState_Ensure();
		
		// 创建 Python bytes 对象
		PyObject* bytes = PyBytes_FromStringAndSize(data, len);
		if (!bytes) {
			Bridge::fpPyGILState_Release(gstate);
			luaL_error(L, "创建 Python bytes 对象失败");
			return 0;
		}
		
		// 创建 PyObject 包装器并返回给 Lua
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = bytes;
		
		// 设置元表
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		Bridge::fpPyGILState_Release(gstate);
		return 1;
	}

	// 自动将Lua表转换为合适的Python数据结构（列表、元组或字典）
	int Bridge::toPyAuto(lua_State* L) {
		if (!lua_istable(L, 1)) {
			luaL_error(L, "需要提供表作为参数");
			return 0;
		}
		
		// 检查是否有第二个参数指定类型
		const char* typeHint = nullptr;
		if (lua_isstring(L, 2)) {
			typeHint = lua_tostring(L, 2);
		}
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 检查表的结构特征
		size_t len = lua_objlen(L, 1);
		bool isSequential = true;
		
		// 只有长度大于0的表才需要检查是否是序列
		if (len > 0) {
			// 检查是否所有键从1到len都有值
			for (size_t i = 1; i <= len; i++) {
				lua_rawgeti(L, 1, i);
				if (lua_isnil(L, -1)) {
					isSequential = false;
					lua_pop(L, 1);
					break;
				}
				lua_pop(L, 1);
			}
			
			// 检查是否有非数字键
			if (isSequential) {
				lua_pushnil(L);
				while (lua_next(L, 1) != 0) {
					// 检查键是否是数字，且是否在1到len的范围内
					if (lua_isnumber(L, -2)) {
						int idx = lua_tointeger(L, -2);
						if (idx < 1 || idx > len) {
							isSequential = false;
						}
					} else {
						isSequential = false;
					}
					lua_pop(L, 1);  // 弹出值，保留键
					if (!isSequential) break;
				}
			}
		}
		
		PyObject* result = nullptr;
		
		// 根据表的特征和类型提示创建合适的Python对象
		if (isSequential) {
			if (typeHint && strcmp(typeHint, "tuple") == 0) {
				// 创建元组
				result = fpPyTuple_New(len);
				if (!result) {
					fpPyGILState_Release(gstate);
					luaL_error(L, "创建Python元组失败");
					return 0;
				}
				
				for (size_t i = 1; i <= len; i++) {
					lua_rawgeti(L, 1, i);
					
					// 递归处理嵌套表
					if (lua_istable(L, -1)) {
						// 保存当前堆栈
						int top = lua_gettop(L);
						
						// 调用自身处理嵌套表
						lua_pushcfunction(L, toPyAuto);
						lua_pushvalue(L, -2);  // 复制表到栈顶
						lua_call(L, 1, 1);     // 调用toPyAuto(table)
						
						// 获取结果
						if (lua_isuserdata(L, -1)) {
							PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, -1);
							if (wrapper && wrapper->obj) {
								PyTuple_SetItem(result, i-1, wrapper->obj);
								wrapper->obj = nullptr;  // 防止自动清理
							}
						}
						
						lua_settop(L, top);  // 恢复堆栈
					} else {
						// 直接转换非表值
						PyObject* item = LuaToPython(L, -1);
						if (!item) {
							//Py_DECREF(result);
							lua_pop(L, 1);
							fpPyGILState_Release(gstate);
							luaL_error(L, "转换嵌套值失败");
							return 0;
						}
						PyTuple_SetItem(result, i-1, item);
					}
					
					lua_pop(L, 1);  // 弹出表项
				}
			} else {
				// 默认创建列表
				result = fpPyList_New(len);
				if (!result) {
					fpPyGILState_Release(gstate);
					luaL_error(L, "创建Python列表失败");
					return 0;
				}
				
				for (size_t i = 1; i <= len; i++) {
					lua_rawgeti(L, 1, i);
					
					// 递归处理嵌套表
					if (lua_istable(L, -1)) {
						// 保存当前堆栈
						int top = lua_gettop(L);
						
						// 调用自身处理嵌套表
						lua_pushcfunction(L, toPyAuto);
						lua_pushvalue(L, -2);  // 复制表到栈顶
						lua_call(L, 1, 1);     // 调用toPyAuto(table)
						
						// 获取结果
						if (lua_isuserdata(L, -1)) {
							PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, -1);
							if (wrapper && wrapper->obj) {
								PyList_SetItem(result, i-1, wrapper->obj);
								wrapper->obj = nullptr;  // 防止自动清理
							}
						}
						
						lua_settop(L, top);  // 恢复堆栈
					} else {
						// 直接转换非表值
						PyObject* item = LuaToPython(L, -1);
						if (!item) {
							//Py_DECREF(result);
							lua_pop(L, 1);
							fpPyGILState_Release(gstate);
							luaL_error(L, "转换嵌套值失败");
							return 0;
						}
						PyList_SetItem(result, i-1, item);
					}
					
					lua_pop(L, 1);  // 弹出表项
				}
			}
		} else {
			// 创建字典
			result = fpPyDict_New();
			if (!result) {
				fpPyGILState_Release(gstate);
				luaL_error(L, "创建Python字典失败");
				return 0;
			}
			
			lua_pushnil(L);  // 第一个键
			while (lua_next(L, 1) != 0) {
				// 处理键
				PyObject* pyKey = nullptr;
				
				// 检查键是否已经是 PyObject
				if (lua_isuserdata(L, -2)) {
					if (lua_getmetatable(L, -2)) {
						luaL_getmetatable(L, "PyObject");
						if (lua_rawequal(L, -1, -2)) {
							PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, -2);
							if (wrapper && wrapper->obj) {
								////Py_INCREF(wrapper->obj);
								pyKey = wrapper->obj;
							}
						}
						lua_pop(L, 2);  // 弹出两个元表
					}
				}
				
				// 如果键不是 PyObject，则转换它
			if (!pyKey) {
					pyKey = LuaToPython(L, -2);
				}
				
				if (!pyKey) {
					//Py_DECREF(result);
				fpPyGILState_Release(gstate);
					lua_pop(L, 1);  // 弹出值
					luaL_error(L, "转换键失败");
					return 0;
				}
				
				// 处理值
				PyObject* pyValue = nullptr;
				
				// 首先检查值是否已经是 PyObject
				if (lua_isuserdata(L, -1)) {
					if (lua_getmetatable(L, -1)) {
						luaL_getmetatable(L, "PyObject");
						if (lua_rawequal(L, -1, -2)) {
							PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, -1);
							if (wrapper && wrapper->obj) {
								////Py_INCREF(wrapper->obj);
								pyValue = wrapper->obj;
							}
						}
						lua_pop(L, 2);
					}
				}
				// 如果不是 PyObject，但是表，则递归处理
				else if (lua_istable(L, -1)) {
					// 保存当前状态
					int top = lua_gettop(L);
					
					// 调用自身处理嵌套表
					lua_pushcfunction(L, toPyAuto);
					lua_pushvalue(L, -2);  // 复制表到栈顶
					if (lua_pcall(L, 1, 1, 0) != 0) {  // 使用 pcall 而不是 call，以捕获可能的错误
						const char* error = lua_tostring(L, -1);
			//Py_DECREF(pyKey);
						//Py_DECREF(result);
						fpPyGILState_Release(gstate);
						lua_pop(L, 1);  // 弹出错误消息
						luaL_error(L, "递归处理表失败: %s", error ? error : "未知错误");
						return 0;
					}
					
					// 获取结果
					if (lua_isuserdata(L, -1)) {
						PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, -1);
						if (wrapper && wrapper->obj) {
							pyValue = wrapper->obj;
							////Py_INCREF(pyValue);  // 增加引用计数，因为我们会在后面手动减少
						}
					}
					
					// 恢复堆栈
					lua_settop(L, top);  // 恢复堆栈，但保留键值对
				}
				// 否则尝试直接转换
				else {
					pyValue = LuaToPython(L, -1);
				}
				
				if (!pyValue) {
					//Py_DECREF(pyKey);
					//Py_DECREF(result);
				fpPyGILState_Release(gstate);
					lua_pop(L, 1);  // 弹出值
					luaL_error(L, "转换值失败");
					return 0;
				}
				
				// 添加键值对到字典
				if (fpPyDict_SetItem(result, pyKey, pyValue) < 0) {
					//Py_DECREF(pyKey);
					//Py_DECREF(pyValue);
					//Py_DECREF(result);
					fpPyGILState_Release(gstate);
					luaL_error(L, "添加键值对到字典失败");
					return 0;
				}
				
				// 减少引用计数
				//Py_DECREF(pyKey);
				//Py_DECREF(pyValue);
				
				// 弹出值，保留键用于下一次迭代
				lua_pop(L, 1);
			}
		}
		
		// 创建PyObject包装器并返回
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = result;
		
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 创建Python元组 - 支持多参数调用，模拟Python的元组语法
	int Bridge::createPyTuple(lua_State* L) {
		int nargs = lua_gettop(L);  // 获取参数数量
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 创建Python元组
		PyObject* tuple = fpPyTuple_New(nargs);
		if (!tuple) {
			fpPyGILState_Release(gstate);
			luaL_error(L, "创建Python元组失败");
			return 0;
		}
		
		// 将所有参数添加到元组中
		for (int i = 0; i < nargs; i++) {
			PyObject* item = nullptr;
			
			// 检查是否已经是PyObject
			if (lua_isuserdata(L, i+1)) {
				if (lua_getmetatable(L, i+1)) {
					luaL_getmetatable(L, "PyObject");
					if (lua_rawequal(L, -1, -2)) {
						PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, i+1);
						if (wrapper && wrapper->obj) {
							////Py_INCREF(wrapper->obj);
							item = wrapper->obj;
						}
					}
					lua_pop(L, 2);
				}
			}
			
			// 如果不是PyObject，转换它
			if (!item) {
				item = LuaToPython(L, i+1);
			}
			
			if (!item) {
				//Py_DECREF(tuple);
				fpPyGILState_Release(gstate);
				luaL_error(L, "转换参数 %d 失败", i+1);
				return 0;
			}
			
			PyTuple_SetItem(tuple, i, item);  // 偷取引用
		}
		
		// 创建PyObject包装器并返回
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = tuple;
		
			luaL_getmetatable(L, "PyObject");
			lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 创建Python列表 - 支持多参数调用，模拟Python的列表语法
	int Bridge::createPyList(lua_State* L) {
		int nargs = lua_gettop(L);  // 获取参数数量
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 创建Python列表
		PyObject* list = fpPyList_New(nargs);
		if (!list) {
			fpPyGILState_Release(gstate);
			luaL_error(L, "创建Python列表失败");
			return 0;
		}
		
		// 将所有参数添加到列表中
		for (int i = 0; i < nargs; i++) {
			PyObject* item = nullptr;
			
			// 检查是否已经是PyObject
			if (lua_isuserdata(L, i+1)) {
				if (lua_getmetatable(L, i+1)) {
					luaL_getmetatable(L, "PyObject");
					if (lua_rawequal(L, -1, -2)) {
						PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, i+1);
						if (wrapper && wrapper->obj) {
							////Py_INCREF(wrapper->obj);
							item = wrapper->obj;
						}
					}
					lua_pop(L, 2);
				}
			}
			
			// 如果不是PyObject，转换它
			if (!item) {
				item = LuaToPython(L, i+1);
			}
			
			if (!item) {
				//Py_DECREF(list);
				fpPyGILState_Release(gstate);
				luaL_error(L, "转换参数 %d 失败", i+1);
				return 0;
			}
			
			PyList_SetItem(list, i, item);  // 偷取引用
		}
		
		// 创建PyObject包装器并返回
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = list;
		
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 创建Python字典 - 支持键值对参数，键和值交替出现
	int Bridge::createPyDict(lua_State* L) {
		int nargs = lua_gettop(L);
		
		// 检查参数数量是否为偶数（键值对）
		if (nargs % 2 != 0) {
			luaL_error(L, "字典函数需要偶数个参数（键值对）");
			return 0;
		}
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 创建Python字典
		PyObject* dict = fpPyDict_New();
		if (!dict) {
			fpPyGILState_Release(gstate);
			luaL_error(L, "创建Python字典失败");
			return 0;
		}
		
		// 添加键值对
		for (int i = 0; i < nargs; i += 2) {
			// 处理键（奇数索引参数）
			PyObject* key = nullptr;
			
			// 检查键是否已经是PyObject
			if (lua_isuserdata(L, i+1)) {
				if (lua_getmetatable(L, i+1)) {
					luaL_getmetatable(L, "PyObject");
					if (lua_rawequal(L, -1, -2)) {
						PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, i+1);
						if (wrapper && wrapper->obj) {
							////Py_INCREF(wrapper->obj);
							key = wrapper->obj;
						}
					}
					lua_pop(L, 2);
				}
			}
			
			// 如果键不是PyObject，转换它
			if (!key) {
				key = LuaToPython(L, i+1);
			}
			
			if (!key) {
				//Py_DECREF(dict);
				fpPyGILState_Release(gstate);
				luaL_error(L, "转换字典键失败（参数 %d）", i+1);
				return 0;
			}
			
			// 处理值（偶数索引参数）
			PyObject* value = nullptr;
			
			// 检查值是否已经是PyObject
			if (lua_isuserdata(L, i+2)) {
				if (lua_getmetatable(L, i+2)) {
					luaL_getmetatable(L, "PyObject");
					if (lua_rawequal(L, -1, -2)) {
						PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, i+2);
						if (wrapper && wrapper->obj) {
							////Py_INCREF(wrapper->obj);
							value = wrapper->obj;
						}
					}
					lua_pop(L, 2);
				}
			}
			
			// 如果值不是PyObject，转换它
			if (!value) {
				value = LuaToPython(L, i+2);
			}
			
			if (!value) {
				//Py_DECREF(key);
				//Py_DECREF(dict);
				fpPyGILState_Release(gstate);
				luaL_error(L, "转换字典值失败（参数 %d）", i+2);
				return 0;
			}
			
			// 添加键值对到字典
			if (fpPyDict_SetItem(dict, key, value) < 0) {
				//Py_DECREF(key);
				//Py_DECREF(value);
				//Py_DECREF(dict);
				fpPyGILState_Release(gstate);
				luaL_error(L, "添加键值对到字典失败");
				return 0;
			}
			
			// PyDict_SetItem 不会偷取引用，所以需要减少引用计数
			//Py_DECREF(key);
			//Py_DECREF(value);
		}
		
		// 创建PyObject包装器并返回
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		wrapper->obj = dict;
		
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);
		
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 添加一个辅助函数来打印Python错误信息
	void Bridge::PrintPythonError(const char* context) {
		if (!PyErr_Occurred()) {
			return;
		}
		
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		PyObject* type, *value, *traceback;
		PyErr_Fetch(&type, &value, &traceback);
		PyErr_NormalizeException(&type, &value, &traceback);
		
		if (type && value) {
			PyObject* str_value = fpPyObject_Str(value);
			if (str_value) {
				const char* error_msg = PyUnicode_AsUTF8(str_value);
				//g_Console->cLog("[Python错误 in %s] %s", Console::EColor_red, context, error_msg ? error_msg : "未知错误");
				//Py_DECREF(str_value);
			}
			
			if (traceback) {
				// 使用函数指针创建字符串和导入模块
				PyObject* module_name = fpPy_BuildValue("s", "traceback");
				PyObject* traceback_module = fpPyImport_Import(module_name);
				//Py_DECREF(module_name);
				
				if (traceback_module) {
					// 获取格式化异常函数
					PyObject* format_func = PyObject_GetAttrString(traceback_module, "format_exception");
					if (format_func && PyCallable_Check(format_func)) {
						// 创建参数元组
						PyObject* format_args = fpPyTuple_New(3);
						//Py_INCREF(type);
						//Py_INCREF(value);
						//Py_INCREF(traceback);
						PyTuple_SetItem(format_args, 0, type);
						PyTuple_SetItem(format_args, 1, value);
						PyTuple_SetItem(format_args, 2, traceback);
						
						// 调用函数
						PyObject* traceback_list = PyObject_CallObject(format_func, format_args);
						//Py_DECREF(format_args);
						
						if (traceback_list) {
							//g_Console->cLog("\n===== Python堆栈跟踪 =====", Console::EColor_red);
							Py_ssize_t size = PyList_Size(traceback_list);
							for (Py_ssize_t i = 0; i < size; i++) {
								PyObject* line = PyList_GetItem(traceback_list, i);
								const char* line_str = PyUnicode_AsUTF8(line);
								if (line_str) {
									//g_Console->cLog("%s", Console::EColor_red, line_str);
								}
							}
							//Py_DECREF(traceback_list);
						}
						

					}
					
					//Py_DECREF(traceback_module);
				}
			}
		}
		

		
		fpPyGILState_Release(gstate);
	}

	// 安全属性设置函数
	int Bridge::py_object_set_attr(lua_State* L) {
		//g_Console->cLog("GMP.setattr: 开始安全属性设置...", Console::EColor_green);
		
		// 检查参数数量
		if (lua_gettop(L) != 3) {
			return luaL_error(L, "setattr需要3个参数: (对象, 属性名, 值)");
		}
		
		// 1. 检查并获取目标对象
		if (!luaL_testudata(L, 1, "PyObject")) {
			return luaL_error(L, "参数 #1 必须是PyObject");
		}
		
		PyObjectWrapper* wrapper = (PyObjectWrapper*)lua_touserdata(L, 1);
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "参数 #1 是无效的PyObject");
		}
		
		// 打印对象信息
		//g_Console->cLog("目标对象: %p", Console::EColor_green, wrapper->obj);
		
		// 2. 检查并获取属性名
		if (!lua_isstring(L, 2)) {
			return luaL_error(L, "参数 #2 必须是字符串属性名");
		}
		
		const char* attrName = lua_tostring(L, 2);
		//g_Console->cLog("属性名: '%s'", Console::EColor_green, attrName);
		
		// 3. 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();
		
		// 4. 创建Python属性名
		PyObject* pyAttrName = fpPy_BuildValue("s", attrName);
		if (!pyAttrName) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "创建Python属性名失败");
		}
		
		// 5. 获取目标对象的类型信息
		PyObject* objType = PyObject_Type(wrapper->obj);
		if (objType) {
			PyObject* typeName = PyObject_GetAttrString(objType, "__name__");
			if (typeName) {
				const char* typeStr = PyUnicode_AsUTF8(typeName);
				//g_Console->cLog("目标对象类型: %s", Console::EColor_green, typeStr ? typeStr : "未知");
				//Py_DECREF(typeName);
			}
			//Py_DECREF(objType);
		}
		
		// 检查对象是否可以设置属性
		bool hasSetattr = PyObject_HasAttrString(wrapper->obj, "__setattr__");
		bool hasDict = PyObject_HasAttrString(wrapper->obj, "__dict__");
		//g_Console->cLog("对象特性: __setattr__=%d, __dict__=%d", Console::EColor_green, hasSetattr, hasDict);
		
		// 6. 处理值 - 根据类型进行转换
		PyObject* pyValue = nullptr;
		int valueType = lua_type(L, 3);
		//g_Console->cLog("值类型: %s", Console::EColor_green, lua_typename(L, valueType));
		
		// 如果是PyObject类型，直接使用
		if (valueType == LUA_TUSERDATA && luaL_testudata(L, 3, "PyObject")) {
			PyObjectWrapper* valueWrapper = (PyObjectWrapper*)lua_touserdata(L, 3);
			if (valueWrapper && valueWrapper->obj) {
				pyValue = valueWrapper->obj;
				//Py_INCREF(pyValue);  // 增加引用计数
				//g_Console->cLog("值是PyObject: %p", Console::EColor_green, pyValue);
				
				// 打印值的类型
				PyObject* valType = PyObject_Type(pyValue);
				if (valType) {
					PyObject* valTypeName = PyObject_GetAttrString(valType, "__name__");
					if (valTypeName) {
						const char* valTypeStr = PyUnicode_AsUTF8(valTypeName);
						//g_Console->cLog("值的类型: %s", Console::EColor_green, valTypeStr ? valTypeStr : "未知");
						//Py_DECREF(valTypeName);
					}
					//Py_DECREF(valType);
				}
				
				// 尝试打印值的内容
				PyObject* strRep = fpPyObject_Str(pyValue);
				if (strRep) {
					const char* strValue = PyUnicode_AsUTF8(strRep);
					//g_Console->cLog("值内容: %s", Console::EColor_green, strValue ? strValue : "无法显示");
					//Py_DECREF(strRep);
				}
			}
		}
		
		// 如果不是PyObject或获取失败，使用通用转换
		if (!pyValue) {
			switch (valueType) {
				case LUA_TNIL:
					//Py_INCREF(Py_None);
					pyValue = Py_None;
					//g_Console->cLog("值转换为None", Console::EColor_green);
					break;
					
				case LUA_TBOOLEAN:
					if (lua_toboolean(L, 3)) {
						//Py_INCREF(Py_True);
						pyValue = Py_True;
		} else {
						//Py_INCREF(Py_False);
						pyValue = Py_False;
					}
					//g_Console->cLog("值转换为布尔值: %s", Console::EColor_green, lua_toboolean(L, 3) ? "True" : "False");
					break;
					
				case LUA_TNUMBER:
					{
						lua_Number num = lua_tonumber(L, 3);
						pyValue = fpPy_BuildValue("d", num);
						//g_Console->cLog("值转换为数字: %f", Console::EColor_green, num);
					}
					break;
					
				case LUA_TSTRING:
					{
						size_t len;
						const char* str = lua_tolstring(L, 3, &len);
						pyValue = fpPy_BuildValue("s#", str, len);
						//g_Console->cLog("值转换为字符串: \"%s\"", Console::EColor_green, str);
					}
					break;
					
				default:
					pyValue = LuaToPython(L, 3);
					//g_Console->cLog("使用通用转换方法", Console::EColor_green);
					break;
			}
		}
		
		// 检查值转换结果
		if (!pyValue) {
			//Py_DECREF(pyAttrName);
			fpPyGILState_Release(gstate);
			return luaL_error(L, "无法将值转换为Python对象");
		}
		
		// 7. 执行属性设置
		//g_Console->cLog("执行属性设置操作...", Console::EColor_green);
		int result = fpPyObject_SetAttr(wrapper->obj, pyAttrName, pyValue);
		
		// 8. 清理资源
		//Py_DECREF(pyAttrName);
		//Py_DECREF(pyValue);
		
		// 9. 处理结果
		if (result < 0) {
			//g_Console->cLog("属性设置失败!", Console::EColor_red);
			
			// 尝试获取更详细的错误信息
			if (PyErr_Occurred()) {
				PyObject* ptype, *pvalue, *ptraceback;
				PyErr_Fetch(&ptype, &pvalue, &ptraceback);
				
				if (pvalue) {
					PyObject* str_exc = fpPyObject_Str(pvalue);
					if (str_exc) {
						const char* exc_str = PyUnicode_AsUTF8(str_exc);
						//g_Console->cLog("Python错误: %s", Console::EColor_red, exc_str ? exc_str : "未知错误");
						//Py_DECREF(str_exc);
					}
				}
				
				// 检查是否是AttributeError
				if (ptype) {
					PyObject* err_name = PyObject_GetAttrString(ptype, "__name__");
					if (err_name) {
						const char* err_str = PyUnicode_AsUTF8(err_name);
						//g_Console->cLog("错误类型: %s", Console::EColor_red, err_str ? err_str : "Unknown");
						
						// 给出特定错误类型的帮助信息
						if (err_str && strcmp(err_str, "AttributeError") == 0) {
							//g_Console->cLog("原因: 对象不允许设置该属性或属性不存在", Console::EColor_yellow);
						}
						//Py_DECREF(err_name);
					}
				}
				
				
			}
			
			fpPyGILState_Release(gstate);
			return luaL_error(L, "设置属性 '%s' 失败", attrName);
		}
		
		// 成功设置属性
		//g_Console->cLog("属性设置成功", Console::EColor_green);
		fpPyGILState_Release(gstate);
		
		return 0;  // 返回0表示没有返回值
	}

	// Python字典迭代函数
	int Bridge::py_dict_iterate(lua_State* L) {
		//g_Console->cLog("py_dict_iterate: 开始字典迭代", Console::EColor_green);

		// 检查参数是否为PyObject
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "参数必须是有效的Python对象");
		}

		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 检查是否为字典类型
		if (!PyDict_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "对象不是Python字典");
		}

		// 创建迭代状态对象
		PyObject* items = PyDict_Items(wrapper->obj);
		if (!items) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "无法获取字典项");
		}

		// 创建迭代器对象
		PyObject* iterator = PyObject_GetIter(items);
		if (!iterator) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "无法创建迭代器");
		}

		// 创建迭代器包装
		PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
		iterWrapper->obj = iterator;
		luaL_getmetatable(L, "PyObject");
		lua_setmetatable(L, -2);

		// 创建闭包函数作为迭代器
		lua_pushcclosure(L, [](lua_State* L) -> int {
			// 获取迭代器
			PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_touserdata(L, lua_upvalueindex(1));
			if (!iterWrapper || !iterWrapper->obj) {
				return 0; // 迭代结束
			}

			// 获取GIL
			PyGILState_STATE gstate = Bridge::fpPyGILState_Ensure();

			// 获取下一个项目
			PyObject* item = PyIter_Next(iterWrapper->obj);
			if (!item) {
				if (PyErr_Occurred()) 
				Bridge::fpPyGILState_Release(gstate);
				return 0; // 迭代结束
			}

			// item应该是一个包含键值对的元组
			if (PyTuple_Check(item) && PyTuple_Size(item) == 2) {
				PyObject* key = PyTuple_GetItem(item, 0);   // 借用的引用
				PyObject* value = PyTuple_GetItem(item, 1); // 借用的引用

				// 转换为Lua值并返回
				PythonToLua(L, key);
				PythonToLua(L, value);

				Bridge::fpPyGILState_Release(gstate);
				return 2; // 返回键和值
			}

			// 如果不是键值对，只返回项目本身
			PythonToLua(L, item);
			Bridge::fpPyGILState_Release(gstate);
			return 1;
			}, 1); // 1个upvalue: 迭代器

		// 返回迭代器函数
		fpPyGILState_Release(gstate);
		return 1;
	}

	// Python列表迭代函数
	int Bridge::py_list_iterate(lua_State* L) {
		//g_Console->cLog("py_list_iterate: 开始列表迭代", Console::EColor_green);

		// 检查参数是否为PyObject
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "参数必须是有效的Python对象");
		}

		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 检查是否为列表类型
		if (!PyList_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "对象不是Python列表");
		}

		// 获取列表长度
		Py_ssize_t size = PyList_Size(wrapper->obj);

		// 创建迭代状态表
		lua_newtable(L);
		lua_pushinteger(L, 0); // 当前索引，从0开始（将在迭代函数中+1）
		lua_setfield(L, -2, "index");
		lua_pushinteger(L, size); // 列表大小
		lua_setfield(L, -2, "size");

		// 保存列表对象的引用
		lua_pushlightuserdata(L, wrapper->obj);
		lua_setfield(L, -2, "list");

		// 创建闭包函数作为迭代器
		lua_pushcclosure(L, [](lua_State* L) -> int {
			// 获取状态表
			lua_pushvalue(L, lua_upvalueindex(1));

			// 获取当前索引和大小
			lua_getfield(L, -1, "index");
			lua_Integer index = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_getfield(L, -1, "size");
			lua_Integer size = lua_tointeger(L, -1);
			lua_pop(L, 1);

			// 检查是否迭代结束
			if (index >= size) {
				return 0; // 迭代结束
			}

			// 更新索引
			index++;
			lua_pushinteger(L, index);
			lua_setfield(L, -2, "index");

			// 获取列表对象
			lua_getfield(L, -1, "list");
			PyObject* list = (PyObject*)lua_touserdata(L, -1);
			lua_pop(L, 2); // 弹出list和状态表

			// 获取GIL
			PyGILState_STATE gstate = Bridge::fpPyGILState_Ensure();

			// 获取元素
			PyObject* item = PyList_GetItem(list, index - 1); // Python索引从0开始

			// 转换为Lua值
			lua_pushinteger(L, index); // Lua索引从1开始
			PythonToLua(L, item);

			Bridge::fpPyGILState_Release(gstate);
			return 2; // 返回索引和值
			}, 1); // 1个upvalue: 状态表

		// 返回迭代器函数
		fpPyGILState_Release(gstate);
		return 1;
	}

	// Python元组迭代函数 - 将二元元组视为键值对
	int Bridge::py_tuple_iterate(lua_State* L) {
		// 检查参数是否为PyObject
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "参数必须是有效的Python对象");
		}

		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 检查是否为元组或列表类型
		if (!PyTuple_Check(wrapper->obj) && !PyList_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return luaL_error(L, "对象不是Python元组或列表");
		}

		// 获取长度
		Py_ssize_t size = PyTuple_Check(wrapper->obj) ? 
						 PyTuple_Size(wrapper->obj) : 
						 PyList_Size(wrapper->obj);

		// 检查是否是二元元组，对于inspect.getmembers()的结果，每个项都是二元元组
		bool isTwoElementTuple = (size == 2);

		// 创建迭代状态表
		lua_newtable(L);
		lua_pushinteger(L, 0); // 当前索引，从0开始（将在迭代函数中+1）
		lua_setfield(L, -2, "index");
		lua_pushinteger(L, size); // 集合大小
		lua_setfield(L, -2, "size");
		
		// 保存元组/列表对象的引用
		lua_pushlightuserdata(L, wrapper->obj);
		lua_setfield(L, -2, "collection");
		
		// 标记是否为元组
		lua_pushboolean(L, PyTuple_Check(wrapper->obj));
		lua_setfield(L, -2, "isTuple");
		
		// 标记是否为二元元组
		lua_pushboolean(L, isTwoElementTuple);
		lua_setfield(L, -2, "isTwoElementTuple");

		// 创建闭包函数作为迭代器
		lua_pushcclosure(L, [](lua_State* L) -> int {
			// 获取状态表
			lua_pushvalue(L, lua_upvalueindex(1));

			// 获取当前索引和大小
			lua_getfield(L, -1, "index");
			lua_Integer index = lua_tointeger(L, -1);
			lua_pop(L, 1);

			lua_getfield(L, -1, "size");
			lua_Integer size = lua_tointeger(L, -1);
			lua_pop(L, 1);

			// 检查是否迭代结束
			if (index >= size) {
				return 0; // 迭代结束
			}

			// 更新索引
			index++;
			lua_pushinteger(L, index);
			lua_setfield(L, -2, "index");

			// 获取对象引用
			lua_getfield(L, -1, "collection");
			PyObject* collection = (PyObject*)lua_touserdata(L, -1);
			lua_pop(L, 1);
			
			// 检查是否为元组
			lua_getfield(L, -1, "isTuple");
			bool isTuple = lua_toboolean(L, -1);
			lua_pop(L, 1);
			
			// 检查是否处理为键值对
			lua_getfield(L, -1, "isTwoElementTuple");
			bool isTwoElementTuple = lua_toboolean(L, -1);
			lua_pop(L, 2); // 弹出isTwoElementTuple和状态表

			// 获取GIL
			PyGILState_STATE gstate = Bridge::fpPyGILState_Ensure();

			// 如果这是一个单独的二元元组 (如 ('add', func_obj))，直接解析为键值对
			if (isTwoElementTuple) {
				// 获取键（第一个元素）
				PyObject* key = isTuple ? 
							 PyTuple_GetItem(collection, 0) : 
							 PyList_GetItem(collection, 0);
				
				// 获取值（第二个元素）
				PyObject* value = isTuple ? 
							   PyTuple_GetItem(collection, 1) : 
							   PyList_GetItem(collection, 1);
				
				// 转换为Lua类型并返回
				PythonToLua(L, key);
				PythonToLua(L, value);
				
				Bridge::fpPyGILState_Release(gstate);
				return 2; // 返回键和值
			} 
			// 否则，按正常方式迭代（返回索引和元素）
			else {
				// 获取元素
				PyObject* item = isTuple ? 
							 PyTuple_GetItem(collection, index - 1) : 
							 PyList_GetItem(collection, index - 1);
				
				// 转换为Lua值
				lua_pushinteger(L, index); // Lua索引从1开始
				PythonToLua(L, item);
				
				Bridge::fpPyGILState_Release(gstate);
				return 2; // 返回索引和值
			}
		}, 1); // 1个upvalue: 状态表

		// 返回迭代器函数
		fpPyGILState_Release(gstate);
		return 1;
	}

	// 自动判断Python对象类型并选择合适的迭代函数
	int Bridge::py_iterate(lua_State* L) {
		//g_Console->cLog("py_iterate: 开始智能迭代", Console::EColor_green);

		// 检查参数是否为PyObject
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			return luaL_error(L, "参数必须是有效的Python对象");
		}

		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 根据对象类型选择迭代函数
		if (PyDict_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return py_dict_iterate(L);
		}
		else if (PyList_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return py_list_iterate(L);
		}
		else if (PyTuple_Check(wrapper->obj)) {
			fpPyGILState_Release(gstate);
			return py_tuple_iterate(L);
		}
		else {
			// 尝试通用迭代器
			PyObject* iter = PyObject_GetIter(wrapper->obj);
			if (iter) {
				// 创建迭代器包装
				PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_newuserdata(L, sizeof(PyObjectWrapper));
				iterWrapper->obj = iter;
				luaL_getmetatable(L, "PyObject");
				lua_setmetatable(L, -2);

				// 创建闭包函数作为迭代器
				lua_pushcclosure(L, [](lua_State* L) -> int {
					// 获取迭代器
					PyObjectWrapper* iterWrapper = (PyObjectWrapper*)lua_touserdata(L, lua_upvalueindex(1));
					if (!iterWrapper || !iterWrapper->obj) {
						return 0; // 迭代结束
					}

					// 获取GIL
					PyGILState_STATE gstate = Bridge::fpPyGILState_Ensure();

					// 获取下一个项目
					PyObject* item = PyIter_Next(iterWrapper->obj);
					if (!item) {
						if (PyErr_Occurred()) 
						Bridge::fpPyGILState_Release(gstate);
						return 0; // 迭代结束
					}

					// 转换为Lua值并返回
					static int index = 0;
					index++;
					lua_pushinteger(L, index);
					PythonToLua(L, item);

					Bridge::fpPyGILState_Release(gstate);
					return 2; // 返回索引和值
					}, 1); // 1个upvalue: 迭代器

				fpPyGILState_Release(gstate);
				return 1;
			}

			// 如果无法迭代，返回错误
			fpPyGILState_Release(gstate);
			return luaL_error(L, "对象不支持迭代");
		}
	}

	// 获取Python对象类型的辅助函数
	int Bridge::py_get_type(lua_State* L) {
		// 检查参数是否为PyObject
		PyObjectWrapper* wrapper = (PyObjectWrapper*)luaL_checkudata(L, 1, "PyObject");
		if (!wrapper || !wrapper->obj) {
			lua_pushstring(L, "invalid");
			return 1;
		}

		// 获取GIL
		PyGILState_STATE gstate = fpPyGILState_Ensure();

		// 判断对象类型
		if (PyDict_Check(wrapper->obj)) {
			lua_pushstring(L, "dict");
		}
		else if (PyList_Check(wrapper->obj)) {
			lua_pushstring(L, "list");
		}
		else if (PyTuple_Check(wrapper->obj)) {
			lua_pushstring(L, "tuple");
		}
		else if (PyUnicode_Check(wrapper->obj)) {
			lua_pushstring(L, "str");
		}
		else if (PyLong_Check(wrapper->obj)) {
			lua_pushstring(L, "int");
		}
		else if (PyFloat_Check(wrapper->obj)) {
			lua_pushstring(L, "float");
		}
		else if (PyBool_Check(wrapper->obj)) {
			lua_pushstring(L, "bool");
		}
		else if (PyModule_Check(wrapper->obj)) {
			lua_pushstring(L, "module");
		}
		else if (PyCallable_Check(wrapper->obj)) {
			lua_pushstring(L, "callable");
		}
		else {
			lua_pushstring(L, "object");
		}

		fpPyGILState_Release(gstate);
		return 1;
	}

}