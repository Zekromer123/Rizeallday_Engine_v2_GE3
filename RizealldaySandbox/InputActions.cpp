#include "InputActions.h"
#include "SceneJson.h"
#include "base/ScriptAPI.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <format>

using Axis = InputBinding::Axis;

InputActions InputActions::Defaults()
{
	InputActions a;
	a.Add("MoveX").bindings = { InputBinding::Key(KEY_A, -1.0f), InputBinding::Key(KEY_D), InputBinding::Key(KEY_LEFT, -1.0f),
		InputBinding::Key(KEY_RIGHT), InputBinding::PadAxis(Axis::LeftX) };
	a.Add("MoveY").bindings = { InputBinding::Key(KEY_S, -1.0f), InputBinding::Key(KEY_W), InputBinding::Key(KEY_DOWN, -1.0f),
		InputBinding::Key(KEY_UP), InputBinding::PadAxis(Axis::LeftY) };
	a.Add("Jump").bindings = { InputBinding::Key(KEY_SPACE), InputBinding::Pad(PAD_A) };
	a.Add("Attack").bindings = { InputBinding::Key(KEY_J), InputBinding::Mouse(0), InputBinding::Pad(PAD_X) };
	a.Add("Submit").bindings = { InputBinding::Key(KEY_ENTER), InputBinding::Key(KEY_SPACE), InputBinding::Pad(PAD_A) };
	a.Add("Cancel").bindings = { InputBinding::Key(KEY_ESCAPE), InputBinding::Key(KEY_BACKSPACE), InputBinding::Pad(PAD_B) };
	a.Add("Pause").bindings = { InputBinding::Key(KEY_ESCAPE), InputBinding::Key(KEY_P), InputBinding::Pad(PAD_START) };
	return a;
}

float InputActions::BindingValue(const InputBinding& b, const Source& src)
{
	switch (b.type) {
	case InputBinding::Type::Key: return (src.keyDown && src.keyDown(b.code)) ? b.sign : 0.0f;
	case InputBinding::Type::Mouse: return (src.mouseDown && src.mouseDown(b.code)) ? b.sign : 0.0f;
	case InputBinding::Type::PadButton: return (src.padDown && src.padDown(b.code)) ? b.sign : 0.0f;
	case InputBinding::Type::PadAxis: {
		const float v = src.padAxis ? src.padAxis(b.code) : 0.0f;
		return (std::isfinite(v) && std::fabs(v) >= kDeadZone) ? v * b.sign : 0.0f;
	}
	}
	return 0.0f;
}

bool InputActions::IsDown(const std::string& action, const Source& src) const
{
	const InputAction* a = Find(action);
	if (!a) return false;
	for (const InputBinding& b : a->bindings) {
		const float v = BindingValue(b, src);
		// Buttons count whatever their axis sign; a stick counts when pushed far enough the bound way
		if (b.type == InputBinding::Type::PadAxis ? v >= kPressPoint : v != 0.0f) return true;
	}
	return false;
}

bool InputActions::IsTriggered(const std::string& action, const Source& src) const
{
	auto it = wasDown_.find(action);
	return IsDown(action, src) && !(it != wasDown_.end() && it->second);
}

bool InputActions::IsReleased(const std::string& action, const Source& src) const
{
	auto it = wasDown_.find(action);
	return !IsDown(action, src) && it != wasDown_.end() && it->second;
}

float InputActions::GetAxis(const std::string& action, const Source& src) const
{
	const InputAction* a = Find(action);
	if (!a) return 0.0f;
	float sum = 0.0f;
	for (const InputBinding& b : a->bindings) sum += BindingValue(b, src);
	return std::clamp(sum, -1.0f, 1.0f);
}

void InputActions::EndFrame(const Source& src)
{
	for (const InputAction& a : actions_) wasDown_[a.name] = IsDown(a.name, src);
}

InputAction* InputActions::Find(const std::string& name)
{
	for (InputAction& a : actions_) if (a.name == name) return &a;
	return nullptr;
}

const InputAction* InputActions::Find(const std::string& name) const
{
	for (const InputAction& a : actions_) if (a.name == name) return &a;
	return nullptr;
}

InputAction& InputActions::Add(const std::string& name)
{
	if (InputAction* existing = Find(name)) return *existing;
	actions_.push_back({ name, {} });
	return actions_.back();
}

