#include <pch.h>
#include <Engine.h>
#include <Menu.h>
#include <luaVM.h>
#include <mutex>  // 添加头文件

namespace DX11Base
{
	
	

	namespace Styles
	{
		// 在初始化 ImGui 时设置样式
		

		//  Hides the Dear ImGui Navigation Interface ( Windowing Mode ) 
		// @TODO: Disable ImGui Navigation
		void SetNavigationMenuViewState(bool bShow)
		{
			ImVec4* colors = ImGui::GetStyle().Colors;
			switch (bShow)
			{
			case true:
			{
				//  Show Navigation Panel | Default ImGui Dark Style
				//  Perhaps just call BaseStyle() ?
				colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
				colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
				colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				break;
			}
			case false:
			{
				//  Hide Navigation Panel
				colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				break;
			}
			}
		}


		void SetModernStyle()
		{
			ImGuiStyle& style = ImGui::GetStyle();
			ImVec4* colors = style.Colors;

			// 深色主题配色 - 暗黑调
			const ImVec4 windowBg = ImVec4(0.031f, 0.031f, 0.031f, 0.925f);      // #080808EC
			const ImVec4 childBg = ImVec4(0.051f, 0.047f, 0.047f, 0.949f);       // #0D0C0CF2
			const ImVec4 titleBg = ImVec4(0.004f, 0.004f, 0.004f, 0.980f);       // #010101FA
			const ImVec4 darkText = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);          // #B3B3B3
			const ImVec4 lighterBg = ImVec4(0.078f, 0.078f, 0.078f, 0.95f);      // #141414F2
			const ImVec4 borderColor = ImVec4(0.157f, 0.157f, 0.157f, 0.50f);    // #282828
			const ImVec4 highlight = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);      // #191919

