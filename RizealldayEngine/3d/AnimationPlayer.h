#pragma once
#include "3d/Model.h"
#include <vector>

/// Plays skeletal animation clips with crossfade (roadmap 5.1).
/// A clip may come from any model with the same bone names (`source`); nullptr = the model being animated.
/// Advance() moves time (call it from the game update with scaled game time), Evaluate() builds the skin matrices.
class AnimationPlayer {
public:
	struct Track {
		const Model* source = nullptr; // owner of the clip (nullptr = the target model)
		int clip = -1;                 // clip index in `source`, -1 = bind pose
		float time = 0.0f;             // seconds inside the clip (wrapped when looping)
		float elapsed = 0.0f;          // seconds since the track started (not wrapped, scaled by speed)
		float speed = 1.0f;
		bool loop = true;
	};

	/// Starts `clip`. fadeSeconds > 0 crossfades from the current pose; 0 switches instantly.
	void Play(const Model* source, int clip, bool loop = true, float speed = 1.0f, float fadeSeconds = 0.0f, float startTime = 0.0f);
	/// `target` = the animated model (resolves tracks whose source is nullptr)
	void Advance(float deltaTime, const Model* target);
	void Evaluate(const Model& target, std::vector<Matrix4x4>& outSkinMatrices) const;
	/// The blended local pose (skeleton overlay, tests)
	void EvaluatePose(const Model& target, LocalPose& outPose) const;

	Track& Current() { return current_; }
	const Track& Current() const { return current_; }
	const Track& Previous() const { return previous_; }
	bool IsFading() const { return fadeDuration_ > 0.0f && fadeTime_ < fadeDuration_; }
	/// 0 at the start of a crossfade, 1 when it is done (1 when not fading)
	float FadeProgress() const { return IsFading() ? fadeTime_ / fadeDuration_ : 1.0f; }
	float FadeDuration() const { return fadeDuration_; }

	/// Clip length in seconds of a track (0 for the bind pose); `target` resolves a null source
	static float Duration(const Track& track, const Model* target);
	/// elapsed / duration: 0..1 for the first pass, 2.5 = middle of the third loop (exit times, events)
	float NormalizedTime(const Model* target) const;
	/// Clip time window covered by the last Advance() of the current track (animation events)
	float LastAdvanceStart() const { return lastStart_; }
	float LastAdvanceEnd() const { return current_.elapsed; }

private:
	static void AdvanceTrack(Track& track, float deltaTime, const Model* target);

	Track current_;
	Track previous_;
	float fadeTime_ = 0.0f;
	float fadeDuration_ = 0.0f;
	float lastStart_ = 0.0f;
	mutable const Model* lastTarget_ = nullptr; // last target seen (NormalizedTime without a target)
	mutable LocalPose poseA_;
	mutable LocalPose poseB_;
	mutable LocalPose blended_;
};