bool InputActions::Remove(const std::string& name)
{
	const size_t before = actions_.size();
	std::erase_if(actions_, [&](const InputAction& a) { return a.name == name; });
	wasDown_.erase(name);
	return actions_.size() != before;
}

std::string InputActions::ToJson() const
{
	using SceneJson::Json;
	Json root = Json::object();
	root["version"] = 1;
	Json list = Json::array();
	for (const InputAction& a : actions_) {
		Json bindings = Json::array();
		for (const InputBinding& b : a.bindings) {
			bindings.push_back({ { "type", static_cast<int>(b.type) }, { "code", b.code }, { "sign", SceneJson::Num(b.sign) } });
		}
		list.push_back({ { "name", a.name }, { "bindings", std::move(bindings) } });
	}
	root["actions"] = std::move(list);
	return SceneJson::DumpCompact(root);
}

std::optional<InputActions> InputActions::FromJson(const std::string& text, std::string* outError)
{
	auto parsed = SceneJson::Parse(text, outError);
	if (!parsed || !parsed->is_object()) {
		if (outError && outError->empty()) *outError = "not a JSON object";
		return std::nullopt;
	}
	InputActions result;
	const SceneJson::Json* list = SceneJson::Find(*parsed, "actions");
	if (!list || !list->is_array()) {
		if (outError) *outError = "missing \"actions\" array";
		return std::nullopt;
	}
	for (const auto& ja : *list) {
		std::string name;
		if (!SceneJson::Read(ja, "name", name) || name.empty()) continue;
		InputAction& action = result.Add(name);
		if (const SceneJson::Json* bindings = SceneJson::Find(ja, "bindings"); bindings && bindings->is_array()) {
			for (const auto& jb : *bindings) {
				int type = 0;
				InputBinding b;
				SceneJson::Read(jb, "type", type);
				SceneJson::Read(jb, "code", b.code);
				SceneJson::Read(jb, "sign", b.sign);
				if (type < 0 || type > 3 || !std::isfinite(b.sign)) continue;
				b.type = static_cast<InputBinding::Type>(type);
				action.bindings.push_back(b);
			}
		}
	}
	return result;
}

std::string InputActions::BindingLabel(const InputBinding& b)
{
	const char* sign = (b.sign < 0.0f) ? " (-)" : "";
	switch (b.type) {
	case InputBinding::Type::Key: {
		// DIK codes are scan codes; codes >= 0x80 are the extended (E0-prefixed) keys such as the arrows
		const LONG lParam = ((b.code & 0x7F) << 16) | ((b.code & 0x80) ? (1 << 24) : 0);
		wchar_t name[64]{};
		if (GetKeyNameTextW(lParam, name, 64) > 0) {
			char utf8[128]{};
			WideCharToMultiByte(CP_UTF8, 0, name, -1, utf8, sizeof(utf8), nullptr, nullptr);
			return std::string(utf8) + sign;
		}
		return std::format("Key 0x{:02X}{}", b.code, sign);
	}
	case InputBinding::Type::Mouse: return std::format("Mouse {}{}", b.code, sign);
	case InputBinding::Type::PadButton: {
		static const std::pair<int, const char*> kNames[] = {
			{ PAD_A, "Pad A" }, { PAD_B, "Pad B" }, { PAD_X, "Pad X" }, { PAD_Y, "Pad Y" }, { PAD_LB, "Pad LB" }, { PAD_RB, "Pad RB" },
			{ PAD_START, "Pad Start" }, { PAD_BACK, "Pad Back" }, { PAD_DPAD_UP, "Pad Up" }, { PAD_DPAD_DOWN, "Pad Down" },
			{ PAD_DPAD_LEFT, "Pad Left" }, { PAD_DPAD_RIGHT, "Pad Right" }, { PAD_LEFT_THUMB, "Pad L3" }, { PAD_RIGHT_THUMB, "Pad R3" } };
		for (const auto& [bits, label] : kNames) if (bits == b.code) return std::string(label) + sign;
		return std::format("Pad 0x{:04X}{}", b.code, sign);
	}
	case InputBinding::Type::PadAxis: {
		static const char* kAxes[] = { "Left Stick X", "Left Stick Y", "Right Stick X", "Right Stick Y", "Left Trigger", "Right Trigger" };
		const char* axis = (b.code >= 0 && b.code < 6) ? kAxes[b.code] : "Axis ?";
		return std::string(axis) + (b.sign < 0.0f ? " (-)" : "");
	}
	}
	return "?";
}
