#pragma once
#include "pch.h"

namespace DX11Base {
	namespace Offset {
		UINT64 offset_PyEval_CallObjectWithKeywords = 0xB2680;

		UINT64 offset_PyObject_GetAttr = 0x6fc30;

		//dict_repr函数中 有如下调用 PyUnicode_FromString("{...}")
		UINT64 offset_PyUnicode_FromString = 0x84900;

		UINT64 offset_Py_BuildValue = 0xab0b0;

		UINT64 offset_PyImport_Import = 0x6eae0;

		UINT64 offset_PyGILState_Ensure = 0x9fde0;

		UINT64 offset_PyGILState_Release = 0x9fe80;

		UINT64 offset_PyObject_Str = 0x6f360;

		UINT64 offset_PyTuple_New = 0xbc3a0;
	}

}
