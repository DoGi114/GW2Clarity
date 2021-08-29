#pragma once

#include <Main.h>
#include <Singleton.h>
#include <set>
#include <list>
#include <functional>
#include <algorithm>
#include <optional>
#include <ConfigurationOption.h>
#include <ScanCode.h>
#include <Utility.h>
#include <ActivationKeybind.h>

namespace GW2Radial
{

enum class InputResponse : uint
{
	PASS_TO_GAME = 0, // Do not prevent any input from reaching the game
	PREVENT_MOUSE = 1, // Prevent mouse movement only from reaching the game
	PREVENT_KEYBOARD = 2, // Prevent keyboard inputs only from reaching the game
	PREVENT_ALL = 3 // Prevent all input from reaching the game
};

enum class KeybindAction : uint {
	NONE = 0,
	DOWN = 1,
	UP = 2,
	BOTH = 3
};

struct EventKey
{
    ScanCode sc : 31;
	bool down : 1;
};

struct Point
{
	int x;
	int y;
};

class ActivationKeybind;

template<typename Func>
struct Callback {
	Func callback;
	int priority = 0;

	inline bool operator<(const Callback& other) const {
		if (priority != other.priority)
			return priority < other.priority;
		else
			return &callback < &other.callback;
	}

	Callback(Func cb, int p = 0) :
		callback(std::move(cb)), priority(p) { }
};

class Input : public Singleton<Input>
{
public:
	using MouseMoveCallback = Callback<std::function<void(bool& retval)>>;

	uint id_H_LBUTTONDOWN() const { return id_H_LBUTTONDOWN_; }
	uint id_H_LBUTTONUP() const { return id_H_LBUTTONUP_; }
	uint id_H_RBUTTONDOWN() const { return id_H_RBUTTONDOWN_; }
	uint id_H_RBUTTONUP() const { return id_H_RBUTTONUP_; }
	uint id_H_MBUTTONDOWN() const { return id_H_MBUTTONDOWN_; }
	uint id_H_MBUTTONUP() const { return id_H_MBUTTONUP_; }
	uint id_H_SYSKEYDOWN() const { return id_H_SYSKEYDOWN_; }
	uint id_H_SYSKEYUP() const { return id_H_SYSKEYUP_; }
	uint id_H_KEYDOWN() const { return id_H_KEYDOWN_; }
	uint id_H_KEYUP() const { return id_H_KEYUP_; }

	// Returns true to consume message
	bool OnInput(UINT& msg, WPARAM& wParam, LPARAM& lParam);
	void OnFocusLost();
	void OnUpdate();
	
	void AddMouseMoveCallback(MouseMoveCallback* cb) { mouseMoveCallbacks_.insert(cb); }
	void RemoveMouseMoveCallback(MouseMoveCallback* cb) { mouseMoveCallbacks_.erase(cb); }
	void SendKeybind(const KeyCombo& ks, std::optional<Point> const& cursorPos = std::nullopt, KeybindAction action = KeybindAction::BOTH);

protected:
	struct DelayedInput
	{
		uint msg;
		WPARAM wParam;
        union {
            KeyLParam lParamKey;
            LPARAM lParamValue;
        };

		mstime t;
		std::optional<Point> cursorPos;
	};

	ActivationKeybind::PreventPassToGame TriggerKeybinds(bool downKeysChanged);
	uint ConvertHookedMessage(uint msg) const;
	DelayedInput TransformScanCode(ScanCode sc, bool down, mstime t, const std::optional<Point>& cursorPos);
	std::tuple<WPARAM, LPARAM> CreateMouseEventParams(const std::optional<Point>& cursorPos) const;
	void SendQueuedInputs();

	// ReSharper disable CppInconsistentNaming
	uint id_H_LBUTTONDOWN_;
	uint id_H_LBUTTONUP_;
	uint id_H_RBUTTONDOWN_;
	uint id_H_RBUTTONUP_;
	uint id_H_MBUTTONDOWN_;
	uint id_H_MBUTTONUP_;
	uint id_H_XBUTTONDOWN_;
	uint id_H_XBUTTONUP_;
	uint id_H_SYSKEYDOWN_;
	uint id_H_SYSKEYUP_;
	uint id_H_KEYDOWN_;
	uint id_H_KEYUP_;
	uint id_H_MOUSEMOVE_;
	// ReSharper restore CppInconsistentNaming

	std::set<ScanCode> downKeys_;
	std::list<DelayedInput> queuedInputs_;
	
	std::set<MouseMoveCallback*, PtrComparator<MouseMoveCallback>> mouseMoveCallbacks_;
	std::vector<ActivationKeybind*> keybinds_;
	ActivationKeybind* activeKeybind_ = nullptr;
	void RegisterKeybind(ActivationKeybind* kb);
	void UnregisterKeybind(ActivationKeybind* kb);

	friend class MiscTab;
	friend class ActivationKeybind;
};

}

inline GW2Radial::InputResponse operator|(GW2Radial::InputResponse a, GW2Radial::InputResponse b) {
	return GW2Radial::InputResponse(uint(a) | uint(b));
}

inline GW2Radial::InputResponse& operator|=(GW2Radial::InputResponse& a, GW2Radial::InputResponse b) {
	a = a | b;
	return a;
}

inline GW2Radial::KeybindAction operator&(GW2Radial::KeybindAction a, GW2Radial::KeybindAction b) {
	return GW2Radial::KeybindAction(uint(a) & uint(b));
}