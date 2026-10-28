#include "EditorSuiteInternal.h"

void EditorSuite::DrawInspectorAndDetailsPanel(bool& showParticles)
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	float screenW = io.DisplaySize.x;
	float screenH = io.DisplaySize.y;
	float topY = ImGui::GetFrameHeight() + kTopBarHeight;
	float rightW = screenW * 0.24f;
	if (rightW < 300.0f) rightW = 300.0f;
	float midH = (screenH - topY) * 0.65f;

	ImGui::SetNextWindowPos(ImVec2(screenW - rightW, topY));
	ImGui::SetNextWindowSize(ImVec2(rightW, midH));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin("Inspector", nullptr, flags)) {
		if (ImGui::BeginTabBar("InspectorTabs")) {
			// TAB 1: INSPECTOR
			if (ImGui::BeginTabItem("Inspector")) {
				rightPanelTab_ = 0;
				if (selectedObjectIndex_ < 0 || selectedObjectIndex_ >= static_cast<int>(gameObjects_.size())) {
					ImGui::TextDisabled("No GameObject Selected");
					ImGui::Separator();
					DrawSkyboxSettingsUI();
					ImGui::EndTabItem();
					ImGui::EndTabBar();
					ImGui::End();
					return;
				}

				auto& obj = gameObjects_[selectedObjectIndex_];

				// Widgets take ~60% of the width so right-hand labels (e.g. "Geometry Shader Effect") are not clipped
				const float inspectorLabelW = (std::max)(120.0f, ImGui::GetContentRegionAvail().x * 0.40f);
				ImGui::PushItemWidth(-inspectorLabelW);

				// Header: Active Checkbox + Object Name + Static
				if (ImGui::Checkbox("##Active", &obj.isActive)) {
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}
				ImGui::SameLine();
				char nameBuf[128]{};
				strncpy_s(nameBuf, obj.name.c_str(), sizeof(nameBuf) - 1);
				const float staticCheckboxW = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize("Static").x;
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - staticCheckboxW - ImGui::GetStyle().ItemSpacing.x);
				if (ImGui::InputText("##ObjName", nameBuf, sizeof(nameBuf))) {
					obj.name = nameBuf;
					isSceneDirty_ = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					PushUndoSnapshot();
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("Static", &obj.isStatic)) {
					isSceneDirty_ = true;
					PushUndoSnapshot();
				}

				// Tag + collision layer / mask (layer names match ScriptLayer in ScriptAPI.h)
				{
					char tagBuf[64]{};
					strncpy_s(tagBuf, obj.tag.c_str(), sizeof(tagBuf) - 1);
					if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) {
						obj.tag = tagBuf;
						isSceneDirty_ = true;
					}
					if (ImGui::IsItemDeactivatedAfterEdit()) {
						PushUndoSnapshot();
					}
					ImGui::SetItemTooltip("Free-form tag scripts can compare (e.g. \"Enemy\")");

					const uint32_t layer = obj.layer & 31u;
					if (ImGui::BeginCombo("Layer", GetCollisionLayerName(layer).c_str())) {
						for (uint32_t i = 0; i < 32; ++i) {
							if (ImGui::Selectable(GetCollisionLayerName(i).c_str(), i == layer)) {
								obj.layer = i;
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SetItemTooltip("Collision layer of this object's colliders");

					std::string maskPreview;
					if (obj.collisionMask == 0xFFFFFFFFu) {
						maskPreview = "Everything";
					} else if (obj.collisionMask == 0u) {
						maskPreview = "Nothing";
					} else {
						for (uint32_t i = 0; i < 32; ++i) {
							if (obj.collisionMask & (1u << i)) {
								if (!maskPreview.empty()) maskPreview += ", ";
								maskPreview += GetCollisionLayerName(i);
							}
						}
					}
					if (ImGui::BeginCombo("Collides With", maskPreview.c_str())) {
						if (ImGui::Selectable("Everything")) { obj.collisionMask = 0xFFFFFFFFu; isSceneDirty_ = true; PushUndoSnapshot(); }
						if (ImGui::Selectable("Nothing")) { obj.collisionMask = 0u; isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::Separator();
						for (uint32_t i = 0; i < 32; ++i) {
							bool on = (obj.collisionMask & (1u << i)) != 0;
							if (ImGui::Checkbox(GetCollisionLayerName(i).c_str(), &on)) {
								obj.collisionMask = on ? (obj.collisionMask | (1u << i)) : (obj.collisionMask & ~(1u << i));
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::SetItemTooltip("Layers this object collides with. A pair collides only if both masks accept each other's layer.");
				}

				// 1. TRANSFORM COMPONENT (Always Present)
				if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
					DrawVec3Control("Position", obj.translation, 0.0f);
					ImGui::SetItemTooltip("World position (X, Y, Z)");

					if (DrawVec3Control("Rotation", obj.rotation, 0.0f)) {
						obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
					}
					ImGui::SetItemTooltip("Euler rotation in radians (X, Y, Z)");

					DrawVec3Control("Scale", obj.scale, 1.0f);
					ImGui::SetItemTooltip("Local scale vector (X, Y, Z)");

					// Debug-only readouts stay out of the way unless expanded
					if (ImGui::TreeNodeEx("Debug##TransformDebug", ImGuiTreeNodeFlags_SpanAvailWidth)) {
						float qVal[4] = { obj.rotationQuat.x, obj.rotationQuat.y, obj.rotationQuat.z, obj.rotationQuat.w };
						ImGui::InputFloat4("Quaternion", qVal, "%.4f", ImGuiInputTextFlags_ReadOnly);
						ImGui::SetItemTooltip("Live unit quaternion (X, Y, Z, W)");

						if (ImGui::Button("Test 90-deg Roll Slerp", ImVec2(-1.0f, 0.0f))) {
							Quaternion target = Quaternion::FromAxisAngle({ 0, 0, 1 }, 3.14159265f / 2.0f);
							obj.rotationQuat = Quaternion::Slerp(obj.rotationQuat, target, 0.5f);
							obj.rotation = obj.rotationQuat.ToEuler();
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Interpolate orientation using spherical linear interpolation (Slerp)");
						ImGui::TreePop();
					}
				}

				// 2. MESH FILTER & MESH RENDERER
				if (obj.hasMesh) {
					if (DrawComponentHeader("Mesh Filter & Renderer", ComponentKind::Mesh)) {
						if (ImGui::Checkbox("Enabled##Mesh", &obj.meshEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						const char* meshTypes[] = { "Cube", "Sphere", "Plane", "Quad", "CustomModel" };
						int currentMesh = static_cast<int>(obj.meshType);
						if (ImGui::Combo("Mesh Prototype", &currentMesh, meshTypes, 5)) {
							obj.meshType = static_cast<GameObject::MeshType>(currentMesh);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						if (obj.meshType == GameObject::MeshType::CustomModel) {
							char modelBuf[256] = {};
							strncpy_s(modelBuf, obj.modelPath.c_str(), sizeof(modelBuf) - 1);
							if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf))) {
								obj.modelPath = modelBuf;
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						}

						if (ImGui::ColorEdit4("Material Color", &obj.materialColor.x)) { isSceneDirty_ = true; }
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

						if (ImGui::Checkbox("Lit Material", &obj.isLit)) { isSceneDirty_ = true; PushUndoSnapshot(); }

						const char* reflectionModels[] = { "0: Unlit", "1: Lambert", "2: Half-Lambert", "3: Phong", "4: Blinn-Phong" };
						if (ImGui::Combo("Reflection Model", &obj.lightingType, reflectionModels, 5)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Select illumination shading equation (02_00 Phong, 02_01 Blinn-Phong, Half-Lambert, or Unlit)");

						if (obj.lightingType >= 3) {
							if (ImGui::SliderFloat("Shininess", &obj.shininess, 1.0f, 256.0f, "%.1f")) { isSceneDirty_ = true; }
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Specular exponent power factor");

							if (ImGui::ColorEdit3("Specular Color", &obj.specularColor.x)) { isSceneDirty_ = true; }
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						}

						if (ImGui::Checkbox("Cast Shadows##Mesh", &obj.castShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Render this mesh into the directional light shadow map");
						ImGui::SameLine();
						if (ImGui::Checkbox("Receive Shadows##Mesh", &obj.receiveShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Receive shadows cast by other geometry (Unlit objects never receive shadows)");

						const char* gsEffects[] = { "None", "Standard Pass-Through", "Explode Faces Along Normals" };
						if (ImGui::Combo("Geometry Shader Effect", &obj.geometryShaderEffect, gsEffects, 3)) {
							obj.useGeometryShader = (obj.geometryShaderEffect != 0);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("DirectX 12 SM 6.0 Geometry Shader pipeline variants");

						if (obj.geometryShaderEffect == 2) {
							if (ImGui::SliderFloat("Explode Amount", &obj.explodeAmount, 0.0f, 2.0f, "%.2f")) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Displacement distance along polygon face normals in the geometry shader");
						}

						ImGui::Separator();
						ImGui::Text("Textured Surface / UV Controls:");

						char texBuf[256];
						strncpy_s(texBuf, obj.texturePath.c_str(), sizeof(texBuf));
						if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf))) {
							obj.texturePath = texBuf;
							obj.textureHandle = TextureManager::GetInstance()->LoadTexture(ResolveAssetPath(obj.texturePath));
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Path to texture file (e.g. Resources/circle.png)");

						const char* texPresets[] = { "(Choose Preset...)", "Resources/circle.png", "Resources/Model/white.png", "Resources/uvChecker.png" };
						int selectedPreset = 0;
						if (ImGui::Combo("Preset Selector", &selectedPreset, texPresets, 4)) {
							if (selectedPreset == 1) {
								obj.texturePath = "Resources/circle.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(ResolveAssetPath(obj.texturePath));
								isSceneDirty_ = true;
								PushUndoSnapshot();
							} else if (selectedPreset == 2) {
								obj.texturePath = "Resources/Model/white.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(ResolveAssetPath(obj.texturePath));
								isSceneDirty_ = true;
								PushUndoSnapshot();
							} else if (selectedPreset == 3) {
								obj.texturePath = "Resources/uvChecker.png";
								obj.textureHandle = TextureManager::GetInstance()->LoadTexture(ResolveAssetPath(obj.texturePath));
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}

						if (ImGui::ColorEdit4("Tint Color", &obj.materialColor.x)) { isSceneDirty_ = true; }
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Color multiplier tint applied to the texture (RGBA)");

						if (ImGui::DragFloat2("UV Offset XY", &obj.uvOffset.x, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV texture coordinate offset (U, V)");

						if (ImGui::DragFloat2("UV Tiling XY", &obj.uvTiling.x, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV texture coordinate scaling / repeat (U, V)");

						if (ImGui::DragFloat("UV Rotation", &obj.uvRotation, 0.01f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("UV coordinate rotation angle in radians");
					}

					// ANIMATOR / SKELETAL ANIMATION SECTION
					Model* activeModel = obj.editObject3d ? obj.editObject3d->GetModel() : (obj.gameObject3d ? obj.gameObject3d->GetModel() : nullptr);
					if (activeModel && activeModel->GetAnimationCount() > 0) {
						if (ImGui::CollapsingHeader("Animator (FBX / glTF Animation)", ImGuiTreeNodeFlags_DefaultOpen)) {
							uint32_t animCount = static_cast<uint32_t>(activeModel->GetAnimationCount());
							std::vector<std::string> clipNames;
							std::vector<const char*> clipNamePtrs;
							for (uint32_t a = 0; a < animCount; ++a) {
								clipNames.push_back(std::format("[{}] {}", a, activeModel->GetAnimationName(static_cast<int>(a))));
							}
							for (const auto& cn : clipNames) {
								clipNamePtrs.push_back(cn.c_str());
							}

							int curClip = std::clamp(obj.animationIndex, 0, static_cast<int>(animCount) - 1);
							if (ImGui::Combo("Animation Clip", &curClip, clipNamePtrs.data(), static_cast<int>(clipNamePtrs.size()))) {
								obj.animationIndex = curClip;
								if (obj.editObject3d) {
									obj.editObject3d->SetCurrentAnimationIndex(static_cast<uint32_t>(curClip));
									obj.editObject3d->ResetAnimationTime();
								}
								if (obj.gameObject3d) {
									obj.gameObject3d->SetCurrentAnimationIndex(static_cast<uint32_t>(curClip));
									obj.gameObject3d->ResetAnimationTime();
								}
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}

							if (ImGui::Checkbox("Play Animation", &obj.animationPlaying)) {
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
							ImGui::SameLine();
							if (ImGui::Checkbox("Loop##Anim", &obj.animationLoop)) {
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
							ImGui::SameLine();
							if (ImGui::Button("Restart Clip")) {
								if (obj.editObject3d) obj.editObject3d->ResetAnimationTime();
								if (obj.gameObject3d) obj.gameObject3d->ResetAnimationTime();
							}

							if (ImGui::SliderFloat("Playback Speed", &obj.animationSpeed, 0.0f, 3.0f, "%.2fx")) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

							if (obj.editObject3d) {
								ImGui::TextDisabled("Current Time: %.2f s  |  Clips: %u", obj.editObject3d->GetAnimationTime(), animCount);
							}
						}
					}
				}

				// 3. BOX COLLIDER COMPONENT
				if (obj.hasBoxCollider) {
					if (DrawComponentHeader("Box Collider", ComponentKind::BoxCollider)) {
						if (ImGui::Checkbox("Enabled##BoxCol", &obj.boxColliderEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SameLine();
						if (ImGui::Button(isEditingColliderBounds_ ? " [ Editing Bounds ] " : " Edit Bounds ")) {
							isEditingColliderBounds_ = !isEditingColliderBounds_;
						}
						ImGui::SetItemTooltip("Enable interactive handle editing of box collider extents");

						if (ImGui::Checkbox("Is Trigger", &obj.isTrigger)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Trigger volume: detects overlaps without generating physical collision response");

						ImGui::SameLine();
						if (ImGui::Checkbox("Provides Contacts", &obj.providesContacts)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Generate contact points and impulse resolution");

						DrawVec3Control("Center", obj.boxCenter, 0.0f);
						ImGui::SetItemTooltip("Offset of box collider center relative to transform position");

						DrawVec3Control("Size", obj.boxSize, 1.0f);
						ImGui::SetItemTooltip("Dimensions of box collider along X, Y, Z axes");
					}
				}

				// 4. SPHERE COLLIDER COMPONENT
				if (obj.hasSphereCollider) {
					if (DrawComponentHeader("Sphere Collider", ComponentKind::SphereCollider)) {
						if (ImGui::Checkbox("Enabled##SphCol", &obj.sphereColliderEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						if (ImGui::Checkbox("Is Trigger##Sph", &obj.sphereIsTrigger)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						DrawVec3Control("Center##Sph", obj.sphereCenter, 0.0f);
						ImGui::SetItemTooltip("Center offset of sphere collider");

						if (ImGui::DragFloat("Radius", &obj.sphereRadius, 0.05f, 0.01f, 100.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Radius of sphere collider");
					}
				}

				// 5. RIGIDBODY COMPONENT
				if (obj.hasRigidbody) {
					if (DrawComponentHeader("Rigidbody", ComponentKind::Rigidbody)) {
						if (ImGui::Checkbox("Enabled##RB", &obj.rbEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }

						if (ImGui::DragFloat("Mass (kg)", &obj.mass, 0.1f, 0.01f, 1000.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Mass of the object in kilograms");

						if (ImGui::DragFloat("Linear Drag", &obj.drag, 0.01f, 0.0f, 10.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Linear air resistance damping");

						if (ImGui::DragFloat("Gravity Scale", &obj.gravityScale, 0.05f, -10.0f, 10.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Multiplier for gravitational acceleration");

						if (ImGui::Checkbox("Use Gravity", &obj.useGravity)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Enable constant downward gravitational acceleration");

						ImGui::SameLine();
						if (ImGui::Checkbox("Is Kinematic", &obj.isKinematic)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						ImGui::SetItemTooltip("Ignore physical forces and move only via script or transform");

						if (ImGui::SliderFloat("Bounciness", &obj.bounciness, 0.0f, 1.0f, "%.2f")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Coefficient of restitution for collisions");
					}
				}

				// 6. CAMERA COMPONENT
				if (obj.hasCamera) {
					if (DrawComponentHeader("Camera", ComponentKind::Camera)) {
						if (ImGui::Checkbox("Enabled##Cam", &obj.cameraEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						if (ImGui::SliderFloat("Field of View", &obj.fov, 10.0f, 150.0f, "%.1f deg")) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						if (ImGui::DragFloat("Near Clip", &obj.nearClip, 0.01f, 0.001f, 10.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						if (ImGui::DragFloat("Far Clip", &obj.farClip, 5.0f, 10.0f, 5000.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
					}
				}

				// 7. LIGHT COMPONENT (Directional + Point Light 03_00 + Spot Light 03_01)
				if (obj.hasLight) {
					if (DrawComponentHeader("Light", ComponentKind::Light)) {
						if (ImGui::Checkbox("Enabled##Light", &obj.lightEnabled)) { isSceneDirty_ = true; PushUndoSnapshot(); }
						const char* lightTypes[] = { "Directional", "Point", "Spot" };
						int currentLight = static_cast<int>(obj.lightType);
						if (ImGui::Combo("Light Type", &currentLight, lightTypes, 3)) {
							obj.lightType = static_cast<GameObject::LightType>(currentLight);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Light emitter type: Directional, Point Light (03_00), or Spot Light (03_01)");

						if (ImGui::ColorEdit3("Color", &obj.lightColor.x)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("RGB light emission color");

						if (ImGui::DragFloat("Intensity", &obj.lightIntensity, 0.05f, 0.0f, 100.0f)) isSceneDirty_ = true;
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Light brightness multiplier");

						if (obj.lightType == GameObject::LightType::Directional) {
							if (DrawVec3Control("Direction", obj.lightDirection, 0.0f)) {
								Vector3 d = obj.lightDirection;
								float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
								if (dLen > 0.0001f) {
									d = { d.x / dLen, d.y / dLen, d.z / dLen };
									obj.rotation.x = std::asin(std::clamp(-d.y, -1.0f, 1.0f));
									obj.rotation.y = std::atan2(d.x, d.z);
									obj.rotation.z = 0.0f;
									obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
								}
							}
							ImGui::SetItemTooltip("Direction vector of light emission (linked with Transform Rotation)");

							if (ImGui::Checkbox("Cast Shadows##Light", &obj.lightCastShadow)) { isSceneDirty_ = true; PushUndoSnapshot(); }
							ImGui::SetItemTooltip("Enable directional light shadow map rendering and attenuation");

							if (ImGui::SliderFloat("Shadow Bias", &obj.lightShadowBias, 0.0001f, 0.02f, "%.4f")) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Depth bias offset to prevent shadow acne (self-shadow artifacts)");

							if (ImGui::SliderFloat("Shadow Strength", &obj.lightShadowStrength, 0.0f, 1.0f, "%.2f")) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Intensity of the shadows cast by this light (1.0 = pitch black shadows, 0.0 = no shadow attenuation)");
						} else {
							if (ImGui::DragFloat(obj.lightType == GameObject::LightType::Point ? "Radius (Max Dist)" : "Distance", &obj.lightRange, 0.2f, 0.1f, 200.0f)) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Maximum illumination radius / distance");

							if (ImGui::DragFloat("Decay", &obj.lightDecay, 0.1f, 0.0f, 10.0f)) isSceneDirty_ = true;
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Attenuation decay exponent");

							if (obj.lightType == GameObject::LightType::Spot) {
								if (DrawVec3Control("Direction", obj.lightDirection, 0.0f)) {
									Vector3 d = obj.lightDirection;
									float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
									if (dLen > 0.0001f) {
										d = { d.x / dLen, d.y / dLen, d.z / dLen };
										obj.rotation.x = std::asin(std::clamp(-d.y, -1.0f, 1.0f));
										obj.rotation.y = std::atan2(d.x, d.z);
										obj.rotation.z = 0.0f;
										obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
									}
								}
								ImGui::SetItemTooltip("Direction vector of spot cone (linked with Transform Rotation)");
								if (ImGui::SliderFloat("Spot Angle", &obj.lightSpotAngle, 1.0f, 90.0f)) isSceneDirty_ = true;
								if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
								ImGui::SetItemTooltip("Cone half-angle of spot light in degrees");

								if (ImGui::SliderFloat("Falloff Start Angle", &obj.lightSpotFalloffStart, 1.0f, obj.lightSpotAngle)) isSceneDirty_ = true;
								if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							}
						}
					}
				}

				// 8. PARTICLE SYSTEM COMPONENT (Parent Attachment, Presets & Controls)
				if (obj.hasParticleSystem) {
					EnsureParticleQuadInitialized(obj);
					if (DrawComponentHeader("Particle System", ComponentKind::ParticleSystem)) {
						// 1. Parent Attachment
						std::vector<std::string> parentNames;
						std::vector<int> parentIds;
						parentNames.push_back("None (World)");
						parentIds.push_back(-1);

						int selectedParentIdx = 0;
						for (size_t p = 0; p < gameObjects_.size(); ++p) {
							if (gameObjects_[p].id != obj.id) {
								if (static_cast<int>(gameObjects_[p].id) == obj.attachedParentId) {
									selectedParentIdx = static_cast<int>(parentIds.size());
								}
								parentNames.push_back("[" + std::to_string(gameObjects_[p].id) + "] " + gameObjects_[p].name);
								parentIds.push_back(static_cast<int>(gameObjects_[p].id));
							}
						}

						std::vector<const char*> parentNamePtrs;
						for (const auto& name : parentNames) parentNamePtrs.push_back(name.c_str());

						if (ImGui::Combo("Attach To GameObject", &selectedParentIdx, parentNamePtrs.data(), static_cast<int>(parentNamePtrs.size()))) {
							obj.attachedParentId = parentIds[selectedParentIdx];
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Attach particle emitter to track a parent GameObject");

						if (obj.attachedParentId != -1) {
							if (DrawVec3Control("Local Offset", obj.localOffset, 0.0f)) {
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Position offset relative to parent GameObject");
						}

						ImGui::Separator();

						// 2. Presets
						const char* presetNames[] = {
							"Select Preset...",
							"Smoke",
							"Water Fountain",
							"Fireworks",
							"Explosion",
							"Torch / Campfire"
						};
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 110.0f);
						if (ImGui::Combo("Preset Template", &obj.particlePreset, presetNames, 6)) {
							if (obj.particlePreset > 0) {
								ApplyParticlePreset(obj, obj.particlePreset);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::SetItemTooltip("Select quick particle preset template");
						ImGui::SameLine();
						if (ImGui::Button("Apply Preset", ImVec2(100.0f, 0.0f))) {
							if (obj.particlePreset > 0) {
								ApplyParticlePreset(obj, obj.particlePreset);
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
						ImGui::SetItemTooltip("Apply selected preset to fill all parameters above");

						ImGui::Separator();

						// 3. Emitter Shape & Volume
						const char* shapeNames[] = { "Point", "Sphere", "Box", "Cone" };
						if (ImGui::Combo("Emitter Shape", &obj.particleShape, shapeNames, 4)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Shape geometry of particle emission source");

						if (obj.particleShape == 1 || obj.particleShape == 3) {
							if (ImGui::DragFloat("Spawn Radius", &obj.particleSpawnRadius, 0.01f, 0.0f, 20.0f, "%.2f")) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Radius of emission volume");
						}
						if (obj.particleShape == 2) {
							if (DrawVec3Control("Box Extents", obj.particleBoxExtents, 1.0f)) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Width, height, and depth extents of emission box");
						}
						if (obj.particleShape == 3) {
							if (ImGui::DragFloat("Cone Angle", &obj.particleConeAngle, 0.1f, 0.0f, 90.0f, "%.1f deg")) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Angle of cone emission apex");
						}

						ImGui::Separator();

						// 4. Emission Rate & Capacity
						if (ImGui::DragFloat("Spawn Rate", &obj.particleSpawnRate, 0.5f, 0.1f, 200.0f, "%.1f /s")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Particles spawned per second");

						if (ImGui::DragInt("Max Particles", &obj.particleMaxParticles, 1.0f, 1, 1000)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Maximum particle pool capacity for this emitter");

						// Burst Now button with count
						ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
						if (ImGui::DragInt("##BurstCount", &obj.particleBurstCount, 1.0f, 1, 200, "Count: %d")) {
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SameLine();
						if (ImGui::Button("Burst Now", ImVec2(110.0f, 0.0f))) {
							if (obj.particleQuad) {
								obj.particleQuad->EmitBurst(static_cast<uint32_t>(obj.particleBurstCount));
							}
						}
						ImGui::SetItemTooltip("Instantly emit a burst of particles");

						ImGui::Separator();

						// 5. Move Direction & Velocity
						if (DrawVec3Control("Move Direction", obj.particleDirection, 0.0f)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Primary emission travel direction vector");

						if (ImGui::SliderFloat("Spread Angle", &obj.particleSpreadAngle, 0.0f, 180.0f, "%.1f deg")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Velocity dispersion cone (0 = straight beam, 180 = full spherical omni)");

						float speedRange[2] = { obj.particleSpeedMin, obj.particleSpeedMax };
						if (ImGui::DragFloat2("Speed Min / Max", speedRange, 0.05f, 0.0f, 100.0f, "%.2f")) {
							obj.particleSpeedMin = speedRange[0];
							obj.particleSpeedMax = (std::max)(speedRange[0], speedRange[1]);
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Initial emission velocity speed range");

						ImGui::Separator();

						// 6. Forces & Drag
						if (ImGui::DragFloat("Gravity Modifier", &obj.particleGravity, 0.05f, -50.0f, 50.0f, "%.2f")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Vertical acceleration (negative = downward fall, positive = upward draft)");

						if (ImGui::DragFloat("Linear Drag", &obj.particleDrag, 0.01f, 0.0f, 10.0f, "%.2f")) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Air resistance deceleration factor");

						// Wind Field
						if (ImGui::Checkbox("Enable Wind Field", &obj.particleWindEnabled)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Enable global directional wind force on particles");
						if (obj.particleWindEnabled) {
							if (DrawVec3Control("Wind Accel", obj.particleWindAcceleration, 0.0f)) {
								SyncGameObjectToParticleQuad(obj);
								isSceneDirty_ = true;
							}
							if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
							ImGui::SetItemTooltip("Continuous directional acceleration applied to active particles");
						}

						ImGui::Separator();

						// 7. Lifetime & Scale
						float lifeRange[2] = { obj.particleLifetimeMin, obj.particleLifetimeMax };
						if (ImGui::DragFloat2("Lifetime Min / Max", lifeRange, 0.05f, 0.05f, 30.0f, "%.2f s")) {
							obj.particleLifetimeMin = lifeRange[0];
							obj.particleLifetimeMax = (std::max)(lifeRange[0], lifeRange[1]);
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Lifespan duration of individual particles");

						float scaleRange[2] = { obj.particleStartScale, obj.particleEndScale };
						if (ImGui::DragFloat2("Start -> End Scale", scaleRange, 0.01f, 0.001f, 10.0f, "%.2f")) {
							obj.particleStartScale = scaleRange[0];
							obj.particleEndScale = scaleRange[1];
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Particle scale at birth (left) interpolated to death (right)");

						ImGui::Separator();

						// 8. Color & Blending
						float startCol[4] = { obj.particleStartColor.x, obj.particleStartColor.y, obj.particleStartColor.z, obj.particleStartColor.w };
						if (ImGui::ColorEdit4("Start Color", startCol)) {
							obj.particleStartColor = { startCol[0], startCol[1], startCol[2], startCol[3] };
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Initial particle color and opacity at birth");

						float endCol[4] = { obj.particleEndColor.x, obj.particleEndColor.y, obj.particleEndColor.z, obj.particleEndColor.w };
						if (ImGui::ColorEdit4("End Color", endCol)) {
							obj.particleEndColor = { endCol[0], endCol[1], endCol[2], endCol[3] };
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();
						ImGui::SetItemTooltip("Final particle color and opacity at death");

						const char* blendModeNames[] = { "Normal Alpha", "Additive Glow" };
						if (ImGui::Combo("Blend Mode", &obj.particleBlendMode, blendModeNames, 2)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Alpha blending: Normal Alpha vs Additive Glow");

						const char* billboardModeNames[] = { "Camera Facing", "Y-Axis Cylindrical", "Velocity Aligned" };
						if (ImGui::Combo("Billboard Mode", &obj.particleBillboardMode, billboardModeNames, 3)) {
							SyncGameObjectToParticleQuad(obj);
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Billboard alignment: Camera Facing, Cylindrical Y, or Velocity Aligned");
					}
				}

				// 9. MAPCHIP LEVEL EDITOR
				if (obj.hasMapchip || obj.type == GameObjectType::MapchipGrid) {
					DrawMapchipInspectorSection();
				}

				// 9b. AUDIO SOURCE COMPONENT (XAudio2 + Windows Media Foundation: .wav / .mp3 / .m4a / .wma)
				if (obj.hasAudioSource || obj.type == GameObjectType::AudioSource) {
					obj.hasAudioSource = true;
					if (DrawComponentHeader("Audio Source (XAudio2 + Media Foundation)", ComponentKind::AudioSource)) {
						// Discover available audio files in Resources/Sound and Assets/Audio
						std::vector<std::string> audioClips;
						audioClips.push_back("None");
						std::vector<std::string> soundDirs = { "Resources/Sound", "Resources", "Assets/Audio", "Assets" };
						for (const auto& dir : soundDirs) {
							std::error_code ec;
							if (std::filesystem::exists(dir, ec) && std::filesystem::is_directory(dir, ec)) {
								for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
									if (!entry.is_regular_file(ec)) continue;
									std::string ext = entry.path().extension().string();
									std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
									if (ext == ".wav" || ext == ".mp3" || ext == ".m4a" || ext == ".wma") {
										std::string relPath = entry.path().generic_string();
										if (std::find(audioClips.begin(), audioClips.end(), relPath) == audioClips.end()) {
											audioClips.push_back(relPath);
										}
									}
								}
							}
						}

						int curClipIdx = 0;
						for (size_t cIdx = 0; cIdx < audioClips.size(); ++cIdx) {
							if (audioClips[cIdx] == obj.audioClipName) {
								curClipIdx = static_cast<int>(cIdx);
								break;
							}
						}

						std::vector<const char*> clipPtrs;
						for (const auto& s : audioClips) clipPtrs.push_back(s.c_str());

						if (ImGui::Combo("Audio ClipPreset", &curClipIdx, clipPtrs.data(), static_cast<int>(clipPtrs.size()))) {
							obj.audioClipName = audioClips[curClipIdx];
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(ResolveAssetPath(obj.audioClipName));
							}
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SetItemTooltip("Select audio file (.wav, .mp3, .m4a, .wma) decoded via Media Foundation & cached in Audio::LoadAudio");

						char clipBuf[256] = {};
						strncpy_s(clipBuf, obj.audioClipName.c_str(), sizeof(clipBuf) - 1);
						if (ImGui::InputText("Clip Path", clipBuf, sizeof(clipBuf))) {
							obj.audioClipName = clipBuf;
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) {
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(ResolveAssetPath(obj.audioClipName));
							}
							PushUndoSnapshot();
						}

						if (ImGui::SliderFloat("Volume", &obj.audioVolume, 0.0f, 1.0f, "%.2f")) {
							if (obj.runtimeVoiceHandle != 0) {
								Audio::GetInstance()->SetVolume(obj.runtimeVoiceHandle, obj.audioVolume);
							}
							isSceneDirty_ = true;
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) PushUndoSnapshot();

						if (ImGui::Checkbox("Play On Awake", &obj.audioPlayOnAwake)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						ImGui::SameLine();
						if (ImGui::Checkbox("Loop##Audio", &obj.audioLoop)) {
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}

						bool isPlayingNow = (obj.runtimeVoiceHandle != 0) && Audio::GetInstance()->IsPlaying(obj.runtimeVoiceHandle);
						if (isPlayingNow) {
							ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.45f, 1.0f), "Status: Playing (Voice #%u)", obj.runtimeVoiceHandle);
						} else {
							ImGui::TextDisabled("Status: Stopped");
						}

						if (ImGui::Button("[ > Play Preview ]", ImVec2(135.0f, 24.0f))) {
							if (obj.audioClipName != "None" && !obj.audioClipName.empty()) {
								if (obj.runtimeVoiceHandle != 0) {
									Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
								}
								obj.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(ResolveAssetPath(obj.audioClipName));
								obj.runtimeVoiceHandle = Audio::GetInstance()->PlayAudio(obj.runtimeSoundHandle, obj.audioLoop, obj.audioVolume);
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("[ [] Stop ]##AudioStop", ImVec2(100.0f, 24.0f))) {
							if (obj.runtimeVoiceHandle != 0) {
								Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
								obj.runtimeVoiceHandle = 0;
							}
						}
					}
				}

				// 10. SCRIPT COMPONENTS
				EnsureEditModeScriptInstances();
				for (size_t sIdx = 0; sIdx < obj.scripts.size(); ) {
					ImGui::PushID(static_cast<int>(sIdx));
					std::string headerLabel = std::format("{} (Script)##ScriptHeader_{}", obj.scripts[sIdx].className, sIdx);
					bool open = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
					bool removeScript = false;
					if (ImGui::BeginPopupContextItem("ScriptCompCtx")) {
						if (ImGui::MenuItem("Open Script (.cpp)")) {
							std::filesystem::path cppPath = ResolveGameScriptsDir() / "Scripts" / (obj.scripts[sIdx].className + ".cpp");
							OpenFileInVisualStudio(cppPath);
						}
						if (ImGui::MenuItem("Open Header (.h)")) {
							std::filesystem::path hPath = ResolveGameScriptsDir() / "Scripts" / (obj.scripts[sIdx].className + ".h");
							OpenFileInVisualStudio(hPath);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Remove Component")) {
							removeScript = true;
						}
						ImGui::EndPopup();
					}
					if (open) {
						ImGui::Indent(8.0f);
						auto& sc = obj.scripts[sIdx];
						ImGui::Text("Script Class: %s", sc.className.c_str());
						bool isReg = scriptModule_.IsRegistered(sc.className);
						if (sc.isCompiling || (isBuildingScripts_.load() && !isReg)) {
							ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f), "Status: Compiling...");
						} else if (!isReg || sc.isMissingScript) {
							ImGui::TextColored(ImVec4(1.0f, 0.40f, 0.25f, 1.0f), "Status: Missing Script (Click [ Build Scripts ] or fix class)");
						} else if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause) {
							ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Status: Active (Running in Play Mode)");
						} else {
							ImGui::TextDisabled("Status: Ready (GameScripts.dll)");
						}

						// [ Open Script ] and [ .h ] buttons on the same line, plus [ Remove ]
						float availW = ImGui::GetContentRegionAvail().x;
						float openW = (std::max)(90.0f, availW - 135.0f);
						if (ImGui::Button("[ Open Script ]", ImVec2(openW, 24.0f))) {
							std::filesystem::path cppPath = ResolveGameScriptsDir() / "Scripts" / (sc.className + ".cpp");
							OpenFileInVisualStudio(cppPath);
						}
						ImGui::SetItemTooltip("Open %s.cpp in Visual Studio", sc.className.c_str());

						ImGui::SameLine();
						if (ImGui::Button("[ .h ]", ImVec2(45.0f, 24.0f))) {
							std::filesystem::path hPath = ResolveGameScriptsDir() / "Scripts" / (sc.className + ".h");
							OpenFileInVisualStudio(hPath);
						}
						ImGui::SetItemTooltip("Open %s.h in Visual Studio", sc.className.c_str());

						ImGui::SameLine();
						if (ImGui::Button("Remove", ImVec2(75.0f, 24.0f))) {
							removeScript = true;
						}
						ImGui::SetItemTooltip("Remove this Script component from GameObject");

						// Exposed Fields UI (EXPOSE_FIELD reflection)
						if (sc.instance && isReg) {
							int fieldCount = scriptModule_.GetFieldCount(sc.instance.get());
							if (fieldCount > 0) {
								ImGui::Spacing();
								ImGui::Separator();
								ImGui::TextDisabled("Exposed Fields:");
								for (int fIdx = 0; fIdx < fieldCount; ++fIdx) {
									ScriptFieldInfo info{};
									if (!scriptModule_.GetFieldInfo(sc.instance.get(), fIdx, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;

									ImGui::PushID(fIdx);
									bool fieldEdited = false;
									bool fieldCommitted = false;

									switch (info.type) {
									case ScriptFieldType::Float: {
										float* ptr = static_cast<float*>(info.dataPtr);
										if (ImGui::DragFloat(info.name, ptr, 0.05f, 0.0f, 0.0f, "%.3f")) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Int: {
										int* ptr = static_cast<int*>(info.dataPtr);
										if (ImGui::DragInt(info.name, ptr, 1.0f)) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Bool: {
										bool* ptr = static_cast<bool*>(info.dataPtr);
										if (ImGui::Checkbox(info.name, ptr)) {
											fieldEdited = true;
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::Vec3: {
										Vector3* ptr = static_cast<Vector3*>(info.dataPtr);
										if (DrawVec3Control(info.name, *ptr, 0.0f)) {
											fieldEdited = true;
											fieldCommitted = true;
										}
										break;
									}
									case ScriptFieldType::CharBuffer: {
										char* ptr = static_cast<char*>(info.dataPtr);
										if (ImGui::InputText(info.name, ptr, info.sizeInBytes)) {
											fieldEdited = true;
										}
										if (ImGui::IsItemDeactivatedAfterEdit()) {
											fieldCommitted = true;
										}
										break;
									}
									}

									if (fieldEdited || fieldCommitted) {
										CaptureScriptFieldsToSaved(sc);
										isSceneDirty_ = true;
									}
									if (fieldCommitted) {
										PushUndoSnapshot();
									}
									ImGui::PopID();
								}
							}
						}

						ImGui::Unindent(8.0f);
					}
					ImGui::PopID();

					if (removeScript) {
						if (obj.scripts[sIdx].instance) {
							if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode()) {
								obj.scripts[sIdx].instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
								obj.scripts[sIdx].instance->SetEngineAPI(&engineApi_);
								obj.scripts[sIdx].instance->SetHost(this, this);
								obj.scripts[sIdx].instance->OnDestroy();
							}
							obj.scripts[sIdx].instance.reset();
						}
						obj.scripts.erase(obj.scripts.begin() + static_cast<std::ptrdiff_t>(sIdx));
						obj.hasScript = !obj.scripts.empty();
						obj.scriptClassName = obj.scripts.empty() ? "" : obj.scripts.front().className;
						isSceneDirty_ = true;
						PushUndoSnapshot();
					} else {
						++sIdx;
					}
				}

				// Centered [ Add Component ] Button
				ImGui::Spacing();
				ImGui::Separator();
				float btnWidth = 180.0f;
				float availWidth = ImGui::GetContentRegionAvail().x;
				ImGui::SetCursorPosX((availWidth - btnWidth) * 0.5f);
				if (ImGui::Button("Add Component", ImVec2(btnWidth, 28.0f))) {
					ImGui::OpenPopup("AddComponentPopup");
				}

				if (ImGui::BeginPopup("AddComponentPopup")) {
					if (ImGui::MenuItem("Mesh Filter & Renderer (Cube)")) { obj.hasMesh = true; obj.meshType = GameObject::MeshType::Cube; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Textured Quad")) { obj.hasMesh = true; obj.meshType = GameObject::MeshType::Quad; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Box Collider")) { obj.hasBoxCollider = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Sphere Collider")) { obj.hasSphereCollider = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Rigidbody")) { obj.hasRigidbody = true; obj.rbEnabled = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Camera")) { obj.hasCamera = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Light")) { obj.hasLight = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Particle System")) { obj.hasParticleSystem = true; EnsureParticleQuadInitialized(obj); isSceneDirty_ = true; PushUndoSnapshot(); }
					if (ImGui::MenuItem("Audio Source")) { obj.hasAudioSource = true; isSceneDirty_ = true; PushUndoSnapshot(); }
					ImGui::Separator();
					if (ImGui::BeginMenu("Script")) {
						const auto& registeredScripts = scriptModule_.GetRegisteredNames();
						if (registeredScripts.empty()) {
							ImGui::TextDisabled("No registered scripts in GameScripts.dll");
						} else {
							for (const auto& scriptName : registeredScripts) {
								if (ImGui::MenuItem(scriptName.c_str())) {
									GameObject::ScriptComponent sc(scriptName);
									sc.instance = ScriptInstancePtr(scriptModule_.CreateScript(scriptName), ScriptInstanceDeleter{ &scriptModule_ });
									if (sc.instance) {
										RefreshAllScriptObjectViews();
										ScriptGameObjectView* view = GetOrUpdateScriptViewById(obj.id);
										sc.instance->SetOwner(view, obj.id);
										sc.instance->SetEngineAPI(&engineApi_);
										sc.instance->SetHost(this, this);
										CaptureScriptFieldsToSaved(sc);
										if (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode()) {
											sc.instance->Initialize();
											if (view && view->dirtyFromScript) {
												SyncViewToGameObject(*view, obj);
											}
										}
									}
									obj.scripts.push_back(std::move(sc));
									obj.hasScript = true;
									obj.scriptClassName = obj.scripts.front().className;
									isSceneDirty_ = true;
									PushUndoSnapshot();
								}
							}
						}
						ImGui::Separator();
						if (ImGui::MenuItem("New Script...")) {
							scriptGenTargetObjectIndex_ = selectedObjectIndex_;
							scriptGenErrorMsg_.clear();
							showScriptGenModal_ = true;
						}
						ImGui::EndMenu();
					}
					ImGui::EndPopup();
				}

				// Apply a "Remove Component" request now that nothing else reads this object's component data
				if (pendingComponentRemoval_ >= 0) {
					if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
						RemoveComponent(gameObjects_[selectedObjectIndex_], static_cast<ComponentKind>(pendingComponentRemoval_));
					}
					pendingComponentRemoval_ = -1;
				}

				ImGui::PopItemWidth();
				ImGui::EndTabItem();
			}

			// DETAILS TAB (Live Asset Previews & Engine Statistics)
			ImGuiTabItemFlags detailsTabFlags = focusDetailsTab_ ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
			if (ImGui::BeginTabItem("Details", nullptr, detailsTabFlags)) {
				focusDetailsTab_ = false;
				rightPanelTab_ = 1;
				ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Asset & Scene Details");
				ImGui::Separator();

				if (!selectedAssetPath_.empty() && std::filesystem::exists(selectedAssetPath_) && !std::filesystem::is_directory(selectedAssetPath_)) {
					std::string ext = selectedAssetPath_.extension().string();
					std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

					ImGui::Text("Selected Asset: %s", selectedAssetPath_.filename().string().c_str());
					ImGui::TextDisabled("Path: %s", selectedAssetPath_.string().c_str());

					try {
						auto sizeBytes = std::filesystem::file_size(selectedAssetPath_);
						if (sizeBytes < 1024) {
							ImGui::Text("File Size: %llu Bytes (%.2f KB)", static_cast<unsigned long long>(sizeBytes), sizeBytes / 1024.0f);
						} else {
							ImGui::Text("File Size: %.2f KB (%llu Bytes)", sizeBytes / 1024.0f, static_cast<unsigned long long>(sizeBytes));
						}
					} catch (...) {}

					if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds") {
						ImGui::Separator();
						ImGui::Text("2D Texture Preview:");
						if (currentPreviewTextureHandle_ == 0 && currentPreviewAssetPath_ != selectedAssetPath_.string()) {
							SelectAssetForPreview(selectedAssetPath_);
						}
						if (currentPreviewTextureHandle_ != 0) {
							ImGui::Image(
								ImTextureRef((ImTextureID)TextureManager::GetInstance()->GetGPUHandle(currentPreviewTextureHandle_).ptr),
								ImVec2(140.0f, 140.0f));
						}

						if (ImGui::Button("[ Create Textured Rectangle in Scene ]", ImVec2(-1.0f, 26.0f))) {
							uint32_t permHandle = TextureManager::GetInstance()->LoadTexture(selectedAssetPath_.string());
							auto& q = CreateQuad(selectedAssetPath_.stem().string(), { 0.0f, 1.0f, 0.0f });
							q.texturePath = ToProjectRelativePath(selectedAssetPath_);
							q.textureHandle = permHandle;
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
						if (selectedObjectIndex_ >= 0 && selectedObjectIndex_ < static_cast<int>(gameObjects_.size())) {
							if (ImGui::Button("[ Assign to Selected Object Material ]", ImVec2(-1.0f, 26.0f))) {
								uint32_t permHandle = TextureManager::GetInstance()->LoadTexture(selectedAssetPath_.string());
								gameObjects_[selectedObjectIndex_].texturePath = ToProjectRelativePath(selectedAssetPath_);
								gameObjects_[selectedObjectIndex_].textureHandle = permHandle;
								isSceneDirty_ = true;
								PushUndoSnapshot();
							}
						}
					} else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
						ImGui::Separator();
						ImGui::Text("3D Model Preview (Assimp):");
						if (!previewModel_ && currentPreviewAssetPath_ != selectedAssetPath_.string()) {
							SelectAssetForPreview(selectedAssetPath_);
						}
						ImGui::Text("Format: %s", ext.c_str());
						ImGui::Text("Vertices: %u  |  Triangles: %u", previewVertexCount_, previewTriangleCount_);
						if (previewModel_) {
							ImGui::Image(
								ImTextureRef((ImTextureID)modelPreviewRT_.GetSrvGPUHandle().ptr),
								ImVec2(180.0f, 180.0f));
						} else {
							ImGui::TextColored(ImVec4(0.95f, 0.4f, 0.4f, 1.0f), "Unable to parse 3D geometry.");
						}
						if (ImGui::Button("[ Spawn 3D Model in Scene ]", ImVec2(-1.0f, 26.0f))) {
							auto& obj = CreateGameObject(selectedAssetPath_.stem().string(), GameObjectType::Cube);
							obj.hasMesh = true;
							obj.meshEnabled = true;
							obj.meshType = GameObject::MeshType::CustomModel;
							obj.modelPath = ToProjectRelativePath(selectedAssetPath_);
							obj.translation = { 0.0f, 1.0f, 0.0f };
							isSceneDirty_ = true;
							PushUndoSnapshot();
							LogMessage(std::format("[Scene] Spawned 3D Model '{}' from '{}'", obj.name, obj.modelPath), 0);
						}
					} else if (ext == ".wav" || ext == ".mp3" || ext == ".m4a" || ext == ".wma") {
						ImGui::Separator();
						ImGui::Text("Audio Clip Preview (XAudio2 + Media Foundation):");
						if (ImGui::Button("[ > Play Audio Clip ]", ImVec2(-1.0f, 26.0f))) {
							uint32_t sh = Audio::GetInstance()->LoadAudio(selectedAssetPath_.string());
							Audio::GetInstance()->PlayAudio(sh, false, 1.0f);
						}
						if (ImGui::Button("[ [] Stop All Audio ]", ImVec2(-1.0f, 26.0f))) {
							Audio::GetInstance()->StopAllAudio();
						}
						if (ImGui::Button("[ Create Audio Source in Scene ]", ImVec2(-1.0f, 26.0f))) {
							auto& aud = CreateGameObject(selectedAssetPath_.stem().string(), GameObjectType::AudioSource);
							aud.hasAudioSource = true;
							aud.audioClipName = ToProjectRelativePath(selectedAssetPath_);
							aud.audioPlayOnAwake = true;
							aud.runtimeSoundHandle = Audio::GetInstance()->LoadAudio(ResolveAssetPath(aud.audioClipName));
							isSceneDirty_ = true;
							PushUndoSnapshot();
						}
					} else if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
						ImGui::Separator();
						ImGui::Text("C++ Script Source File:");
						if (ImGui::Button("[ Open in Visual Studio ]", ImVec2(-1.0f, 26.0f))) {
							OpenFileInVisualStudio(selectedAssetPath_);
						}
					}
					ImGui::Separator();
				} else {
					ImGui::TextDisabled("Select a texture (.png, .jpg, .dds) or 3D model (.obj, .fbx, .gltf) in the Project browser to inspect.");
					ImGui::Separator();
				}

				ImGui::Text("Total GameObjects: %zu", gameObjects_.size());
				ImGui::Text("Active Cameras: %s", gameCamera_ ? "1" : "0");
				ImGui::Text("Lighting: %s", enableLighting_ ? "Enabled" : "Disabled");
				ImGui::Checkbox("Frustum Culling", &enableFrustumCulling_);
				ImGui::Text("Frustum Culling Stats: %d Visible / %d Culled", lastVisibleObjects_, lastCulledObjects_);

				ImGui::EndTabItem();
			} else {
				focusDetailsTab_ = false;
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
#endif
}

bool EditorSuite::DrawVec3Control(const std::string& label, Vector3& values, float resetValue, float columnWidth)
{
	bool modified = false;
#ifdef USE_IMGUI
	ImGui::PushID(label.c_str());

	if (ImGui::BeginTable("Vec3ControlTable", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("LabelCol", ImGuiTableColumnFlags_WidthFixed, columnWidth);
		ImGui::TableSetupColumn("ControlCol", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(label.c_str());

		ImGui::TableNextColumn();
		float availWidth = ImGui::GetContentRegionAvail().x;
		float itemWidth = (availWidth - 54.0f) / 3.0f;
		ImVec2 buttonSize = ImVec2(18.0f, 0.0f);

		// X Axis (Red)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
		if (ImGui::Button("X", buttonSize)) {
			values.x = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}
		ImGui::SameLine(0.0f, 4.0f);

		// Y Axis (Green)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.20f, 1.0f));
		if (ImGui::Button("Y", buttonSize)) {
			values.y = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}
		ImGui::SameLine(0.0f, 4.0f);

		// Z Axis (Blue)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.35f, 0.78f, 1.0f));
		if (ImGui::Button("Z", buttonSize)) {
			values.z = resetValue;
			modified = true;
			isSceneDirty_ = true;
			PushUndoSnapshot();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine(0.0f, 2.0f);
		ImGui::SetNextItemWidth(itemWidth);
		if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) {
			modified = true;
			isSceneDirty_ = true;
		}
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			PushUndoSnapshot();
		}

		ImGui::EndTable();
	}

	ImGui::PopID();
#endif
	return modified;
}


bool EditorSuite::DrawComponentHeader(const char* label, ComponentKind kind)
{
#ifdef USE_IMGUI
	bool keep = true;
	// Passing &keep adds the close (x) button on the right side of the header
	const bool open = ImGui::CollapsingHeader(label, &keep, ImGuiTreeNodeFlags_DefaultOpen);
	ImGui::SetItemTooltip("Right-click or press x to remove this component");
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::MenuItem("Remove Component")) {
			keep = false;
		}
		ImGui::EndPopup();
	}
	if (!keep) {
		pendingComponentRemoval_ = static_cast<int>(kind);
	}
	return open && keep;
#else
	(void)label;
	(void)kind;
	return false;
#endif
}

void EditorSuite::RemoveComponent(GameObject& obj, ComponentKind kind)
{
	// Objects created from a typed menu entry (Camera, Light, ...) are recognised by their type in several
	// places (gizmos, camera sync, audio on Play), so removing that component also makes the object Empty.
	auto clearTypeIf = [&](std::initializer_list<GameObjectType> types) {
		for (GameObjectType t : types) {
			if (obj.type == t) {
				obj.type = GameObjectType::Empty;
				return;
			}
		}
	};

	const char* name = "Component";
	switch (kind) {
	case ComponentKind::Mesh:
		name = "Mesh Filter & Renderer";
		obj.hasMesh = false;
		obj.editObject3d.reset();
		obj.gameObject3d.reset();
		obj.shadowTransform.reset();
		clearTypeIf({ GameObjectType::Cube, GameObjectType::Sphere, GameObjectType::Plane, GameObjectType::Quad });
		break;
	case ComponentKind::BoxCollider:
		name = "Box Collider";
		obj.hasBoxCollider = false;
		break;
	case ComponentKind::SphereCollider:
		name = "Sphere Collider";
		obj.hasSphereCollider = false;
		break;
	case ComponentKind::Rigidbody:
		name = "Rigidbody";
		obj.hasRigidbody = false;
		obj.velocity = { 0.0f, 0.0f, 0.0f };
		obj.isGrounded = false;
		break;
	case ComponentKind::Camera:
		name = "Camera";
		obj.hasCamera = false;
		clearTypeIf({ GameObjectType::Camera });
		break;
	case ComponentKind::Light:
		name = "Light";
		obj.hasLight = false;
		clearTypeIf({ GameObjectType::DirectionalLight, GameObjectType::PointLight, GameObjectType::SpotLight });
		break;
	case ComponentKind::ParticleSystem:
		name = "Particle System";
		obj.hasParticleSystem = false;
		obj.particleQuad.reset(); // GPU resources go through DeferRelease in ~ParticleQuad
		obj.attachedParentId = -1;
		obj.attachToIndex = -1;
		clearTypeIf({ GameObjectType::ParticleSystem });
		break;
	case ComponentKind::AudioSource:
		name = "Audio Source";
		if (obj.runtimeVoiceHandle != 0) {
			Audio::GetInstance()->StopAudio(obj.runtimeVoiceHandle);
			obj.runtimeVoiceHandle = 0;
		}
		obj.hasAudioSource = false;
		clearTypeIf({ GameObjectType::AudioSource });
		break;
	case ComponentKind::Mapchip:
		name = "Mapchip Level Editor";
		obj.hasMapchip = false;
		clearTypeIf({ GameObjectType::MapchipGrid });
		break;
	}

	isSceneDirty_ = true;
	PushUndoSnapshot();
	LogMessage(std::format("[Inspector] Removed {} from '{}'.", name, obj.name), 0);
}
