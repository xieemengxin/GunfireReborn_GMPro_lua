#include <pch.h>
#include <Engine.h>
#include <Menu.h>
#include <LuaVM.h>
#include <Bridge.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace DX11Base 
{
	FILE*				Console::pOutStream{ nullptr };
	bool				Console::bInit{ false };
	static				uint64_t* MethodsTable{ nullptr };

	//----------------------------------------------------------------------------------------------------
	//										ENGINE
	//-----------------------------------------------------------------------------------
#pragma region //	ENGINE

	Engine::Engine() 
	{
		g_Console = std::make_unique<Console>();
		g_D3D11Window = std::make_unique<D3D11Window>();
		g_Hooking = std::make_unique<Hooking>();
		g_LuaVM = std::make_unique<LuaVM>();
		g_Bridge = std::make_unique<Bridge>();
	}

	Engine::~Engine()
	{
		g_Hooking.release();
		g_D3D11Window.release();
		g_Console.release();
		g_LuaVM.release();
		g_Bridge.release();
	}

	void Engine::Init()
	{
		mGamePID = GetCurrentProcessId();
		pGameHandle = GetCurrentProcess();
		pGameWindow = GetForegroundWindow();
		pGameModule = GetModuleHandle(0);
		pGameBaseAddr = reinterpret_cast<__int64>(pGameModule);

		RECT tempRECT;
		GetWindowRect(pGameWindow, &tempRECT);
		mGameWidth = tempRECT.right - tempRECT.left;
		mGameHeight = tempRECT.bottom - tempRECT.top;

		char tempTitle[MAX_PATH];
		GetWindowTextA(pGameWindow, tempTitle, sizeof(tempTitle));
		pGameTitle = tempTitle;

		char tempClassName[MAX_PATH];
		GetClassNameA(pGameWindow, tempClassName, sizeof(tempClassName));
		pClassName = tempClassName;

		char tempPath[MAX_PATH];
		GetModuleFileNameExA(pGameHandle, 0, tempPath, sizeof(tempPath));
		pGamePath = tempPath;

		// 初始化 LuaVM
		if (!g_LuaVM->Initialize()) {
			g_Console->LogError("Failed to initialize LuaVM\n");
		}
		else {
			g_Console->cLog("LuaVM initialized \n",Console::EColor_green);
		}
		//初始化Python桥
		if (!g_Bridge->InitPyBridge()) {
			g_Console->LogError("Failed to initialize Bridge\n");
		}
		else {
			g_Console->cLog("Bridge initialized \n", Console::EColor_green);
		}
		
		SetConsoleOutputCP(CP_UTF8);   // 控制台使用UTF-8
		SetConsoleCP(CP_UTF8);         // 控制台输入使用UTF-8

		// 在适当的初始化位置添加以下代码来加载中文字体

	}

	bool Engine::GetKeyState(WORD vKey, SHORT delta)
	{
		static int lastTick = 0;

		bool result = ((GetAsyncKeyState(vKey) & 0x8000) && (GetTickCount64() - lastTick) > delta);

		if (result)
			lastTick = GetTickCount64();

		return result;
	}

	bool Engine::GamePadGetKeyState(WORD vButton)
	{
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		DWORD result = XInputGetState(0, &state);
		if (result == ERROR_SUCCESS)
		{
			if ((state.Gamepad.wButtons & vButton) == vButton)
				return true;
		}
		return false;
	}

	void Engine::Update()
	{
		// 使用高精度计时器来限制帧率
		static LARGE_INTEGER frequency;
		static LARGE_INTEGER lastTime;
		static bool initialized = false;

		if (!initialized)
		{
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&lastTime);
			initialized = true;
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		// 计算帧时间
		double deltaTime = (currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;

		// 目标帧率为 144 FPS
		const double targetFrameTime = 1.0 / 144.0;

		// 如果帧时间太短，则等待
		if (deltaTime < targetFrameTime)
		{
			DWORD sleepTime = (DWORD)((targetFrameTime - deltaTime) * 1000.0);
			if (sleepTime > 0)
				Sleep(sleepTime);
		}

		lastTime = currentTime;
	}

	void Engine::LoadImGuiFonts()
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		// 清除现有字体
		io.Fonts->Clear();

		// 加载中文字体
		ImFontConfig font_config;
		font_config.OversampleH = 2;
		font_config.OversampleV = 1;
		font_config.PixelSnapH = false;

		// 定义字符范围 - 包括中文字符
		static const ImWchar ranges[] = {
			0x0020, 0x00FF, // 基本拉丁字符
			0x2000, 0x206F, // 常用标点
			0x3000, 0x30FF, // 日文假名（也常用于中文）
			0x31F0, 0x31FF, // 假名扩展
			0x4e00, 0x9FAF, // 常用汉字
			0x0400, 0x052F, // 西里尔字母
			0xFF00, 0xFFEF, // 全角字符
			0,
		};

		// 加载微软雅黑字体（支持中文）
		ImFont* font = io.Fonts->AddFontFromFileTTF(
			"c:\\Windows\\Fonts\\msyh.ttc", 16.0f,
			&font_config, ranges);

		if (!font) {
			// 如果加载失败，使用默认字体
			g_Console->LogError("Failed to load Chinese font, using default");
			io.Fonts->AddFontDefault();
		}

		// 重建字体纹理
		io.Fonts->Build();
	}

#pragma endregion


	//----------------------------------------------------------------------------------------------------
	//										CONSOLE
	//-----------------------------------------------------------------------------------
#pragma region //	CONSOLE

	Console::Console() {

	}
	
	Console::Console(const char* title) { InitializeConsole(title); }
	
	Console::Console(const char* title, bool bShow) { InitializeConsole(title, bShow); }
	
	Console::~Console() { DestroyConsole(); }

	HANDLE Console::GetHandle() { return this->pHandle; }
	
	HWND Console::GetWindowHandle() { return this->pHwnd; }

	// 在适当位置添加UTF-8转换函数
	std::string ConvertToUTF8(const char* text)
	{
		if (!text || !strlen(text))
			return "";
		
		// 获取需要的缓冲区大小
		int wideLen = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
		if (wideLen <= 0) return text;
		
		// 转换到宽字符
		wchar_t* wideStr = new wchar_t[wideLen];
		MultiByteToWideChar(CP_ACP, 0, text, -1, wideStr, wideLen);
		
		// 转换到UTF-8
		int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
		if (utf8Len <= 0) {
			delete[] wideStr;
			return text;
		}
		
		char* utf8Str = new char[utf8Len];
		WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, utf8Len, NULL, NULL);
		
		std::string result(utf8Str);
		delete[] wideStr;
		delete[] utf8Str;
		
		return result;
	}

	// 修改Clear函数来同时清除ImGui缓冲区
	void Console::Clear() 
	{
		// 清除控制台
		//system("cls"); 
		
		// 同时清除ImGui缓冲区
		if (g_Console) {
			g_Console->logBuffer.clear();
			g_Console->ScrollToBottom = true;
		}
	}

	// 修改Log函数确保中文正确显示
	void Console::Log(const char* format, ...)
	{
		// 原有代码...
		va_list args;
		va_start(args, format);
		
		char buffer[1024];
		vsprintf_s(buffer, format, args);
		
		va_end(args);
		
		// 转换为UTF-8
		std::string utf8Text = ConvertToUTF8(buffer);
		
		// 添加到ImGui缓冲区
		if (g_Console->bInit) {
			char timestamp[32];
			time_t now = time(nullptr);
			struct tm timeinfo;
			localtime_s(&timeinfo, &now);
			strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", &timeinfo);
			
			g_Console->logBuffer.append(timestamp);
			g_Console->logBuffer.append(utf8Text.c_str());
			g_Console->logBuffer.append("\n");
			g_Console->ScrollToBottom = true;
		}
		
		// 输出到控制台
		if (pOutStream)
			fprintf(pOutStream, "%s\n", utf8Text.c_str());
		
		// 如果有日志文件，写入日志
		if (g_Console && g_Console->logFile.is_open()) {
			g_Console->logFile << buffer << std::endl;
			g_Console->logFile.flush();
		}
	}

	// 修改 cLog 函数以支持中文字符，添加时间戳和换行符
	void Console::cLog(const char* fmt, EColors color, ...)
	{
		if (!pOutStream)
			return;

		// 获取当前时间
		SYSTEMTIME st;
		GetLocalTime(&st);
		
		// 格式化时间戳
		char timestamp[64];
		sprintf_s(timestamp, "[%02d:%02d:%02d] ", 
			st.wHour, st.wMinute, st.wSecond);
		
		// 格式化参数
		va_list arg;
		va_start(arg, color);
		char buffer[4096];
		vsprintf_s(buffer, fmt, arg);
		va_end(arg);

		// 转换为UTF-8
		std::string utf8Text = ConvertToUTF8(buffer);
		
		// 创建完整消息（带时间戳）
		std::string fullMsg = timestamp;
		fullMsg += utf8Text;

		// 设置控制台文本颜色
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hConsole, color);

		// 输出到控制台（添加换行符）
		fprintf(pOutStream, "%s\n", fullMsg.c_str());

		// 恢复默认颜色
		SetConsoleTextAttribute(hConsole, EColors::EColor_DEFAULT);

		// 添加到ImGui缓冲区（添加换行符）
		if (g_Console && g_Console->bInit) {
			g_Console->logBuffer.append(fullMsg.c_str());
			g_Console->logBuffer.append("\n");
			g_Console->ScrollToBottom = true;
		}

		// 如果有日志文件，写入日志（添加换行符）
		if (g_Console && g_Console->logFile.is_open()) {
			g_Console->logFile << fullMsg << std::endl;
			g_Console->logFile.flush();
		}
	}

	// 在InitializeConsole函数中添加中文支持
	void Console::InitializeConsole(const char* ConsoleName, bool bShowWindow)
	{
		if (Console::bInit)
		{
			LogError("[!] [Console::InitializeConsole] failed to initialize console.\n");
			return;
		}
		// 初始化其他成员...
		g_Console->logBuffer = ImGuiTextBuffer();
		g_Console->AutoScroll = true;
		g_Console->ScrollToBottom = false;

		// 获取程序目录
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string::size_type pos = std::string(buffer).find_last_of("\\/");
		std::string logPath = std::string(buffer).substr(0, pos + 1) + "GMPro.log";

		// 打开日志文件
		logFile.open(logPath, std::ios::out | std::ios::trunc);
		if (!logFile.is_open()) {
			MessageBoxA(NULL, "Failed to create log file", "Error", MB_OK | MB_ICONERROR);
		}

		// 初始化控制台
		AllocConsole();
		// 设置控制台代码页为UTF-8
		SetConsoleOutputCP(CP_UTF8);
		SetConsoleCP(CP_UTF8);
		pHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		pHwnd = GetConsoleWindow();
		freopen_s(&pOutStream, "CONOUT$", "w", stdout);

		char titleBuff[256];
		sprintf_s(titleBuff, "[DX11Base] %s", ConsoleName);
		SetConsoleTitleA(titleBuff);

		Console::bInit = true;
		Console::bShow = bShowWindow;

		ShowWindow(this->pHwnd, Console::bShow ? SW_SHOW : SW_HIDE);

		// 写入日志头
		time_t now = time(nullptr);
		char timestamp[64];
		strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
		logFile << "=== GMPro Log Started at " << timestamp << " ===" << std::endl;
		logFile.flush();

	
	}

	void Console::AddToLogBuffer(const char* text) {
		if (logBuffer.size() > 0 && logBuffer.end()[-1] != '\n') {
			logBuffer.append("\n");
		}
		
		logBuffer.append(text);
		ScrollToBottom = true;
	}

	void Console::LogError(const char* fmt, ...)
	{
		// 获取当前时间
		SYSTEMTIME st;
		GetLocalTime(&st);
		
		// 格式化时间戳
		char timestamp[64];
		sprintf_s(timestamp, " [%04d-%02d-%02d %02d:%02d:%02d.%03d]\t", 
			st.wYear, st.wMonth, st.wDay,
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		// 格式化错误消息
		char buf[1024];
		va_list arg;
		va_start(arg, fmt);
		vsnprintf(buf, sizeof(buf), fmt, arg);
		va_end(arg);

		// 输出到控制台
		SetConsoleTextAttribute(pHandle, static_cast<WORD>(EColors::EColor_red));
		fprintf(pOutStream, "%s%s", timestamp, buf);
		SetConsoleTextAttribute(pHandle, static_cast<WORD>(EColors::EColor_DEFAULT));

		// 输出到日志文件
		if (logFile.is_open()) {
			std::lock_guard<std::mutex> lock(logMutex);
			logFile << timestamp << "[ERROR] " << buf;
			logFile.flush();
		}
	}

	//	
	void Console::DestroyConsole()
	{
		if (Console::bInit)
		{
			// 写入日志尾
			if (logFile.is_open()) {
				time_t now = time(nullptr);
				char timestamp[64];
				strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
				logFile << "\n=== GMPro Log Ended at " << timestamp << " ===" << std::endl;
				logFile.close();
			}

			fclose(pOutStream);
			FreeConsole();
			Console::bInit = false;
		}
	}

	//	
	void Console::SetConsoleVisibility(bool bShow)
	{
		this->bShow = bShow;
		ShowWindow(pHwnd, bShow ? SW_SHOW : SW_HIDE);
	}

#pragma endregion


	//----------------------------------------------------------------------------------------------------
	//										D3DWINDOW
	//-----------------------------------------------------------------------------------
#pragma region	//	D3DWINDOW

	D3D11Window::D3D11Window() {}

	D3D11Window::~D3D11Window() { bInit = false; }

	//-----------------------------------------------------------------------------------
	//  
	LRESULT CALLBACK D3D11Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// 设置窗口使用UTF-8
		if (uMsg == WM_CREATE) {
			SetWindowLongW(hWnd, GWL_STYLE, GetWindowLongW(hWnd, GWL_STYLE) | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		}
		
		// 处理输入法消息，以支持中文输入
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
			return true;
		
		// ... 其他消息处理
		return CallWindowProc((WNDPROC)g_D3D11Window->m_OldWndProc, hWnd, uMsg, wParam, lParam);
	}

	//-----------------------------------------------------------------------------------
	//  
	HRESULT APIENTRY D3D11Window::SwapChain_Present_hook(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
	{
		// 保存当前渲染状态
		ID3D11DeviceContext* pContext = nullptr;
		ID3D11RenderTargetView* pRTV = nullptr;
		ID3D11DepthStencilView* pDSV = nullptr;
		D3D11_VIEWPORT viewport;
		UINT numViewports = 1;
		ID3D11BlendState* pBlendState = nullptr;
		FLOAT blendFactor[4];
		UINT sampleMask;
		
		// 获取设备上下文
		pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_D3D11Window->m_Device);
		g_D3D11Window->m_Device->GetImmediateContext(&pContext);
		
		// 保存原始状态
		pContext->OMGetRenderTargets(1, &pRTV, &pDSV);
		pContext->RSGetViewports(&numViewports, &viewport);
		pContext->OMGetBlendState(&pBlendState, blendFactor, &sampleMask);
		
		// 更新我们的 D3D11 状态
		if (g_D3D11Window->m_pSwapChain != pSwapChain)
			g_D3D11Window->m_pSwapChain = pSwapChain;

		// 渲染 ImGui 界面
		g_D3D11Window->Overlay(pSwapChain);
		
		// 恢复原始渲染状态
		pContext->OMSetRenderTargets(1, &pRTV, pDSV);
		pContext->RSSetViewports(numViewports, &viewport);
		pContext->OMSetBlendState(pBlendState, blendFactor, sampleMask);
		
		// 释放引用
		if (pRTV) pRTV->Release();
		if (pDSV) pDSV->Release();
		if (pBlendState) pBlendState->Release();
		if (pContext) pContext->Release();
		
		// 调用原始的 Present 函数以显示游戏内容
		return g_D3D11Window->IDXGISwapChain_Present_stub(pSwapChain, SyncInterval, Flags);
	}

	//-----------------------------------------------------------------------------------
	//  
	HRESULT APIENTRY D3D11Window::SwapChain_ResizeBuffers_hook(IDXGISwapChain* p, UINT bufferCount, UINT Width, UINT Height, DXGI_FORMAT fmt, UINT scFlags)
	{
		//  Get new data & release render target
		g_D3D11Window->m_pSwapChain = p;
		g_D3D11Window->m_RenderTargetView->Release();
		g_D3D11Window->m_RenderTargetView = nullptr;

		//  get fn result
		HRESULT result = g_D3D11Window->IDXGISwapChain_ResizeBuffers_stub(p, bufferCount, Width, Height, fmt, scFlags);

		// Get new render target
		ID3D11Texture2D* backBuffer;
		p->GetBuffer(0, __uuidof(ID3D11Texture2D*), (LPVOID*)&backBuffer);
		if (backBuffer)
		{
			g_D3D11Window->m_Device->CreateRenderTargetView(backBuffer, 0, &g_D3D11Window->m_RenderTargetView);
			backBuffer->Release();
		}

		//  Reset ImGui 
		if (g_D3D11Window->bInitImGui)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(static_cast<float>(Width), static_cast<float>(Height));
		}

		return result;
	}

	bool D3D11Window::HookD3D()
	{
		if (GetD3DContext())
		{
			Hooking::CreateHook((void*)MethodsTable[IDXGI_PRESENT], &SwapChain_Present_hook, (void**)&IDXGISwapChain_Present_stub);
			Hooking::CreateHook((void*)MethodsTable[IDXGI_RESIZE_BUFFERS], &SwapChain_ResizeBuffers_hook, (void**)&IDXGISwapChain_ResizeBuffers_stub);
			bInit = true;
			return true;
		}
		return false;
	}

	void D3D11Window::UnhookD3D()
	{
		SetWindowLongPtr(g_Engine->pGameWindow, GWLP_WNDPROC, (LONG_PTR)m_OldWndProc);
		Hooking::DisableHook((void*)MethodsTable[IDXGI_PRESENT]);
		Hooking::DisableHook((void*)MethodsTable[IDXGI_RESIZE_BUFFERS]);
		free(MethodsTable);
	}

	bool D3D11Window::GetD3DContext()
	{
		if (!InitWindow())
			return false;

		HMODULE D3D11Module = GetModuleHandleA("d3d11.dll");

		D3D_FEATURE_LEVEL FeatureLevel;
		const D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

		DXGI_RATIONAL RefreshRate;
		RefreshRate.Numerator = 60;
		RefreshRate.Denominator = 1;

		DXGI_MODE_DESC BufferDesc;
		BufferDesc.Width = 100;
		BufferDesc.Height = 100;
		BufferDesc.RefreshRate = RefreshRate;
		BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

		DXGI_SAMPLE_DESC SampleDesc;
		SampleDesc.Count = 1;
		SampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_DESC SwapChainDesc;
		SwapChainDesc.BufferDesc = BufferDesc;
		SwapChainDesc.SampleDesc = SampleDesc;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.BufferCount = 1;
		SwapChainDesc.OutputWindow = WindowHwnd;
		SwapChainDesc.Windowed = 1;
		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		IDXGISwapChain* SwapChain;
		ID3D11Device* Device;
		ID3D11DeviceContext* Context;
		if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, FeatureLevels, 1, D3D11_SDK_VERSION, &SwapChainDesc, &SwapChain, &Device, &FeatureLevel, &Context) < 0)
		{
			DeleteWindow();
			return false;
		}

		MethodsTable = (uint64_t*)::calloc(205, sizeof(uint64_t));
		memcpy(MethodsTable, *(uint64_t**)SwapChain, 18 * sizeof(uint64_t));
		memcpy(MethodsTable + 18, *(uint64_t**)Device, 43 * sizeof(uint64_t));
		memcpy(MethodsTable + 18 + 43, *(uint64_t**)Context, 144 * sizeof(uint64_t));
		Sleep(1000);

		//	INIT NOTICE
		Beep(300, 300);

		SwapChain->Release();
		SwapChain = 0;
		Device->Release();
		Device = 0;
		Context->Release();
		Context = 0;
		DeleteWindow();
		return true;
	}

	bool D3D11Window::InitWindow()
	{
		WindowClass.cbSize = sizeof(WNDCLASSEX);
		WindowClass.style = CS_HREDRAW | CS_VREDRAW;
		WindowClass.lpfnWndProc = DefWindowProc;
		WindowClass.cbClsExtra = 0;
		WindowClass.cbWndExtra = 0;
		WindowClass.hInstance = GetModuleHandle(NULL);
		WindowClass.hIcon = NULL;
		WindowClass.hCursor = NULL;
		WindowClass.hbrBackground = NULL;
		WindowClass.lpszMenuName = NULL;
		WindowClass.lpszClassName = L"MJ";
		WindowClass.hIconSm = NULL;
		RegisterClassEx(&WindowClass);
		WindowHwnd = CreateWindow(WindowClass.lpszClassName, L"DX11 Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, WindowClass.hInstance, NULL);
		if (WindowHwnd == NULL) {
			return FALSE;
		}
		return TRUE;
	}

	bool D3D11Window::DeleteWindow()
	{
		DestroyWindow(WindowHwnd);
		UnregisterClass(WindowClass.lpszClassName, WindowClass.hInstance);
		if (WindowHwnd != 0) {
			return FALSE;
		}
		return TRUE;
	}

	bool D3D11Window::InitImGui(IDXGISwapChain* swapChain)
	{
		if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&m_Device))) 
		{
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			Engine::LoadImGuiFonts();
		

			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

			m_Device->GetImmediateContext(&m_DeviceContext);

			DXGI_SWAP_CHAIN_DESC Desc;
			swapChain->GetDesc(&Desc);
			g_Engine->pGameWindow = Desc.OutputWindow;

			ID3D11Texture2D* BackBuffer;
			swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
			m_Device->CreateRenderTargetView(BackBuffer, NULL, &m_RenderTargetView);
			BackBuffer->Release();

			ImGui_ImplWin32_Init(g_Engine->pGameWindow);
			ImGui_ImplDX11_Init(m_Device, m_DeviceContext);
			ImGui_ImplDX11_CreateDeviceObjects();
			m_OldWndProc = (WNDPROC)SetWindowLongPtr(g_Engine->pGameWindow, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
			bInitImGui = true;
			m_pSwapChain = swapChain;
			pImGui = GImGui;
			pViewport = pImGui->Viewports[0];

			//设置默认风格
			DX11Base::Styles::SetModernStyle();
			//加载脚本列表
			Menu::Initialize();

			return true;
		}
		bInitImGui = false;
		return false;
	}

	void D3D11Window::Overlay(IDXGISwapChain* pSwapChain)
	{
		if (!bInitImGui)
			InitImGui(pSwapChain);


		// 设置当前 ImGui 上下文
		ImGui::SetCurrentContext(pImGui);
		
		// 开始新帧
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		// 确保鼠标处理正确
		ImGui::GetIO().MouseDrawCursor = g_Engine->bShowMenu;
		
		// 渲染菜单
		Menu::Draw();
		
		// 完成渲染
		ImGui::Render();
		
		// 设置渲染目标但不清除背景
		m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, NULL);
		
		// 绘制 ImGui 数据
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		
		// 不要在这里调用 Present - 让原始的 Present 函数在 hook 中完成
	}

