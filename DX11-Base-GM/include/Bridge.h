#pragma once
#include "helper.h"
#include <fstream>
#include <mutex>
#include <string>
#include <LuaVM.h>
namespace DX11Base {

	typedef PyObject* (__stdcall* TPyEval_CallObjectWithKeywords)(PyObject*, PyObject*, PyObject*);
	typedef PyObject* (__stdcall* TPyObject_GetAttr)(PyObject*, PyObject*);
	typedef PyObject* (__stdcall* TPy_BuildValue)(const char*, ...);
	typedef PyObject* (__stdcall* TPyImport_Import)(PyObject*);
	typedef PyGILState_STATE(__stdcall* TPyGILState_Ensure)(void);
	typedef void(__stdcall* TPyGILState_Release)(PyGILState_STATE);
	typedef PyObject* (__stdcall* TPyObject_Str)(PyObject*);
	typedef PyObject* (__stdcall* TPyTuple_New)(Py_ssize_t);

	typedef struct {
		PyObject* obj;
	} PyObjectWrapper;

	class Bridge {
	public:
	
		UINT64 m1logic4{ 0 };

	
		static TPyEval_CallObjectWithKeywords fpPyEval_CallObjectWithKeywords;
		static TPyObject_GetAttr fpPyObject_GetAttr;
		static TPyImport_Import fpPyImport_Import;
		static TPyGILState_Ensure fpPyGILState_Ensure;
		static TPyGILState_Release fpPyGILState_Release;
		static TPy_BuildValue fpPy_BuildValue;
		static TPyObject_Str fpPyObject_Str;
		static TPyTuple_New fpPyTuple_New;
	public:
		bool isInitialized();
		bool InitPyBridge();

		void RegisterPyObjectMetatables();

	

		static int BuildValue(lua_State* L);
		void RegisterBridgeFunctions();

		static PyObject* LuaToPython(lua_State* L, int index);
	

		// 添加新的方法
		static void RegisterPyObjectMetatable(lua_State* L);

		// Python 函数的 Lua 绑定
		static int CallObject(lua_State* L);
		static int GetAttr(lua_State* L);
		static int FromString(lua_State* L);
		static int ImportModule(lua_State* L);
		static int ToString(lua_State* L);

		static int ToLua(lua_State* L);

		static int ToPython(lua_State* L);

		static int py_object_call(lua_State* L);





		

	private:
		
		bool   isInitionalized{ false };
		
		// 元表方法
		static int py_object_gc(lua_State* L);
	
		static int py_object_index(lua_State* L);
	
	};
	inline std::unique_ptr<Bridge> g_Bridge;
}