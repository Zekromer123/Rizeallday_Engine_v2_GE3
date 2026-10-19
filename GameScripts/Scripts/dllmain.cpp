#include "ScriptAPI.h"
#include <windows.h>
#include <cstring>
#include <vector>
#include <memory>
#include <algorithm>

namespace {
	struct RegisteredScriptEntry {
		char name[128]{};
		std::unique_ptr<ScriptBase> (*factory)() = nullptr;
	};

	constexpr int kMaxRegisteredScripts = 256;
	RegisteredScriptEntry g_registeredScripts[kMaxRegisteredScripts]{};
	int g_registeredScriptCount = 0;
	const EngineAPI* g_engineApi = nullptr;
	std::vector<std::unique_ptr<ScriptBase>> g_liveInstances;
}

void GameScripts_RegisterClass(const char* className, std::unique_ptr<ScriptBase> (*factory)()) {
	if (!className || !factory || g_registeredScriptCount >= kMaxRegisteredScripts) {
		return;
	}
	for (int i = 0; i < g_registeredScriptCount; ++i) {
		if (std::strcmp(g_registeredScripts[i].name, className) == 0) {
			g_registeredScripts[i].factory = factory;
			return;
		}
	}
	strncpy_s(g_registeredScripts[g_registeredScriptCount].name, className, sizeof(g_registeredScripts[0].name) - 1);
	g_registeredScripts[g_registeredScriptCount].factory = factory;
	++g_registeredScriptCount;
}

GAMESCRIPTS_API void GameScripts_Init(const EngineAPI* api) {
	g_engineApi = api;
}

GAMESCRIPTS_API ScriptBase* CreateScript(const char* name) {
	if (!name) return nullptr;
	for (int i = 0; i < g_registeredScriptCount; ++i) {
		if (std::strcmp(g_registeredScripts[i].name, name) == 0 && g_registeredScripts[i].factory) {
			std::unique_ptr<ScriptBase> inst = g_registeredScripts[i].factory();
			if (inst) {
				inst->SetEngineAPI(g_engineApi);
				ScriptBase* rawPtr = inst.get();
				g_liveInstances.push_back(std::move(inst));
				return rawPtr;
			}
			return nullptr;
		}
	}
	return nullptr;
}

GAMESCRIPTS_API void DestroyScript(ScriptBase* instance) {
	if (!instance) return;
	auto it = std::find_if(g_liveInstances.begin(), g_liveInstances.end(),
		[instance](const std::unique_ptr<ScriptBase>& p) { return p.get() == instance; });
	if (it != g_liveInstances.end()) {
		g_liveInstances.erase(it);
	}
}

GAMESCRIPTS_API int GetScriptCount() {
	return g_registeredScriptCount;
}

GAMESCRIPTS_API const char* GetScriptName(int index) {
	if (index < 0 || index >= g_registeredScriptCount) {
		return "";
	}
	return g_registeredScripts[index].name;
}

GAMESCRIPTS_API int GetScriptFieldCount(ScriptBase* instance) {
	if (!instance) return 0;
	return instance->GetExposedFieldCount();
}

GAMESCRIPTS_API bool GetScriptFieldInfo(ScriptBase* instance, int fieldIndex, ScriptFieldInfo* outInfo) {
	if (!instance || !outInfo) return false;
	const ScriptFieldInfo* info = instance->GetExposedField(fieldIndex);
	if (!info) return false;
	*outInfo = *info;
	return true;
}

GAMESCRIPTS_API const char* GetScriptFieldName(ScriptBase* instance, int fieldIndex) {
	if (!instance) return "";
	const ScriptFieldInfo* info = instance->GetExposedField(fieldIndex);
	return info ? info->name : "";
}

GAMESCRIPTS_API int GetScriptFieldType(ScriptBase* instance, int fieldIndex) {
	if (!instance) return 0;
	const ScriptFieldInfo* info = instance->GetExposedField(fieldIndex);
	return info ? static_cast<int>(info->type) : 0;
}

GAMESCRIPTS_API void* GetScriptFieldData(ScriptBase* instance, int fieldIndex, uint32_t* outSize) {
	if (!instance) return nullptr;
	const ScriptFieldInfo* info = instance->GetExposedField(fieldIndex);
	if (!info) return nullptr;
	if (outSize) *outSize = info->sizeInBytes;
	return info->dataPtr;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	(void)hModule;
	(void)lpReserved;
	if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		g_liveInstances.clear();
	}
	return TRUE;
}
