#include "input/Input.h"
#include <cassert>
#include <cmath>

Input* Input::GetInstance()
{
	static Input instance;
	return &instance;
}

void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT hr;
	hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
	assert(SUCCEEDED(hr));

	// Keyboard
	hr = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(hr));
	hr = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(hr));
	hr = keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(hr));

	// Mouse
	hr = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(hr));
	hr = mouse_->SetDataFormat(&c_dfDIMouse);
	assert(SUCCEEDED(hr));
	hr = mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(hr));
}

void Input::Update()
{
	// Save previous state
	memcpy(preKey_, key_, sizeof(key_));
	preMouseState_ = mouseState_;

	// Keyboard
	keyboard_->Acquire();
	keyboard_->GetDeviceState(sizeof(key_), key_);

	// Mouse
	mouse_->Acquire();
	mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);

	// Gamepad (XInput)
	for (int i = 0; i < kMaxGamepads; i++) {
		preGamepadState_[i] = gamepadState_[i];
		DWORD result = XInputGetState(i, &gamepadState_[i]);
		gamepadConnected_[i] = (result == ERROR_SUCCESS);
	}
}

bool Input::PushKey(BYTE keyNumber) const
{
	return (key_[keyNumber] & 0x80) != 0;
}

bool Input::TriggerKey(BYTE keyNumber) const
{
	return (key_[keyNumber] & 0x80) && !(preKey_[keyNumber] & 0x80);
}

bool Input::ReleaseKey(BYTE keyNumber) const
{
	return !(key_[keyNumber] & 0x80) && (preKey_[keyNumber] & 0x80);
}

bool Input::IsGamepadConnected(int padIndex) const
{
	if (padIndex < 0 || padIndex >= kMaxGamepads) return false;
	return gamepadConnected_[padIndex];
}

bool Input::PushGamepadButton(WORD button, int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return false;
	return (gamepadState_[padIndex].Gamepad.wButtons & button) != 0;
}

bool Input::TriggerGamepadButton(WORD button, int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return false;
	return (gamepadState_[padIndex].Gamepad.wButtons & button) &&
		   !(preGamepadState_[padIndex].Gamepad.wButtons & button);
}

float Input::GetLeftStickX(int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return 0.0f;
	float val = static_cast<float>(gamepadState_[padIndex].Gamepad.sThumbLX) / 32767.0f;
	return (std::fabsf(val) < deadZone_) ? 0.0f : val;
}

float Input::GetLeftStickY(int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return 0.0f;
	float val = static_cast<float>(gamepadState_[padIndex].Gamepad.sThumbLY) / 32767.0f;
	return (std::fabsf(val) < deadZone_) ? 0.0f : val;
}

float Input::GetRightStickX(int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return 0.0f;
	float val = static_cast<float>(gamepadState_[padIndex].Gamepad.sThumbRX) / 32767.0f;
	return (std::fabsf(val) < deadZone_) ? 0.0f : val;
}

float Input::GetRightStickY(int padIndex) const
{
	if (!gamepadConnected_[padIndex]) return 0.0f;
	float val = static_cast<float>(gamepadState_[padIndex].Gamepad.sThumbRY) / 32767.0f;
	return (std::fabsf(val) < deadZone_) ? 0.0f : val;
}

void Input::SetVibration(int padIndex, float leftMotor, float rightMotor)
{
	XINPUT_VIBRATION vibration{};
	vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
	vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
	XInputSetState(padIndex, &vibration);
}