#include "EditorSuiteInternal.h"

void EditorSuite::EnsureParticleQuadInitialized(GameObject& obj) {
	if (!obj.hasParticleSystem) return;
	if (!obj.particleQuad) {
		obj.particleQuad = std::make_shared<ParticleQuad>();
		obj.particleQuad->Initialize(dxCommon_, &dxc_, particleTextureHandle_, static_cast<uint32_t>(obj.particleMaxParticles));
	}
	SyncGameObjectToParticleQuad(obj);
}

void EditorSuite::SyncGameObjectToParticleQuad(GameObject& obj) {
	if (!obj.hasParticleSystem || !obj.particleQuad) return;
	auto& p = *obj.particleQuad;
	p.emitter.shape = obj.particleShape;
	p.emitter.spawnRadius = obj.particleSpawnRadius;
	p.emitter.boxExtents = obj.particleBoxExtents;
	p.emitter.coneAngle = obj.particleConeAngle;
	p.emitter.spawnRate = obj.particleSpawnRate;
	p.SetMaxParticles(static_cast<uint32_t>(obj.particleMaxParticles));
	p.emitter.direction = obj.particleDirection;
	p.emitter.spreadAngle = obj.particleSpreadAngle;
	p.emitter.speedMin = obj.particleSpeedMin;
	p.emitter.speedMax = obj.particleSpeedMax;
	p.emitter.gravity = obj.particleGravity;
	p.emitter.drag = obj.particleDrag;
	p.windField.enabled = obj.particleWindEnabled;
	p.windField.acceleration = obj.particleWindAcceleration;
	p.emitter.lifetimeMin = obj.particleLifetimeMin;
	p.emitter.lifetimeMax = obj.particleLifetimeMax;
	p.emitter.startScale = obj.particleStartScale;
	p.emitter.endScale = obj.particleEndScale;
	p.emitter.startColor = obj.particleStartColor;
	p.emitter.endColor = obj.particleEndColor;
	p.blendType = (obj.particleBlendMode == 1) ? ParticleQuad::BlendType::Add : ParticleQuad::BlendType::Normal;
	p.emitter.billboardMode = obj.particleBillboardMode;
	p.emitter.emit = obj.particleEmitContinuously;
	p.emitter.translate = obj.translation;
}

void EditorSuite::ApplyParticlePreset(GameObject& obj, int presetIndex) {
	if (presetIndex == 1) { // Smoke
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.3f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 20.0f;
		obj.particleSpeedMin = 0.5f;
		obj.particleSpeedMax = 1.5f;
		obj.particleGravity = 0.3f; // upward drift
		obj.particleDrag = 0.05f;
		obj.particleLifetimeMin = 2.0f;
		obj.particleLifetimeMax = 3.5f;
		obj.particleStartScale = 0.2f;
		obj.particleEndScale = 0.8f; // scale grows over lifetime
		obj.particleStartColor = { 0.8f, 0.8f, 0.8f, 0.6f };
		obj.particleEndColor = { 0.4f, 0.4f, 0.4f, 0.0f }; // alpha fades out
		obj.particleBlendMode = 0; // Normal Alpha
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 15.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 2) { // Water Fountain
		obj.particleShape = 3; // Cone
		obj.particleConeAngle = 15.0f;
		obj.particleSpawnRadius = 0.2f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 15.0f; // upward cone with spread
		obj.particleSpeedMin = 5.0f;
		obj.particleSpeedMax = 8.0f;
		obj.particleGravity = -9.8f; // gravity Y = -9.8
		obj.particleDrag = 0.02f;
		obj.particleLifetimeMin = 1.5f;
		obj.particleLifetimeMax = 2.2f;
		obj.particleStartScale = 0.2f;
		obj.particleEndScale = 0.15f;
		obj.particleStartColor = { 0.3f, 0.8f, 1.0f, 0.85f };
		obj.particleEndColor = { 0.85f, 0.95f, 1.0f, 0.1f };
		obj.particleBlendMode = 0; // Normal Alpha
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 25.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 3) { // Fireworks
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.1f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 180.0f; // 360-degree spherical velocity
		obj.particleSpeedMin = 4.0f;
		obj.particleSpeedMax = 9.0f;
		obj.particleGravity = -2.0f;
		obj.particleDrag = 1.2f; // drag deceleration
		obj.particleLifetimeMin = 1.5f;
		obj.particleLifetimeMax = 2.5f;
		obj.particleStartScale = 0.25f;
		obj.particleEndScale = 0.05f;
		obj.particleStartColor = { 1.0f, 0.9f, 0.2f, 1.0f };
		obj.particleEndColor = { 1.0f, 0.1f, 0.5f, 0.0f };
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 2; // Velocity Aligned
		obj.particleSpawnRate = 30.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 4) { // Explosion
		obj.particleShape = 1; // Sphere
		obj.particleSpawnRadius = 0.4f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 180.0f; // fast radial burst
		obj.particleSpeedMin = 6.0f;
		obj.particleSpeedMax = 14.0f;
		obj.particleGravity = 0.0f;
		obj.particleDrag = 2.5f;
		obj.particleLifetimeMin = 0.8f;
		obj.particleLifetimeMax = 1.5f;
		obj.particleStartScale = 0.3f;
		obj.particleEndScale = 1.8f; // rapid expansion
		obj.particleStartColor = { 1.0f, 0.7f, 0.1f, 1.0f };
		obj.particleEndColor = { 0.2f, 0.2f, 0.2f, 0.0f }; // orange/yellow/red turning to dark gray
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 40.0f;
		obj.particleWindEnabled = false;
	} else if (presetIndex == 5) { // Torch / Campfire
		obj.particleShape = 3; // Cone
		obj.particleConeAngle = 20.0f;
		obj.particleSpawnRadius = 0.15f;
		obj.particleDirection = { 0.0f, 1.0f, 0.0f };
		obj.particleSpreadAngle = 15.0f;
		obj.particleSpeedMin = 1.2f;
		obj.particleSpeedMax = 2.8f;
		obj.particleGravity = 0.6f; // continuous upward flame
		obj.particleDrag = 0.05f;
		obj.particleLifetimeMin = 0.8f;
		obj.particleLifetimeMax = 1.6f;
		obj.particleStartScale = 0.35f;
		obj.particleEndScale = 0.08f; // shrinking scale
		obj.particleStartColor = { 1.0f, 0.95f, 0.2f, 1.0f };
		obj.particleEndColor = { 0.9f, 0.15f, 0.0f, 0.0f }; // yellow -> orange -> red
		obj.particleBlendMode = 1; // Additive Glow
		obj.particleBillboardMode = 0; // Camera Facing
		obj.particleSpawnRate = 20.0f;
		obj.particleWindEnabled = false;
	}
	EnsureParticleQuadInitialized(obj);
	if (obj.particleQuad) {
		obj.particleQuad->ClearParticles();
		obj.particleQuad->EmitBurst(15);
	}
}

