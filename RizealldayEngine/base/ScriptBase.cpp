#include "base/ScriptBase.h"
#include "base/ScriptModule.h"
#include <algorithm>

void ScriptInstanceDeleter::operator()(ScriptBase* ptr) const {
	if (!ptr) return;
	if (module) {
		module->DestroyScript(ptr);
	} else if (const ScriptModule* activeMod = ScriptRegistry::GetInstance().GetActiveModule()) {
		if (activeMod->IsLoaded()) {
			activeMod->DestroyScript(ptr);
			return;
		}
		delete ptr;
	} else {
		delete ptr;
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
			return ScriptInstancePtr(dllInst, ScriptInstanceDeleter{ activeModule_ });
		}
	}
	auto it = factories_.find(className);
	if (it != factories_.end() && it->second) {
		return ScriptInstancePtr(it->second(), ScriptInstanceDeleter{ nullptr });
	}
	return ScriptInstancePtr(nullptr, ScriptInstanceDeleter{ activeModule_ });
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
