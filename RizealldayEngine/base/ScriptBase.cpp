#include "base/ScriptBase.h"
#include "base/ScriptModule.h"
#include <algorithm>

void ScriptDeleter::operator()(ScriptBase* ptr) const {
	if (!ptr) return;
	if (module) {
		module->DestroyScript(ptr);
	} else if (const ScriptModule* activeMod = ScriptRegistry::GetInstance().GetActiveModule()) {
		if (activeMod->IsLoaded()) {
			activeMod->DestroyScript(ptr);
			return;
		}
		ScriptRegistry::GetInstance().DestroyBuiltin(ptr);
	} else {
		ScriptRegistry::GetInstance().DestroyBuiltin(ptr);
	}
}

ScriptRegistry& ScriptRegistry::GetInstance() {
	static ScriptRegistry instance;
	return instance;
}

void ScriptRegistry::Register(const std::string& className, ScriptFactoryFunc factory) {
	if (!className.empty() && factory) {
		factories_[className] = std::move(factory);
	}
}

ScriptInstancePtr ScriptRegistry::Create(const std::string& className) const {
	if (activeModule_ && activeModule_->IsLoaded()) {
		if (ScriptBase* dllInst = activeModule_->CreateScript(className)) {
			return ScriptInstancePtr(dllInst, ScriptDeleter{ activeModule_ });
		}
	}
	auto it = factories_.find(className);
	if (it != factories_.end() && it->second) {
		std::unique_ptr<ScriptBase> inst = it->second();
		if (inst) {
			ScriptBase* rawPtr = inst.get();
			builtinInstances_.push_back(std::move(inst));
			return ScriptInstancePtr(rawPtr, ScriptDeleter{ nullptr });
		}
	}
	return ScriptInstancePtr(nullptr, ScriptDeleter{ activeModule_ });
}

void ScriptRegistry::DestroyBuiltin(ScriptBase* ptr) {
	if (!ptr) return;
	auto it = std::find_if(builtinInstances_.begin(), builtinInstances_.end(),
		[ptr](const std::unique_ptr<ScriptBase>& p) { return p.get() == ptr; });
	if (it != builtinInstances_.end()) {
		builtinInstances_.erase(it);
	}
}

bool ScriptRegistry::IsRegistered(const std::string& className) const {
	if (activeModule_ && activeModule_->IsLoaded() && activeModule_->IsRegistered(className)) {
		return true;
	}
	return factories_.find(className) != factories_.end();
}

std::vector<std::string> ScriptRegistry::GetRegisteredNames() const {
	std::vector<std::string> names;
	if (activeModule_ && activeModule_->IsLoaded()) {
		names = activeModule_->GetRegisteredNames();
	}
	for (const auto& kv : factories_) {
		if (std::find(names.begin(), names.end(), kv.first) == names.end()) {
			names.push_back(kv.first);
		}
	}
	return names;
}
