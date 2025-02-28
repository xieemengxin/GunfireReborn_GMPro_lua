#ifndef PCH_H
#define PCH_H
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// STANDARD LIBRARIES
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <fstream>
#include <iostream>
#include <conio.h>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

// DIRECTX
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

//	GAMEPAD
#include <XInput.h>
#pragma comment(lib, "XInput.lib")

//	INTERNET
#include <Wininet.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")

// MINHOOK
#include "libs/MinHook/MinHook.h"


// DEAR IMGUI
#include "libs/ImGui/imgui.h"
#include "libs/ImGui/imgui_internal.h"
#include "libs/ImGui/imgui_Impl_dx11.h"
#include "libs/ImGui/imgui_Impl_Win32.h"

#endif //PCH_H

//Python
#include <Python.h>