#pragma once
#include "base/ScriptAPI.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

class ScriptModule;

/// Custom deleter for ScriptBase instances created inside GameScripts.dll.
/// Ensures all DLL-allocated script instances are destroyed via the DLL's DestroyScript export.
struct ScriptDeleter {
	const ScriptModule* module = nullptr;
	void operator()(ScriptBase* ptr) const;
};

using ScriptInstanceDeleter = ScriptDeleter;
using ScriptInstancePtr = std::unique_ptr<ScriptBase, ScriptDeleter>;
using ScriptFactoryFunc = std::function<std::unique_ptr<ScriptBase>()>;

/// Global registry querying both the loaded GameScripts.dll (via ScriptModule) and any built-in factories.
class ScriptRegistry {
public:
	static ScriptRegistry& GetInstance();

	void SetActiveModule(const ScriptModule* module) { activeModule_ = module; }
	const ScriptModule* GetActiveModule() const { return activeModule_; }

	void Register(const std::string& className, ScriptFactoryFunc factory);
	ScriptInstancePtr Create(const std::string& className) const;
	void DestroyBuiltin(ScriptBase* ptr);
	bool IsRegistered(const std::string& className) const;
	std::vector<std::string> GetRegisteredNames() const;

private:
	ScriptRegistry() = default;
	const ScriptModule* activeModule_ = nullptr;
	std::map<std::string, ScriptFactoryFunc> factories_;
	mutable std::vector<std::unique_ptr<ScriptBase>> builtinInstances_;
};

#ifndef GAMESCRIPTS_EXPORTS
#define REGISTER_SCRIPT(ClassName) \
	namespace { \
		struct ClassName##_ScriptRegistrar { \
			ClassName##_ScriptRegistrar() { \
				ScriptRegistry::GetInstance().Register(#ClassName, []() -> std::unique_ptr<ScriptBase> { \
					return std::make_unique<ClassName>(); \
				}); \
			} \
		}; \
		static ClassName##_ScriptRegistrar s_##ClassName##_registrar; \
	} \
	extern "C" __declspec(dllexport) const void* ForceLinkScript_##ClassName() { \
		return &s_##ClassName##_registrar; \
	} \
	__pragma(comment(linker, "/include:ForceLinkScript_" #ClassName))
#endif
