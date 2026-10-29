#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Input actions: named actions ("Jump", "MoveX") bound to keys, mouse buttons, gamepad buttons and stick axes.
// One script line works with keyboard and gamepad:  if (IsActionTriggered("Jump")) ...;  float x = GetAxis("MoveX");
// The map lives in <project>/ProjectSettings/input.json (shared through git); without the file the defaults apply.

struct InputBinding {
	enum class Type : int { Key = 0, Mouse = 1, PadButton = 2, PadAxis = 3 };
	enum class Axis : int { LeftX = 0, LeftY = 1, RightX = 2, RightY = 3, LeftTrigger = 4, RightTrigger = 5 };
	Type type = Type::Key;
	int code = 0;      // DIK key code / mouse button / XInput button bits / Axis
	float sign = 1.0f; // contribution to GetAxis: keys and buttons give `sign`, axes give value * sign

	static InputBinding Key(int dik, float sign = 1.0f) { return { Type::Key, dik, sign }; }
	static InputBinding Mouse(int button) { return { Type::Mouse, button, 1.0f }; }
	static InputBinding Pad(int buttonBits, float sign = 1.0f) { return { Type::PadButton, buttonBits, sign }; }
	static InputBinding PadAxis(Axis axis, float sign = 1.0f) { return { Type::PadAxis, static_cast<int>(axis), sign }; }
	bool operator==(const InputBinding&) const = default;
};

struct InputAction {
	std::string name;
	std::vector<InputBinding> bindings;
};

class InputActions {
public:
	static constexpr float kDeadZone = 0.2f;   // stick values below this count as 0
	static constexpr float kPressPoint = 0.5f; // an axis binding counts as "pressed" beyond this

	/// Raw device queries (the editor passes the real Input; tests pass fakes). Missing functions read as "not pressed".
	struct Source {
		std::function<bool(int)> keyDown;
		std::function<bool(int)> mouseDown;
		std::function<bool(int)> padDown;
		std::function<float(int)> padAxis; // InputBinding::Axis
	};

	/// MoveX, MoveY, Jump, Attack, Submit, Cancel, Pause (keyboard + mouse + gamepad)
	static InputActions Defaults();

	bool IsDown(const std::string& action, const Source& src) const;
	/// Down now but not at the previous EndFrame
	bool IsTriggered(const std::string& action, const Source& src) const;
	bool IsReleased(const std::string& action, const Source& src) const;
	/// -1..1: sum of all bindings, clamped
	float GetAxis(const std::string& action, const Source& src) const;
	/// Remembers this frame's state for IsTriggered / IsReleased (call once per frame, after scripts)
	void EndFrame(const Source& src);

	InputAction* Find(const std::string& name);
	const InputAction* Find(const std::string& name) const;
	InputAction& Add(const std::string& name); // returns the existing one when the name is taken
	bool Remove(const std::string& name);
	std::vector<InputAction>& All() { return actions_; }
	const std::vector<InputAction>& All() const { return actions_; }

	std::string ToJson() const;
	static std::optional<InputActions> FromJson(const std::string& text, std::string* outError);
	/// "Space", "Mouse 0", "Pad A", "Left Stick X+" (editor UI)
	static std::string BindingLabel(const InputBinding& binding);

private:
	static float BindingValue(const InputBinding& b, const Source& src);
	std::vector<InputAction> actions_;
	std::unordered_map<std::string, bool> wasDown_;
};
