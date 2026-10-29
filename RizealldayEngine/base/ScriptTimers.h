#pragma once
#include <cmath>
#include <functional>
#include <vector>

// Script timers (header only, lives inside GameScripts.dll). Call Tick(deltaTime) from the script's Update:
// deltaTime is game time, so timers pause with Pause / Hitstop and follow SetTimeScale.
// State is not kept across a script hot reload (like any field that is not EXPOSE_FIELD).

/// "Can I fire again?" — usable at start, then blocked for `duration` seconds after each use
struct Cooldown {
	float duration = 1.0f;
	float left = 0.0f;

	Cooldown() = default;
	explicit Cooldown(float seconds) : duration(seconds) {}
	bool Ready() const { return left <= 0.0f; }
	void Tick(float deltaTime) { if (left > 0.0f) left -= deltaTime; }
	/// True (and the cooldown restarts) when ready
	bool TryUse() {
		if (!Ready()) return false;
		left = duration;
		return true;
	}
	void Reset() { left = 0.0f; }
};

/// Delayed and repeating calls:  timers.After(0.5f, [this] { Explode(); });  timers.Every(2.0f, [this] { Spawn(); });
class TimerQueue {
public:
	/// Runs `fn` once after `seconds`. Returns an id for Cancel.
	int After(float seconds, std::function<void()> fn) { return Add(seconds, 0.0f, std::move(fn), false); }
	/// Runs `fn` every `seconds` (a long frame runs it several times to catch up; 0 = once per Tick)
	int Every(float seconds, std::function<void()> fn) { return Add(seconds, seconds, std::move(fn), true); }
	void Cancel(int id) {
		for (Entry& e : entries_) if (e.id == id) e.alive = false;
	}
	void Clear() { entries_.clear(); }
	size_t Count() const {
		size_t n = 0;
		for (const Entry& e : entries_) n += e.alive ? 1 : 0;
		return n;
	}

	void Tick(float deltaTime) {
		// Timers added by a callback start on the next Tick; cancelled ones are removed at the end
		const size_t count = entries_.size();
		for (size_t i = 0; i < count; ++i) {
			if (!entries_[i].alive) continue;
			entries_[i].left -= deltaTime;
			while (entries_[i].alive && entries_[i].left <= 0.0f) {
				std::function<void()> fn = entries_[i].fn; // the callback may add timers (vector may move)
				if (!entries_[i].repeat) entries_[i].alive = false;
				fn();
				if (!entries_[i].alive || !entries_[i].repeat) break;
				if (entries_[i].interval <= 0.0f) { entries_[i].left = 0.0f; break; } // once per Tick
				entries_[i].left += entries_[i].interval;
			}
		}
		std::erase_if(entries_, [](const Entry& e) { return !e.alive; });
	}

private:
	struct Entry {
		int id = 0;
		float left = 0.0f;
		float interval = 0.0f;
		bool repeat = false;
		bool alive = true;
		std::function<void()> fn;
	};

	int Add(float seconds, float interval, std::function<void()> fn, bool repeat) {
		if (!std::isfinite(seconds) || seconds < 0.0f) seconds = 0.0f;
		if (!std::isfinite(interval) || interval < 0.0f) interval = 0.0f;
		entries_.push_back({ ++nextId_, seconds, interval, repeat, true, std::move(fn) });
		return nextId_;
	}

	std::vector<Entry> entries_;
	int nextId_ = 0;
};
