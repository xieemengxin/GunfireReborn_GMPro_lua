﻿#pragma once
#include <pch.h>
#include <helper.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwCallReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwCallReason == DLL_PROCESS_ATTACH)
    {
        DX11Base::g_hModule = hModule;

        DisableThreadLibraryCalls(hModule);


        //不再自动创建，改为导出函数创建
       /* HANDLE hThread = CreateThread(0, 0, MainThread_Initialize, DX11Base::g_hModule, 0, 0);
        if (hThread)
            CloseHandle(hThread);*/
    }

    return TRUE;
}
