#pragma once
#include "pch.h"

namespace DX11Base {
	namespace Offset {
		UINT64 offset_PyEval_CallObjectWithKeywords = 0xb2570;

		UINT64 offset_PyObject_GetAttr = 0x6fb20;

		//dict_repr函数中 有如下调用 PyUnicode_FromString("{...}")
		UINT64 offset_PyUnicode_FromString = 0x7d450;

		UINT64 offset_Py_BuildValue = 0xaafa0;

		UINT64 offset_PyImport_Import = 0x6e9d0;

		UINT64 offset_PyGILState_Ensure = 0x9fcd0;

		UINT64 offset_PyGILState_Release = 0x9fd70;

		UINT64 offset_PyObject_Str = 0x6f250;

		UINT64 offset_PyTuple_New = 0xbc290;
		UINT64 offset_PyList_New = 0xdfe20;
		UINT64 offset_new_dict = 0xbe2d0;
		UINT64 offset_new_keys_object = 0xbe060;

		UINT64 offset_PyObject_SetAttr = 0x6fbe0;

		UINT64 offset_PyDict_SetItem = 0xbffe0;

		UINT64 offset_PyObject_SetItem = 0x65c90;
		UINT64 offset_PyTuple_SetItem = 0xbc490;
	}

}