#pragma endregion


	//----------------------------------------------------------------------------------------------------
	//										HOOKING
	//-----------------------------------------------------------------------------------
#pragma region //	HOOKING

	Hooking::Hooking()
	{
		MH_Initialize();
	}

	Hooking::~Hooking()
	{
		DisableAllHooks();
		RemoveAllHooks();
		MH_Uninitialize();
	}

	void Hooking::Initialize()
	{
		EnableAllHooks();
	}

	void Hooking::Shutdown()
	{
		RemoveAllHooks();
	}

	bool Hooking::CreateHook(LPVOID lpTarget, LPVOID pDetour, LPVOID* pOrig)
	{
		if (MH_CreateHook(lpTarget, pDetour, pOrig) != MH_OK || MH_EnableHook(lpTarget) != MH_OK)
			return false;
		return true;
	}

	void Hooking::EnableHook(LPVOID lpTarget) { MH_EnableHook(lpTarget); }

	void Hooking::DisableHook(LPVOID lpTarget) { MH_DisableHook(lpTarget); }

	void Hooking::RemoveHook(LPVOID lpTarget) { MH_RemoveHook(lpTarget); }

	void Hooking::EnableAllHooks() { MH_EnableHook(MH_ALL_HOOKS); }

	void Hooking::DisableAllHooks() { MH_DisableHook(MH_ALL_HOOKS); }

	void Hooking::RemoveAllHooks() { MH_RemoveHook(MH_ALL_HOOKS); }

#pragma endregion

}