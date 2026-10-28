#include "ComponentRegistry.h"
#include <algorithm>

ComponentData* ComponentStore::Find(const std::string& name)
{
	for (auto& e : entries_) {
		if (e.name == name) return e.data.get();
	}
	return nullptr;
}

const ComponentData* ComponentStore::Find(const std::string& name) const
{
	for (const auto& e : entries_) {
		if (e.name == name) return e.data.get();
	}
	return nullptr;
}

ComponentData* ComponentStore::Add(const std::string& name, std::unique_ptr<ComponentData> data)
{
	if (!data) return nullptr;
	unknown_.erase(name);
	for (auto& e : entries_) {
		if (e.name == name) {
			e.data = std::move(data);
			return e.data.get();
		}
	}
	entries_.push_back({ name, std::move(data) });
	return entries_.back().data.get();
}

bool ComponentStore::Remove(const std::string& name)
{
	const size_t before = entries_.size();
	std::erase_if(entries_, [&](const Entry& e) { return e.name == name; });
	const bool removedUnknown = unknown_.erase(name) > 0;
	return entries_.size() != before || removedUnknown;
}

void ComponentStore::CopyFrom(const ComponentStore& other)
{
	entries_.clear();
	entries_.reserve(other.entries_.size());
	for (const auto& e : other.entries_) {
		entries_.push_back({ e.name, e.data ? e.data->Clone() : nullptr });
	}
	unknown_ = other.unknown_;
}

ComponentRegistry& ComponentRegistry::Get()
{
	static ComponentRegistry registry;
	return registry;
}

ComponentInfo& ComponentRegistry::Register(ComponentInfo info)
{
	for (auto& existing : infos_) {
		if (existing.name == info.name) {
			existing = std::move(info);
			return existing;
		}
	}
	infos_.push_back(std::move(info));
	return infos_.back();
}

bool ComponentRegistry::Unregister(const std::string& name)
{
	const size_t before = infos_.size();
	std::erase_if(infos_, [&](const ComponentInfo& i) { return i.name == name; });
	return infos_.size() != before;
}

const ComponentInfo* ComponentRegistry::Find(const std::string& name) const
{
	for (const auto& info : infos_) {
		if (info.name == name) return &info;
	}
	return nullptr;
}