			// 基础颜色
			colors[ImGuiCol_Text] = darkText;
			colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg] = windowBg;
			colors[ImGuiCol_ChildBg] = childBg;
			colors[ImGuiCol_PopupBg] = childBg;
			colors[ImGuiCol_Border] = borderColor;
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			
			// 框架元素
			colors[ImGuiCol_FrameBg] = ImVec4(0.039f, 0.039f, 0.039f, 0.940f);
			colors[ImGuiCol_FrameBgHovered] = highlight;
			colors[ImGuiCol_FrameBgActive] = lighterBg;
			
			// 标题栏
			colors[ImGuiCol_TitleBg] = titleBg;
			colors[ImGuiCol_TitleBgActive] = titleBg;
			colors[ImGuiCol_TitleBgCollapsed] = titleBg;
			colors[ImGuiCol_MenuBarBg] = titleBg;
			
			// 滚动条
			colors[ImGuiCol_ScrollbarBg] = ImVec4(windowBg.x, windowBg.y, windowBg.z, 0.50f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.078f, 0.078f, 0.078f, 0.80f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.098f, 0.098f, 0.098f, 0.80f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.117f, 0.117f, 0.117f, 0.80f);
			
			// 交互元素
			colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.80f, 0.50f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
			colors[ImGuiCol_Button] = ImVec4(0.059f, 0.059f, 0.059f, 0.940f);
			colors[ImGuiCol_ButtonHovered] = highlight;
			colors[ImGuiCol_ButtonActive] = lighterBg;
			
			// 标签和头部
			colors[ImGuiCol_Header] = ImVec4(0.039f, 0.039f, 0.039f, 0.940f);
			colors[ImGuiCol_HeaderHovered] = highlight;
			colors[ImGuiCol_HeaderActive] = lighterBg;
			
			// Tab相关
			colors[ImGuiCol_Tab] = childBg;
			colors[ImGuiCol_TabHovered] = highlight;
			colors[ImGuiCol_TabActive] = windowBg;
			colors[ImGuiCol_TabUnfocused] = childBg;
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.039f, 0.039f, 0.039f, 0.940f);

			// 样式设置
			style.WindowPadding = ImVec2(8, 8);
			style.WindowRounding = 0.0f;
			style.FramePadding = ImVec2(5, 4);
			style.FrameRounding = 0.0f;
			style.ItemSpacing = ImVec2(8, 4);
			style.ItemInnerSpacing = ImVec2(4, 4);
			style.IndentSpacing = 21.0f;
			style.ScrollbarSize = 12.0f;
			style.ScrollbarRounding = 0.0f;
			style.GrabMinSize = 12.0f;
			style.GrabRounding = 0.0f;
			style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
			style.WindowMenuButtonPosition = ImGuiDir_Right;
			
			// 边框和透明度
			style.Alpha = 1.0f;
			style.ChildRounding = 0.0f;
			style.PopupRounding = 0.0f;
			style.FrameBorderSize = 1.0f;
			style.WindowBorderSize = 1.0f;
		}
	}

	namespace Tabs
	{
		void TABMain()
		{
			ImGui::Text("GMPro (PREVIEW)");
			ImGui::Text("BUILD VERSION: v1.0.1");
			ImGui::Text("BUILD DATE: 2/27/2025");

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();


			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("UNHOOK DLL", ImVec2(ImGui::GetContentRegionAvail().x, 20))) 
			{
				Menu::UninstallHook();

				// 等待一小段时间确保所有线程都已关闭
				Sleep(3000);

				// 设置卸载标志
				g_KillSwitch = TRUE;
			}
		}
	}


	// 添加一个新的命名空间来处理代码编辑器相关的功能
	namespace CodeEditor
	{
		static char EditorBuffer[32768] = "// Write your code here...";
		static bool isFirstEdit = true;
		static bool wordWrap = true;
		static std::vector<std::string> scriptFiles;  // 存储所有脚本文件路径
		static int selectedScript = -1;               // 当前选中的脚本索引
		static char newFileName[256] = "";           // 新文件名输入缓冲
		static bool showNewFileModal = false;        // 是否显示新建文件对话框
		
		// 添加一个标志来指示脚本是否正在执行
		static bool isScriptRunning = false;

		// 获取脚本目录的完整路径
		std::string GetScriptsPath()
		{
			char buffer[MAX_PATH];
			GetModuleFileNameA(NULL, buffer, MAX_PATH);
			std::string path = buffer;
			path = path.substr(0, path.find_last_of("\\/"));
			return path + "\\Scripts\\";
		}

		//执行当前编辑器中的内容
		static void RunScript() {
			// 使用互斥锁确保只有一个线程在执行脚本
			std::lock_guard<std::mutex> lock(scriptMutex);
			
			// 检查脚本是否已经在运行
			if (isScriptRunning) {
				g_Console->cLog("[-] 脚本正在运行中，请稍后再试\n", Console::EColors::EColor_red);
				return;
			}

			// 设置脚本运行标志
			isScriptRunning = true;

			if (g_LuaVM && g_LuaVM->IsInitialized())
			{
				bool success =  g_LuaVM->ExecuteString(EditorBuffer);
				if (success)
				{
					g_Console->cLog("[+] Script executed successfully\n", Console::EColors::EColor_green);
				}
				else
				{
					g_Console->cLog("[-] Script execution failed\n", Console::EColors::EColor_red);
				}
			}

			// 重置脚本运行标志
			isScriptRunning = false;
		}
		

		// 加载所有脚本文件
		static void LoadScriptFiles()
		{
			scriptFiles.clear();
			std::string scriptsPath = GetScriptsPath();
			
			// 确保Scripts目录存在
			CreateDirectoryA(scriptsPath.c_str(), NULL);
			
			WIN32_FIND_DATAA findData;
			HANDLE hFind = FindFirstFileA((scriptsPath + "*.lua").c_str(), &findData);
			
			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
					{
						scriptFiles.push_back(findData.cFileName);
					}
				} while (FindNextFileA(hFind, &findData));
				FindClose(hFind);
			}
		}

		// 加载选中文件的内容
		static void LoadScriptContent(const std::string& filename)
		{
			try 
			{
				std::string fullPath = GetScriptsPath() + filename;
				std::vector<char> buffer;
				
				// 以二进制模式读取文件
				std::ifstream file(fullPath.c_str(), std::ios::binary);
				if (file)
				{
					// 获取文件大小
					file.seekg(0, std::ios::end);
					size_t fileSize = file.tellg();
					file.seekg(0, std::ios::beg);
					
					// 读取文件内容
					buffer.resize(fileSize);
					file.read(buffer.data(), fileSize);
					file.close();

					// 检查并跳过 UTF-8 BOM
					size_t offset = 0;
					if (fileSize >= 3 && 
						(unsigned char)buffer[0] == 0xEF && 
						(unsigned char)buffer[1] == 0xBB && 
						(unsigned char)buffer[2] == 0xBF)
					{
						offset = 3;
					}

					// 直接复制内容到编辑器缓冲区
					size_t contentSize = fileSize - offset;
					if (contentSize < sizeof(EditorBuffer))
					{
						memcpy(EditorBuffer, buffer.data() + offset, contentSize);
						EditorBuffer[contentSize] = '\0';
						isFirstEdit = false;
					}
				}
			}
			catch (const std::exception&) 
			{
				strcpy_s(EditorBuffer, "// Error loading file");
			}
		}

		// 保存编辑器内容到文件
		static void SaveScript(const std::string& filename)
		{
			try 
			{
				std::string fullPath = GetScriptsPath() + filename;
				std::ofstream file(fullPath.c_str(), std::ios::binary);
				
				if (file.is_open())
				{
					// 写入 UTF-8 BOM
					unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
					file.write(reinterpret_cast<char*>(bom), sizeof(bom));
					
					// 直接写入编辑器内容
					file.write(EditorBuffer, strlen(EditorBuffer));
					file.close();
				}
			}
			catch (const std::exception&) 
			{
				// 处理可能的异常
			}
		}

		// 创建新脚本文件
		static void CreateNewScript(const char* filename)
		{
			try 
			{
				std::string fullPath = GetScriptsPath() + filename;
				if (fullPath.find(".lua") == std::string::npos)
				{
					fullPath += ".lua";
				}
				
				std::ofstream file(fullPath.c_str(), std::ios::out | std::ios::binary);
				
				if (file.is_open())
				{
					// 写入 UTF-8 BOM
					unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
					file.write(reinterpret_cast<char*>(bom), sizeof(bom));
					
					// 写入初始内容
					const char* initial_content = "-- New Lua Script\n";
					file.write(initial_content, strlen(initial_content));
					file.close();
					
					LoadScriptFiles();
					
					// 选中新创建的文件
					for (size_t i = 0; i < scriptFiles.size(); i++)
					{
						if (scriptFiles[i] == filename)
						{
							selectedScript = i;
							LoadScriptContent(scriptFiles[i]);
							break;
						}
					}
				}
			}
			catch (const std::exception&) 
			{
				// 处理可能的异常
			}
		}

		// 渲染新建文件对话框
		void RenderNewFileModal()
		{
			// 增加默认大小，确保按钮不会被遮挡
			static ImVec2 modalSize(400, 250);
			
			auto GetTimeString = []() -> std::string {
				time_t now = time(nullptr);
				struct tm timeinfo;
				char buffer[80];
				
				#ifdef _WIN32
					localtime_s(&timeinfo, &now);
				#else
					localtime_r(&now, &timeinfo);
				#endif
				
				strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
				return std::string(buffer);
			};
			
			if (showNewFileModal)
			{
				ImGui::SetNextWindowSize(modalSize, ImGuiCond_FirstUseEver);
				ImGui::OpenPopup("Create New Script");
				
				if (ImGui::BeginPopupModal("Create New Script", &showNewFileModal, 
					ImGuiWindowFlags_AlwaysAutoResize))  // 使用自动调整大小
				{
					// 增加内容区域的内边距
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
					
					ImGui::Text("Enter file name:");
					ImGui::InputText("##FileName", newFileName, sizeof(newFileName));
					
					// 添加垂直空间确保按钮可见
					ImGui::Dummy(ImVec2(0, 20));
					
					// 将按钮对齐到窗口底部
					float buttonWidth = 120;
					float windowWidth = ImGui::GetWindowWidth();
					float totalButtonWidth = 2 * buttonWidth + ImGui::GetStyle().ItemSpacing.x;
					
					ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
					
					if (ImGui::Button("Create", ImVec2(buttonWidth, 0)))
					{
						if (strlen(newFileName) > 0)
						{
							std::string filePath = CodeEditor::GetScriptsPath() + newFileName;
							if (!strstr(newFileName, ".lua"))
								filePath += ".lua";
								
							// 创建文件
							std::ofstream file(filePath);
							if (file.is_open())
							{
								// 修改这里，使用字符串连接而不是 operator<<
								std::string content = "-- ";
								content += newFileName;
								content += "\n-- Created: ";
								content += GetTimeString();
								content += "\n\n";
								
								file.write(content.c_str(), content.length());
								file.close();
								
								// 刷新文件列表并选择新文件
								LoadScriptFiles();
								for (int i = 0; i < scriptFiles.size(); i++)
								{
									std::string filename = scriptFiles[i];
									if (filename == newFileName || filename == std::string(newFileName) + ".lua")
									{
										selectedScript = i;
										LoadScriptContent(filename);
										break;
									}
								}
								
								memset(newFileName, 0, sizeof(newFileName));
								showNewFileModal = false;
							}
						}
					}
					
					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
					{
						memset(newFileName, 0, sizeof(newFileName));
						showNewFileModal = false;
					}
					
					ImGui::PopStyleVar();
					ImGui::EndPopup();
				}
			}
		}

		void RenderCodeEditor()
		{
			// 文件选择下拉框
			if (ImGui::BeginCombo("##ScriptSelector", selectedScript >= 0 ? 
				scriptFiles[selectedScript].c_str() : "Select a script"))
			{
				for (int i = 0; i < scriptFiles.size(); i++)
				{
					bool is_selected = (selectedScript == i);
					if (ImGui::Selectable(scriptFiles[i].c_str(), is_selected))
					{
						if (selectedScript != i)
						{
							selectedScript = i;
							LoadScriptContent(scriptFiles[i]);
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			//新建文件
			ImGui::SameLine();
			if (ImGui::Button("New"))
			{
				showNewFileModal = true;
			}

			//保存文件
			ImGui::SameLine();
			if (ImGui::Button("Save") && selectedScript >= 0)
			{
				SaveScript(scriptFiles[selectedScript]);
			}

			//刷新脚本框
			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
			{
				LoadScriptFiles();
			}

			//执行脚本
			ImGui::SameLine();
			DrawRunButton();

			// 渲染新建文件对话框
			RenderNewFileModal();

			// 计算编辑器大小
			ImVec2 availSpace = ImGui::GetContentRegionAvail();
			ImVec2 editorSize = ImVec2(availSpace.x, availSpace.y);

			// 简化的编辑器代码
			ImGui::InputTextMultiline("##CodeEditor",
				EditorBuffer,
				IM_ARRAYSIZE(EditorBuffer),
				editorSize,
				ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_Multiline,
				nullptr,
				nullptr);
		}

		// 初始化函数
		void Initialize()
		{
			LoadScriptFiles();
		}

		// 在 CodeEditor 命名空间中添加一个简化版的编辑器函数
		void RenderSimplifiedEditor() {
			// 计算编辑器大小
			ImVec2 availSpace = ImGui::GetContentRegionAvail();
			ImVec2 editorSize = ImVec2(availSpace.x, availSpace.y);

			// 编辑器样式设置 - 可以在这里增加背景色
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.05f, 0.05f, 0.05f, 0.97f)); // 稍微亮一点的背景
			
			// 设置更明显的光标颜色
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f)); // 更亮的文本
			
			// 启用制表符支持，增加缩进辅助
			ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | 
										ImGuiInputTextFlags_Multiline;
			
			// 如果启用了自动换行
			if (wordWrap) {
				flags |= ImGuiInputTextFlags_NoHorizontalScroll;
			}
			
			// 使用固定的ID以避免焦点问题
			ImGui::PushID("##FixedCodeEditor");
			
			// 渲染编辑器
			ImGui::InputTextMultiline("##CodeEditor",
				EditorBuffer,
				IM_ARRAYSIZE(EditorBuffer),
				editorSize,
				flags,
				nullptr,
				nullptr);
				
			ImGui::PopID();
			ImGui::PopStyleColor(2); // 恢复颜色样式
		}

		// 在UI中控制按钮的可点击性
		void DrawRunButton() {
			// 如果脚本正在运行，禁用按钮
			if (isScriptRunning) {
				ImGui::BeginDisabled();
			}

			if (ImGui::Button("RUN", ImVec2(ImGui::GetContentRegionAvail().x, 20))) {
				RunScript();
			}

			// 如果按钮被禁用，重新启用
			if (isScriptRunning) {
				ImGui::EndDisabled();
			}
		}
	}


	//----------------------------------------------------------------------------------------------------
	//										MENU
	//-----------------------------------------------------------------------------------
	void Menu::Draw()
	{
		// 移除帧率限制，根据用户建议
		if (g_Engine->bShowMenu)
		{
			// 设置鼠标显示状态
			ImGui::GetIO().MouseDrawCursor = true;
			
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
			MainMenu();
			ImGui::PopStyleVar();

			// 在主窗口绘制完成后绘制日志窗口
			DrawLogWindow();
		}
		else
		{
			// 菜单隐藏时禁用 ImGui 的鼠标绘制
			ImGui::GetIO().MouseDrawCursor = false;
		}

		if (g_Engine->bShowDemoWindow && g_Engine->bShowMenu)
			ImGui::ShowDemoWindow();

		if (g_Engine->bShowStyleEditor && g_Engine->bShowMenu)
			ImGui::ShowStyleEditor();
	}


	void Menu::MainMenu()
	{
		if (!g_Engine->bShowMenu)
			return;

		// 缓存窗口大小和位置
		static ImVec2 lastWindowSize(1000, 600);
		static ImVec2 lastWindowPos(50, 50);

		ImGui::SetNextWindowSize(lastWindowSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(lastWindowPos, ImGuiCond_FirstUseEver);

		// 使用更少的窗口标志
		const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

		if (ImGui::Begin("GM Pro (Preview)", &g_Engine->bShowMenu, window_flags))
		{
			// 缓存当前窗口大小和位置
			lastWindowSize = ImGui::GetWindowSize();
			lastWindowPos = ImGui::GetWindowPos();

			ImVec2 availSpace = ImGui::GetContentRegionAvail();
			
			// 使用固定大小的子窗口来减少布局计算
			const float leftPaneWidth = 200.0f;
			const float rightPaneWidth = availSpace.x - leftPaneWidth - 20.0f;

			// 左侧面板
			ImGui::BeginChild("LeftPane", ImVec2(leftPaneWidth, availSpace.y), true);
			Tabs::TABMain();
			ImGui::EndChild();

			ImGui::SameLine();

			// 右侧面板
			ImGui::BeginChild("RightPane", ImVec2(rightPaneWidth, availSpace.y), true);
			if (ImGui::BeginTabBar("ContentTabs"))
			{
				if (ImGui::BeginTabItem("Settings"))
				{
					// 设置内容
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Debug"))
				{
					if (ImGui::Button("Show Demo Window"))
						g_Engine->bShowDemoWindow = !g_Engine->bShowDemoWindow;
					
					if (ImGui::Button("Show Style Editor"))
						g_Engine->bShowStyleEditor = !g_Engine->bShowStyleEditor;
					
					ImGui::EndTabItem();
				}

				CodeEditor();
				
				ImGui::EndTabBar();
			}
			ImGui::EndChild();
		}
		ImGui::End();

		// 处理其他窗口
		if (g_Engine->bShowDemoWindow)
			ImGui::ShowDemoWindow(&g_Engine->bShowDemoWindow);
		
		if (g_Engine->bShowStyleEditor)
		{
			ImGui::Begin("Style Editor", &g_Engine->bShowStyleEditor);
			ImGui::ShowStyleEditor();
			ImGui::End();
		}
	}

	void Menu::CodeEditor() {
		if (ImGui::BeginTabItem("Code Editor"))
		{
			// 改进布局 - 将按钮分为多行，确保适应不同宽度
			
			// 第一行 - 文件操作
			if (ImGui::BeginCombo("##ScriptSelector", 
				CodeEditor::selectedScript >= 0 ? 
				CodeEditor::scriptFiles[CodeEditor::selectedScript].c_str() : "Select a script", 
				ImGuiComboFlags_HeightLarge))
			{
				for (int i = 0; i < CodeEditor::scriptFiles.size(); i++)
				{
					bool is_selected = (CodeEditor::selectedScript == i);
					if (ImGui::Selectable(CodeEditor::scriptFiles[i].c_str(), is_selected))
					{
						if (CodeEditor::selectedScript != i)
						{
							CodeEditor::selectedScript = i;
							CodeEditor::LoadScriptContent(CodeEditor::scriptFiles[i]);
						}
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// 添加按钮组
			ImGui::SameLine();
			if (ImGui::Button("New")) {
				CodeEditor::showNewFileModal = true;
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Save") && CodeEditor::selectedScript >= 0) {
				CodeEditor::SaveScript(CodeEditor::scriptFiles[CodeEditor::selectedScript]);
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Refresh")) {
				CodeEditor::LoadScriptFiles();
			}

			// 第二行 - 编辑器操作
			if (ImGui::Button("Run")) {
				CodeEditor::RunScript();
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Clear")) {
				memset(CodeEditor::EditorBuffer, 0, sizeof(CodeEditor::EditorBuffer));
				CodeEditor::isFirstEdit = true;
				strcpy_s(CodeEditor::EditorBuffer, "// Write your code here...");
			}
			
			ImGui::SameLine();
			if (ImGui::Button("Copy")) {
				ImGui::SetClipboardText(CodeEditor::EditorBuffer);
			}
			
			ImGui::SameLine();
			ImGui::Checkbox("Word Wrap", &CodeEditor::wordWrap);
			
			// 渲染新建文件对话框
			CodeEditor::RenderNewFileModal();
			
			ImGui::Separator();

			// 设置编辑器区域样式
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8)); // 增加内边距
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			
			if (!CodeEditor::wordWrap) {
				// 使用水平滚动的子窗口，增加左边距确保光标显示
				ImGui::BeginChild("CodeEditorScroll", ImVec2(0, 0), true, 
					ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
			}

			// 增加编辑器左侧边距，确保光标正确显示
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
			
			// 渲染简化版编辑器
			CodeEditor::RenderSimplifiedEditor();
			
			ImGui::PopStyleVar(); // ItemSpacing
			
			if (!CodeEditor::wordWrap) {
				ImGui::EndChild();
			}
			
			ImGui::PopStyleVar(2); // FramePadding, FrameRounding
			ImGui::EndTabItem();
		}
	}


	void Menu::HUD()
	{
		ImVec2 draw_size = g_D3D11Window->pViewport->WorkSize;
		ImVec2 draw_pos = g_D3D11Window->pViewport->WorkSize;
		ImGui::SetNextWindowPos(draw_pos);
		ImGui::SetNextWindowSize(draw_size);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
		if (!ImGui::Begin("##HUDWINDOW", (bool*)true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs))
		{
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::End();
			return;
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImDrawList* ImDraw = ImGui::GetWindowDrawList();
		auto center = ImVec2({ draw_size.x * .5f, draw_size.y * .5f });
		auto top_center = ImVec2({ draw_size.x * .5f, draw_size.y * 0.0f });
		ImDraw->AddText(top_center, ImColor(1.0f, 1.0f, 1.0f, 1.0f), "https://github.com/NightFyre/DX11-ImGui-Internal-Hook");

		ImGui::End();
	}

	void Menu::Loops()
	{

	}

	//----------------------------------------------------------------------------------------------------
	//										GUI
	//-----------------------------------------------------------------------------------

	void GUI::TextCentered(const char* pText)
	{
		ImVec2 textSize = ImGui::CalcTextSize(pText);
		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 textPos = ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f);
		ImGui::SetCursorPos(textPos);
		ImGui::Text("%s", pText);
	}

	//  @ATTN: max buffer is 256chars
	void GUI::TextCenteredf(const char* pText, ...)
	{
		va_list args;
		va_start(args, pText);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), pText, args);
		va_end(args);

		TextCentered(buffer);
	}

	void GUI::DrawText_(ImVec2 pos, ImColor color, const char* pText, float fontSize)
	{
		ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), fontSize, pos, color, pText, pText + strlen(pText), 800, 0);
	}

	//  @ATTN: max buffer is 256chars
	void GUI::DrawTextf(ImVec2 pos, ImColor color, const char* pText, float fontSize, ...)
	{
		va_list args;
		va_start(args, fontSize);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), pText, args);
		va_end(args);

		DrawText_(pos, color, buffer, fontSize);
	}

	void GUI::DrawTextCentered(ImVec2 pos, ImColor color, const char* pText, float fontSize)
	{
		float textSize = ImGui::CalcTextSize(pText).x;
		ImVec2 textPosition = ImVec2(pos.x - (textSize * 0.5f), pos.y);
		DrawText_(textPosition, color, pText, fontSize);
	}

	//  @ATTN: max buffer is 256chars
	void GUI::DrawTextCenteredf(ImVec2 pos, ImColor color, const char* pText, float fontSize, ...)
	{
		va_list args;
		va_start(args, fontSize);
		char buffer[256];
		vsnprintf(buffer, sizeof(buffer), pText, args);
		va_end(args);

		DrawTextCentered(pos, color, pText, fontSize);
	}

	// 使用更简单的方法显示日志
	void Menu::DrawLogWindow() {
		if (!g_Engine->bShowMenu) return;

		static ImVec2 logWindowSize(500, 200);
		
		ImGui::SetNextWindowSize(logWindowSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 220), ImGuiCond_FirstUseEver);
		
		if (!ImGui::Begin("Console Log", nullptr, ImGuiWindowFlags_NoCollapse)) {
			ImGui::End();
			return;
		}
		
		logWindowSize = ImGui::GetWindowSize();

		// 控制按钮
		if (ImGui::Button("Clear")) {
			g_Console->Clear();
		}
		
		ImGui::SameLine();
		bool& autoScroll = g_Console->AutoScroll;
		ImGui::Checkbox("Auto-scroll", &autoScroll);

		ImGui::Separator();

		// 确保使用正确的字体 - 这里我们尝试使用字体索引0，通常是默认加载的中文字体
		if (ImGui::GetIO().Fonts->Fonts.Size > 0) {
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		}

		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		
		// 修改文本颜色设置为白色，在黑色背景上清晰可见
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		
		// 为不同类型的日志设置不同颜色
		// 可以在 Console::Log 和 Console::cLog 中添加颜色标记，然后这里根据标记设置不同颜色
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // 普通日志用浅灰色
		// 错误日志可以用红色: ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
		// 警告日志可以用黄色: ImVec4(1.0f, 0.9f, 0.4f, 1.0f)
		// 成功日志可以用绿色: ImVec4(0.4f, 1.0f, 0.4f, 1.0f)
		
		// 简单直接地显示文本缓冲区
		ImGui::TextUnformatted(g_Console->logBuffer.begin(), g_Console->logBuffer.end());
		
		ImGui::PopStyleColor();
		
		// 自动滚动
		if (g_Console->ScrollToBottom || (g_Console->AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
			ImGui::SetScrollHereY(1.0f);
		}
		g_Console->ScrollToBottom = false;

		ImGui::EndChild();
		
		// 恢复字体
		if (ImGui::GetIO().Fonts->Fonts.Size > 0) {
			ImGui::PopFont();
		}
		
		ImGui::End();
	}

	void Menu::Initialize()
	{
		CodeEditor::Initialize();
	}

	void Menu::UninstallHook() {
		g_Console->cLog("[+] 开始卸载DLL", Console::EColors::EColor_red);

		// 先关闭 LuaVM
		if (g_LuaVM) {
			g_LuaVM->Shutdown();
			g_LuaVM.reset();
			g_Console->cLog("[+] LuaVM 已关闭", Console::EColors::EColor_red);
		}

		// 确保所有后台线程都已终止
		g_Running = false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待线程终止

		
		// 其他卸载操作
		// ...

		g_Console->cLog("[+] DLL 卸载完成", Console::EColors::EColor_red);
	}

}