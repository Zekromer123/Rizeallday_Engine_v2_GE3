#include "3d/AnimationPlayer.h"
#include <algorithm>
#include <cmath>

void AnimationPlayer::Play(const Model* source, int clip, bool loop, float speed, float fadeSeconds, float startTime)
{
	// A crossfade starting while another one runs fades from the current track (the older one is dropped)
	if (fadeSeconds > 0.0f && current_.clip >= 0) {
		previous_ = current_;
		fadeTime_ = 0.0f;
		fadeDuration_ = fadeSeconds;
	} else {
		previous_ = Track{};
		fadeTime_ = 0.0f;
		fadeDuration_ = 0.0f;
	}
	current_ = Track{};
	current_.source = source;
	current_.clip = clip;
	current_.loop = loop;
	current_.speed = speed;
	current_.time = startTime;
	current_.elapsed = startTime;
	lastStart_ = startTime;
}

float AnimationPlayer::Duration(const Track& track, const Model* target)
{
	const Model* model = track.source ? track.source : target;
	return (model && track.clip >= 0) ? model->GetAnimationDuration(track.clip) : 0.0f;
}

void AnimationPlayer::AdvanceTrack(Track& track, float deltaTime, const Model* target)
{
	const float step = deltaTime * track.speed;
	track.elapsed += step;
	track.time += step;
	const float duration = Duration(track, target);
	if (duration <= 0.0f) return;
	if (track.loop) {
		track.time = std::fmod(track.time, duration);
		if (track.time < 0.0f) track.time += duration;
	} else {
		track.time = std::clamp(track.time, 0.0f, duration);
		track.elapsed = (std::min)(track.elapsed, duration); // a finished one-shot stays at 1.0 normalized
	}
}

void AnimationPlayer::Advance(float deltaTime, const Model* target)
{
	if (target) lastTarget_ = target;
	lastStart_ = current_.elapsed;
	AdvanceTrack(current_, deltaTime, lastTarget_);
	if (IsFading()) {
		AdvanceTrack(previous_, deltaTime, lastTarget_);
		fadeTime_ += deltaTime;
		if (fadeTime_ >= fadeDuration_) {
			previous_ = Track{};
			fadeDuration_ = 0.0f;
			fadeTime_ = 0.0f;
		}
	}
}

float AnimationPlayer::NormalizedTime(const Model* target) const
{
	const float duration = Duration(current_, target ? target : lastTarget_);
	return (duration > 0.0f) ? current_.elapsed / duration : 0.0f;
}

void AnimationPlayer::EvaluatePose(const Model& target, LocalPose& outPose) const
{
	lastTarget_ = &target;
	auto clipOf = [&](const Track& t) {
		const Model* model = t.source ? t.source : &target;
		return model->GetAnimationClip(t.clip);
	};
	if (!IsFading()) {
		target.SampleLocalPose(clipOf(current_), current_.time, current_.loop, outPose);
		return;
	}
	target.SampleLocalPose(clipOf(previous_), previous_.time, previous_.loop, poseA_);
	target.SampleLocalPose(clipOf(current_), current_.time, current_.loop, poseB_);
	target.BlendPoses(poseA_, poseB_, FadeProgress(), outPose);
}

void AnimationPlayer::Evaluate(const Model& target, std::vector<Matrix4x4>& outSkinMatrices) const
{
	EvaluatePose(target, blended_);
	target.BuildSkinMatrices(blended_, outSkinMatrices);
}
