#include "EditorSuiteInternal.h"

void EditorSuite::DrawCppScriptGeneratorModal()
{
#ifdef USE_IMGUI
	ImGui::OpenPopup("Unity-Style C++ Script Generator");
	if (ImGui::BeginPopupModal("Unity-Style C++ Script Generator", &showScriptGenModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create New C++ Script in GameScripts/Scripts/ (Derives from ScriptBase)");
		ImGui::Separator();

		if (ImGui::InputText("Class Name", scriptClassNameBuf_, sizeof(scriptClassNameBuf_))) {
			scriptGenErrorMsg_.clear();
		}
		ImGui::Checkbox("Include OnCollision Override", &scriptIncludeCollision_);
		std::filesystem::path targetScriptsDir = ResolveGameScriptsDir() / "Scripts";
		ImGui::TextDisabled("Target folder: %s", targetScriptsDir.string().c_str());
		ImGui::TextDisabled("Generates <ClassName>.h and <ClassName>.cpp, builds GameScripts.dll, and opens in Visual Studio.");

		if (!scriptGenErrorMsg_.empty()) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", scriptGenErrorMsg_.c_str());
		}

		ImGui::Separator();
		if (ImGui::Button("Create & Open", ImVec2(140.0f, 0.0f))) {
			if (GenerateCppScriptFiles(scriptClassNameBuf_, targetScriptsDir.string(), scriptIncludeCollision_)) {
				showScriptGenModal_ = false;
				scriptGenTargetObjectIndex_ = -1;
				scriptGenErrorMsg_.clear();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
			showScriptGenModal_ = false;
			scriptGenTargetObjectIndex_ = -1;
			scriptGenErrorMsg_.clear();
		}
		ImGui::EndPopup();
	}
#endif
}

bool EditorSuite::GenerateCppScriptFiles(const std::string& className, const std::string& folderPath, bool includeCollision)
{
	// Validate C++ identifier
	if (className.empty()) {
		scriptGenErrorMsg_ = "Error: Class Name cannot be empty.";
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}
	if (!(std::isalpha(static_cast<unsigned char>(className.front())) || className.front() == '_')) {
		scriptGenErrorMsg_ = "Error: Class Name must start with a letter or underscore.";
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}
	for (char c : className) {
		if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) {
			scriptGenErrorMsg_ = "Error: Class Name must contain only letters, digits, or underscores.";
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
	}

	std::filesystem::path targetDir = folderPath.empty() ? (ResolveGameScriptsDir() / "Scripts") : std::filesystem::path(folderPath);
	std::error_code ec;
	std::filesystem::create_directories(targetDir, ec);
	std::filesystem::path headerPath = targetDir / (className + ".h");
	std::filesystem::path sourcePath = targetDir / (className + ".cpp");

	if (scriptModule_.IsRegistered(className) || std::filesystem::exists(headerPath, ec) || std::filesystem::exists(sourcePath, ec)) {
		scriptGenErrorMsg_ = std::format("Error: Script '{}' already exists.", className);
		LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
		return false;
	}

	// Generate <ClassName>.h deriving from ScriptBase with EXPOSE_FIELD sample
	{
		std::ofstream h(headerPath);
		if (!h.is_open()) {
			scriptGenErrorMsg_ = std::format("Error: Failed to write header file: {}", headerPath.string());
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
		h << "#pragma once\n";
		h << "#include \"base/ScriptAPI.h\"\n\n";
		h << "// Note: Fields not marked EXPOSE_FIELD are not preserved across a script hot reload.\n";
		h << "class " << className << " : public ScriptBase {\n";
		h << "public:\n";
		h << "    EXPOSE_FIELD(float, speed, 1.0f);\n\n";
		h << "    void Initialize() override;\n";
		h << "    void Update(float deltaTime) override;\n";
		if (includeCollision) {
			h << "    void OnCollision(GameObject& other) override;\n";
		}
		h << "    void OnDestroy() override;\n";
		h << "};\n";
		h.close();
	}

	// Generate <ClassName>.cpp with method stubs and REGISTER_SCRIPT(ClassName)
	{
		std::ofstream cpp(sourcePath);
		if (!cpp.is_open()) {
			scriptGenErrorMsg_ = std::format("Error: Failed to write source file: {}", sourcePath.string());
			LogMessage("[ScriptGen] " + scriptGenErrorMsg_, 2);
			return false;
		}
		cpp << "#include \"" << className << ".h\"\n\n";
		cpp << "void " << className << "::Initialize() {\n";
		cpp << "    if (GameObject* self = GetOwner()) {\n";
		cpp << "        (void)self;\n";
		cpp << "    }\n";
		cpp << "}\n\n";
		cpp << "void " << className << "::Update(float deltaTime) {\n";
		cpp << "    if (GameObject* self = GetOwner()) {\n";
		cpp << "        self->rotation.y += speed * deltaTime;\n";
		cpp << "        self->rotationQuat = Quaternion::FromEuler(self->rotation);\n";
		cpp << "    }\n";
		cpp << "}\n\n";
		if (includeCollision) {
			cpp << "void " << className << "::OnCollision(GameObject& other) {\n";
			cpp << "    (void)other;\n";
			cpp << "}\n\n";
		}
		cpp << "void " << className << "::OnDestroy() {\n";
		cpp << "}\n\n";
		cpp << "REGISTER_SCRIPT(" << className << ")\n";
		cpp.close();
	}

	// Register the new script files in GameScripts.vcxproj and GameScripts.vcxproj.filters under the Scripts filter
	{
		std::filesystem::path vcxprojPath = ResolveGameScriptsVcxproj();
		std::filesystem::path filtersPath = vcxprojPath.string() + ".filters";
		std::string relCpp = "Scripts\\" + className + ".cpp";
		std::string relH = "Scripts\\" + className + ".h";

		auto insertBeforeToken = [](const std::filesystem::path& file, const std::string& checkSubstr, const std::string& anchor, const std::string& insertion) {
			std::ifstream in(file);
			if (!in.is_open()) return;
			std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			in.close();
			if (content.find(checkSubstr) != std::string::npos) return;
			size_t pos = content.find(anchor);
			if (pos != std::string::npos) {
				content.insert(pos, insertion);
				std::ofstream out(file, std::ios::trunc);
				if (out.is_open()) {
					out << content;
				}
			}
		};

		insertBeforeToken(
			vcxprojPath,
			relCpp,
			"    <ClCompile Include=\"Scripts\\dllmain.cpp\" />",
			"    <ClCompile Include=\"" + relCpp + "\" />\n");
		insertBeforeToken(
			vcxprojPath,
			relH,
			"    <ClInclude Include=\"..\\RizealldayEngine\\base\\ScriptAPI.h\" />",
			"    <ClInclude Include=\"" + relH + "\" />\n");

		insertBeforeToken(
			filtersPath,
			relCpp,
			"    <ClCompile Include=\"Scripts\\dllmain.cpp\">",
			"    <ClCompile Include=\"" + relCpp + "\">\n      <Filter>Scripts</Filter>\n    </ClCompile>\n");
		insertBeforeToken(
			filtersPath,
			relH,
			"    <ClInclude Include=\"..\\RizealldayEngine\\base\\ScriptAPI.h\">",
			"    <ClInclude Include=\"" + relH + "\">\n      <Filter>Scripts</Filter>\n    </ClInclude>\n");
	}

	LogMessage(std::format("[Scripts] Created {}.h and {}.cpp in {}", className, className, targetDir.string()), 0);

	// 1. Attach <ClassName> to the target GameObject in a "Pending / Compiling..." state
	int attachIdx = (scriptGenTargetObjectIndex_ >= 0 && scriptGenTargetObjectIndex_ < static_cast<int>(gameObjects_.size()))
		? scriptGenTargetObjectIndex_
		: selectedObjectIndex_;
	if (attachIdx >= 0 && attachIdx < static_cast<int>(gameObjects_.size())) {
		auto& targetObj = gameObjects_[attachIdx];
		GameObject::ScriptComponent sc(className);
		sc.isCompiling = true;
		sc.isMissingScript = false;
		targetObj.scripts.push_back(std::move(sc));
		targetObj.hasScript = true;
		targetObj.scriptClassName = className;
		isSceneDirty_ = true;
		PushUndoSnapshot();
		LogMessage(std::format("[Scripts] Attached '{}' (Pending / Compiling...) to '{}'", className, targetObj.name), 0);
	}

	// 2. Trigger Build Scripts automatically
	TriggerScriptBuild();

	// 3. Open <ClassName>.cpp in Visual Studio
	OpenFileInVisualStudio(sourcePath);

	return true;
}

std::filesystem::path EditorSuite::ResolveGameScriptsDir() const
{
	std::error_code ec;
	std::filesystem::path dir = ScriptModule::ResolveWorkspaceRoot() / ScriptModule::kGameScriptsDirName;
	if (std::filesystem::exists(dir, ec)) {
		return std::filesystem::weakly_canonical(dir, ec);
	}
	return dir;
}

std::filesystem::path EditorSuite::ResolveGameScriptsVcxproj() const
{
	std::error_code ec;
	std::filesystem::path vcxproj = ScriptModule::ResolveWorkspaceRoot() / ScriptModule::kGameScriptsVcxprojSubpath;
	if (std::filesystem::exists(vcxproj, ec)) {
		return std::filesystem::weakly_canonical(vcxproj, ec);
	}
	return vcxproj;
}

std::filesystem::path EditorSuite::ResolveSolutionFilePath() const
{
	std::error_code ec;
	std::filesystem::path vcxproj = ResolveGameScriptsVcxproj();
	std::filesystem::path rootDir = vcxproj.parent_path().parent_path();
	if (std::filesystem::exists(rootDir, ec)) {
		for (const auto& entry : std::filesystem::directory_iterator(rootDir, ec)) {
			if (entry.is_regular_file()) {
				auto ext = entry.path().extension().string();
				if (ext == ".slnx" || ext == ".sln") {
					return std::filesystem::canonical(entry.path(), ec);
				}
			}
		}
	}
	return {};
}

std::string EditorSuite::ResolveMsBuildExe()
{
	if (!resolvedMsBuildPath_.empty() && std::filesystem::exists(resolvedMsBuildPath_)) {
		return resolvedMsBuildPath_;
	}

	std::error_code ec;
	// 1. Try vswhere.exe
	std::filesystem::path vswhere = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
	if (std::filesystem::exists(vswhere, ec)) {
		SECURITY_ATTRIBUTES sa{};
		sa.nLength = sizeof(sa);
		sa.bInheritHandle = TRUE;
		HANDLE rPipe = NULL, wPipe = NULL;
		if (CreatePipe(&rPipe, &wPipe, &sa, 0)) {
			SetHandleInformation(rPipe, HANDLE_FLAG_INHERIT, 0);
			STARTUPINFOW si{};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			si.hStdOutput = wPipe;
			si.hStdError = wPipe;
			PROCESS_INFORMATION pi{};
			std::wstring cmd = L"\"" + vswhere.wstring() + L"\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe";
			std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
			cmdBuf.push_back(L'\0');
			if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
				CloseHandle(wPipe);
				wPipe = NULL;
				std::string output;
				char buf[512];
				DWORD bytesRead = 0;
				while (ReadFile(rPipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
					buf[bytesRead] = '\0';
					output.append(buf, bytesRead);
				}
				WaitForSingleObject(pi.hProcess, 3000);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				std::istringstream iss(output);
				std::string line;
				while (std::getline(iss, line)) {
					while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
						line.pop_back();
					}
					if (!line.empty() && std::filesystem::exists(line, ec)) {
						resolvedMsBuildPath_ = line;
						CloseHandle(rPipe);
						return resolvedMsBuildPath_;
					}
				}
			}
			if (wPipe) CloseHandle(wPipe);
			CloseHandle(rPipe);
		}
	}

	// 2. Check common Visual Studio 18 / 2022 paths and fallback buffer
	std::vector<std::string> fallbacks = {
		msbuildFallbackPathBuf_,
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\MSBuild\\Current\\Bin\\MSBuild.exe",
		"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe"
	};

	for (const auto& candidate : fallbacks) {
		if (!candidate.empty() && std::filesystem::exists(candidate, ec)) {
			resolvedMsBuildPath_ = candidate;
			return resolvedMsBuildPath_;
		}
	}

	return msbuildFallbackPathBuf_;
}

void EditorSuite::TriggerScriptBuild()
{
	if (isBuildingScripts_.load()) {
		queuedScriptRebuild_.store(true);
		LogMessage("[Scripts] Build already in progress; queued another rebuild when it finishes.", 0);
		return;
	}

	if (scriptBuildThread_.joinable()) {
		scriptBuildThread_.join();
	}

	std::string msbuildExe = ResolveMsBuildExe();
	std::filesystem::path vcxprojPath = ResolveGameScriptsVcxproj();
	std::error_code ec;
	if (!std::filesystem::exists(msbuildExe, ec)) {
		LogMessage(std::format("[Scripts] Error: MSBuild.exe not found at '{}'.", msbuildExe), 2);
		lastScriptBuildStatus_.store(2);
		return;
	}
	if (!std::filesystem::exists(vcxprojPath, ec)) {
		LogMessage(std::format("[Scripts] Error: GameScripts.vcxproj not found at '{}'.", vcxprojPath.string()), 2);
		lastScriptBuildStatus_.store(2);
		return;
	}

	isBuildingScripts_.store(true);
	queuedScriptRebuild_.store(false);
	LogMessage("[Scripts] Building GameScripts.vcxproj (Development|x64)...", 0);

	scriptBuildThread_ = std::thread([this, msbuildExe, vcxprojPath]() {
		bool runAgain = true;
		while (runAgain) {
			runAgain = false;
			auto startTime = std::chrono::steady_clock::now();

			SECURITY_ATTRIBUTES sa{};
			sa.nLength = sizeof(sa);
			sa.bInheritHandle = TRUE;
			HANDLE rPipe = NULL, wPipe = NULL;
			if (!CreatePipe(&rPipe, &wPipe, &sa, 0)) {
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[Scripts] Build failed: CreatePipe error.", 2 });
				lastScriptBuildStatus_.store(2);
				break;
			}
			SetHandleInformation(rPipe, HANDLE_FLAG_INHERIT, 0);

			STARTUPINFOW si{};
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			si.hStdOutput = wPipe;
			si.hStdError = wPipe;

			PROCESS_INFORMATION pi{};
			std::wstring cmdLine = L"\"" + std::filesystem::path(msbuildExe).wstring() + L"\" \""
				+ vcxprojPath.wstring() + L"\" /t:Build /p:Configuration=Development /p:Platform=x64 /v:minimal /nologo";
			std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
			cmdBuf.push_back(L'\0');

			std::wstring workDir = vcxprojPath.parent_path().wstring();
			BOOL started = CreateProcessW(
				NULL,
				cmdBuf.data(),
				NULL,
				NULL,
				TRUE,
				CREATE_NO_WINDOW,
				NULL,
				workDir.c_str(),
				&si,
				&pi);

			CloseHandle(wPipe);
			wPipe = NULL;

			if (!started) {
				CloseHandle(rPipe);
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[Scripts] Build failed: Could not start MSBuild.exe process.", 2 });
				lastScriptBuildStatus_.store(2);
				break;
			}

			std::string pendingText;
			char buffer[1024];
			DWORD bytesRead = 0;
			auto pushOutputLine = [this](std::string line) {
				while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
					line.pop_back();
				}
				if (line.empty()) return;
				int logType = 0;
				std::string lower = line;
				std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (lower.find(": error ") != std::string::npos || lower.find("fatal error") != std::string::npos) {
					logType = 2;
				} else if (lower.find(": warning ") != std::string::npos) {
					logType = 1;
				}
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				pendingBuildLogs_.push_back({ "[MSBuild] " + line, logType });
			};

			while (ReadFile(rPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
				buffer[bytesRead] = '\0';
				pendingText.append(buffer, bytesRead);
				size_t newlinePos = 0;
				while ((newlinePos = pendingText.find('\n')) != std::string::npos) {
					std::string line = pendingText.substr(0, newlinePos);
					pendingText.erase(0, newlinePos + 1);
					pushOutputLine(std::move(line));
				}
			}
			if (!pendingText.empty()) {
				pushOutputLine(std::move(pendingText));
			}

			WaitForSingleObject(pi.hProcess, INFINITE);
			DWORD exitCode = 1;
			GetExitCodeProcess(pi.hProcess, &exitCode);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			CloseHandle(rPipe);

			auto endTime = std::chrono::steady_clock::now();
			double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

			{
				std::lock_guard<std::mutex> lock(buildLogMutex_);
				if (exitCode == 0) {
					pendingBuildLogs_.push_back({ std::format("[Scripts] Build succeeded ({:.2f}s)", elapsedSec), 0 });
					lastScriptBuildStatus_.store(1);
				} else {
					pendingBuildLogs_.push_back({ std::format("[Scripts] Build failed (exit code {})", exitCode), 2 });
					lastScriptBuildStatus_.store(2);
				}
			}

			if (queuedScriptRebuild_.exchange(false)) {
				runAgain = true;
			}
		}

		isBuildingScripts_.store(false);
	});
}

void EditorSuite::DrainScriptBuildLogs()
{
	std::vector<PendingBuildLog> logs;
	{
		std::lock_guard<std::mutex> lock(buildLogMutex_);
		if (pendingBuildLogs_.empty()) return;
		logs.swap(pendingBuildLogs_);
	}

	bool hadError = false;
	for (const auto& entry : logs) {
		LogMessage(entry.line, entry.type);
		if (entry.type == 2) {
			hadError = true;
		}
	}
	if (hadError) {
		bottomPanelTab_ = 1; // Focus Console tab on build error
	}
}

void EditorSuite::PollScriptSourceWatcher(float deltaTime)
{
	scriptSourcePollTimer_ += deltaTime;
	if (scriptSourcePollTimer_ >= 0.25f) {
		scriptSourcePollTimer_ = 0.0f;

		std::filesystem::path scriptsDir = ResolveGameScriptsDir() / "Scripts";
		std::error_code ec;
		if (std::filesystem::exists(scriptsDir, ec) && std::filesystem::is_directory(scriptsDir, ec)) {
			std::map<std::string, std::filesystem::file_time_type> currentStamps;
			for (const auto& entry : std::filesystem::directory_iterator(scriptsDir, ec)) {
				if (!entry.is_regular_file(ec)) continue;
				std::string ext = entry.path().extension().string();
				if (ext == ".cpp" || ext == ".h") {
					currentStamps[entry.path().filename().string()] = std::filesystem::last_write_time(entry.path(), ec);
				}
			}

			if (!scriptSourceWatcherInitialized_) {
				scriptSourceTimestamps_ = std::move(currentStamps);
				scriptSourceWatcherInitialized_ = true;
			} else if (currentStamps != scriptSourceTimestamps_) {
				scriptSourceTimestamps_ = std::move(currentStamps);
				if (autoBuildScripts_) {
					scriptSourceChangePending_ = true;
					scriptSourceDebounceTimer_ = 0.0f;
				}
			}
		}
	}

	if (scriptSourceChangePending_ && autoBuildScripts_) {
		scriptSourceDebounceTimer_ += deltaTime;
		if (scriptSourceDebounceTimer_ >= 1.0f) {
			scriptSourceChangePending_ = false;
			scriptSourceDebounceTimer_ = 0.0f;
			LogMessage("[Scripts] Detected source changes in GameScripts/Scripts; triggering Auto Build...", 0);
			TriggerScriptBuild();
		}
	}
}

void EditorSuite::OpenFileInVisualStudio(const std::filesystem::path& filePath)
{
	std::error_code ec;
	if (!std::filesystem::exists(filePath, ec)) {
		LogMessage(std::format("[Scripts] Cannot open file (does not exist): {}", filePath.string()), 1);
		return;
	}

	std::filesystem::path absFile = std::filesystem::canonical(filePath, ec);
	if (ec) absFile = std::filesystem::absolute(filePath, ec);

	std::thread([this, absFile]() {
		auto logAsync = [this](const std::string& text, int type = 0) {
			std::lock_guard<std::mutex> lock(buildLogMutex_);
			pendingBuildLogs_.push_back({ text, type });
		};

		// 1. Try attaching to a running Visual Studio DTE instance via Running Object Table (ROT)
		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		bool comInitialized = SUCCEEDED(hr) || hr == S_FALSE;
		bool openedViaDte = false;

		if (comInitialized) {
			{
				Microsoft::WRL::ComPtr<IRunningObjectTable> pRot = nullptr;
				if (SUCCEEDED(GetRunningObjectTable(0, &pRot)) && pRot) {
					Microsoft::WRL::ComPtr<IEnumMoniker> pEnum = nullptr;
					if (SUCCEEDED(pRot->EnumRunning(&pEnum)) && pEnum) {
						Microsoft::WRL::ComPtr<IMoniker> pMoniker = nullptr;
						ULONG fetched = 0;
						while (!openedViaDte && pEnum->Next(1, &pMoniker, &fetched) == S_OK) {
							Microsoft::WRL::ComPtr<IBindCtx> pBindCtx = nullptr;
							if (SUCCEEDED(CreateBindCtx(0, &pBindCtx)) && pBindCtx) {
								LPOLESTR displayName = nullptr;
								if (SUCCEEDED(pMoniker->GetDisplayName(pBindCtx.Get(), nullptr, &displayName)) && displayName) {
									std::wstring disp(displayName);
									CoTaskMemFree(displayName);
									if (disp.find(L"!VisualStudio.DTE") == 0) {
										Microsoft::WRL::ComPtr<IUnknown> pUnk = nullptr;
										if (SUCCEEDED(pRot->GetObject(pMoniker.Get(), &pUnk)) && pUnk) {
											Microsoft::WRL::ComPtr<IDispatch> pDte = nullptr;
											if (SUCCEEDED(pUnk.As(&pDte)) && pDte) {
												// Get ItemOperations property on DTE
												OLECHAR itemOpsName[] = L"ItemOperations";
												LPOLESTR names[1] = { itemOpsName };
												DISPID dispidItemOps = DISPID_UNKNOWN;
												if (SUCCEEDED(pDte->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &dispidItemOps))) {
													DISPPARAMS noArgs = { nullptr, nullptr, 0, 0 };
													VARIANT vItemOps;
													VariantInit(&vItemOps);
													if (SUCCEEDED(pDte->Invoke(dispidItemOps, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &vItemOps, nullptr, nullptr))
														&& vItemOps.vt == VT_DISPATCH && vItemOps.pdispVal) {
														IDispatch* pItemOps = vItemOps.pdispVal;
														OLECHAR openFileName[] = L"OpenFile";
														LPOLESTR openNames[1] = { openFileName };
														DISPID dispidOpenFile = DISPID_UNKNOWN;
														if (SUCCEEDED(pItemOps->GetIDsOfNames(IID_NULL, openNames, 1, LOCALE_USER_DEFAULT, &dispidOpenFile))) {
															BSTR bstrFile = SysAllocString(absFile.wstring().c_str());
															VARIANT vArg;
															VariantInit(&vArg);
															vArg.vt = VT_BSTR;
															vArg.bstrVal = bstrFile;
															DISPPARAMS openParams = { &vArg, nullptr, 1, 0 };
															VARIANT vRes;
															VariantInit(&vRes);
															if (SUCCEEDED(pItemOps->Invoke(dispidOpenFile, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &openParams, &vRes, nullptr, nullptr))) {
																openedViaDte = true;
															}
															VariantClear(&vRes);
															SysFreeString(bstrFile);
														}

														// Also activate MainWindow if available
														OLECHAR mainWinName[] = L"MainWindow";
														LPOLESTR mwNames[1] = { mainWinName };
														DISPID dispidMw = DISPID_UNKNOWN;
														if (SUCCEEDED(pDte->GetIDsOfNames(IID_NULL, mwNames, 1, LOCALE_USER_DEFAULT, &dispidMw))) {
															VARIANT vMw;
															VariantInit(&vMw);
															if (SUCCEEDED(pDte->Invoke(dispidMw, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &vMw, nullptr, nullptr))
																&& vMw.vt == VT_DISPATCH && vMw.pdispVal) {
																OLECHAR actName[] = L"Activate";
																LPOLESTR actNames[1] = { actName };
																DISPID dispidAct = DISPID_UNKNOWN;
																if (SUCCEEDED(vMw.pdispVal->GetIDsOfNames(IID_NULL, actNames, 1, LOCALE_USER_DEFAULT, &dispidAct))) {
																	VARIANT vDummy;
																	VariantInit(&vDummy);
																	vMw.pdispVal->Invoke(dispidAct, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &noArgs, &vDummy, nullptr, nullptr);
																	VariantClear(&vDummy);
																}
															}
															VariantClear(&vMw);
														}
													}
													VariantClear(&vItemOps);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			CoUninitialize();
		}

		if (openedViaDte) {
			logAsync(std::format("[Scripts] Opened '{}' in running Visual Studio instance via DTE.", absFile.filename().string()), 0);
			return;
		}

		// 2. Fallback: Locate devenv.exe and launch with /edit "<filePath>"
		std::error_code localEc;
		std::vector<std::filesystem::path> devenvCandidates = {
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Professional\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\Common7\\IDE\\devenv.exe",
			"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\Common7\\IDE\\devenv.exe"
		};

		for (const auto& devenv : devenvCandidates) {
			if (std::filesystem::exists(devenv, localEc)) {
				std::wstring cmd = L"\"" + devenv.wstring() + L"\" /edit \"" + absFile.wstring() + L"\"";
				std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
				cmdBuf.push_back(L'\0');
				STARTUPINFOW si{};
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi{};
				if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					logAsync(std::format("[Scripts] Opened '{}' via devenv.exe /edit.", absFile.filename().string()), 0);
					return;
				}
			}
		}

		// 3. Final fallback: ShellExecuteW default handler
		HINSTANCE res = ShellExecuteW(NULL, L"open", absFile.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
		if (reinterpret_cast<INT_PTR>(res) > 32) {
			logAsync(std::format("[Scripts] Opened '{}' with default system association (ShellExecuteW).", absFile.filename().string()), 0);
		} else {
			logAsync(std::format("[Scripts] Warning: Failed to open '{}' in Visual Studio or default editor.", absFile.string()), 1);
		}
	}).detach();
}


