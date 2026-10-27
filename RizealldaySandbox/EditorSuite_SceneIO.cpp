#include "EditorSuiteInternal.h"

using EditorSuiteDetail::SceneNameFromPath;
using EditorSuiteDetail::IsSceneFile;

std::string EditorSuite::SerializeSceneToString() const
{
	std::ostringstream ss;
	ss << "{\n";
	ss << "  \"sceneName\": \"" << currentSceneName_ << "\",\n";
	ss << "  \"skybox\": {\n";
	ss << "    \"mode\": " << static_cast<int>(skybox_.GetMode()) << ",\n";
	ss << "    \"topColor\": [" << skybox_.GetTopColor().x << ", " << skybox_.GetTopColor().y << ", " << skybox_.GetTopColor().z << ", " << skybox_.GetTopColor().w << "],\n";
	ss << "    \"horizonColor\": [" << skybox_.GetHorizonColor().x << ", " << skybox_.GetHorizonColor().y << ", " << skybox_.GetHorizonColor().z << ", " << skybox_.GetHorizonColor().w << "],\n";
	ss << "    \"bottomColor\": [" << skybox_.GetBottomColor().x << ", " << skybox_.GetBottomColor().y << ", " << skybox_.GetBottomColor().z << ", " << skybox_.GetBottomColor().w << "],\n";
	ss << "    \"solidColor\": [" << skybox_.GetSolidColor().x << ", " << skybox_.GetSolidColor().y << ", " << skybox_.GetSolidColor().z << ", " << skybox_.GetSolidColor().w << "],\n";
	ss << "    \"cubemapPath\": \"" << skybox_.GetCubemapPath() << "\",\n";
	ss << "    \"rotation\": " << skybox_.GetRotation() << ",\n";
	ss << "    \"exposure\": " << skybox_.GetExposure() << "\n";
	ss << "  },\n";
	ss << "  \"gameObjects\": [\n";

	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		const auto& obj = gameObjects_[i];
		ss << "    {\n";
		ss << "      \"id\": " << obj.id << ",\n";
		ss << "      \"name\": \"" << obj.name << "\",\n";
		ss << "      \"type\": " << static_cast<int>(obj.type) << ",\n";
		ss << "      \"isActive\": " << (obj.isActive ? "true" : "false") << ",\n";
		ss << "      \"isStatic\": " << (obj.isStatic ? "true" : "false") << ",\n";
		ss << "      \"tag\": \"" << obj.tag << "\",\n";
		ss << "      \"layer\": " << obj.layer << ",\n";
		ss << "      \"parentId\": " << obj.parentId << ",\n";
		ss << "      \"translation\": [" << obj.translation.x << ", " << obj.translation.y << ", " << obj.translation.z << "],\n";
		ss << "      \"rotation\": [" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << "],\n";
		ss << "      \"rotationQuat\": [" << obj.rotationQuat.x << ", " << obj.rotationQuat.y << ", " << obj.rotationQuat.z << ", " << obj.rotationQuat.w << "],\n";
		ss << "      \"scale\": [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "],\n";

		// Mesh
		ss << "      \"hasMesh\": " << (obj.hasMesh ? "true" : "false") << ",\n";
		ss << "      \"meshEnabled\": " << (obj.meshEnabled ? "true" : "false") << ",\n";
		ss << "      \"meshType\": " << static_cast<int>(obj.meshType) << ",\n";
		ss << "      \"materialColor\": [" << obj.materialColor.x << ", " << obj.materialColor.y << ", " << obj.materialColor.z << ", " << obj.materialColor.w << "],\n";
		ss << "      \"isLit\": " << (obj.isLit ? "true" : "false") << ",\n";
		ss << "      \"lightingType\": " << obj.lightingType << ",\n";
		ss << "      \"shininess\": " << obj.shininess << ",\n";
		ss << "      \"specularColor\": [" << obj.specularColor.x << ", " << obj.specularColor.y << ", " << obj.specularColor.z << "],\n";
		ss << "      \"castShadow\": " << (obj.castShadow ? "true" : "false") << ",\n";
		ss << "      \"receiveShadow\": " << (obj.receiveShadow ? "true" : "false") << ",\n";
		ss << "      \"useGeometryShader\": " << (obj.useGeometryShader ? "true" : "false") << ",\n";
		ss << "      \"geometryShaderEffect\": " << obj.geometryShaderEffect << ",\n";
		ss << "      \"explodeAmount\": " << obj.explodeAmount << ",\n";
		ss << "      \"texturePath\": \"" << obj.texturePath << "\",\n";
		ss << "      \"modelPath\": \"" << obj.modelPath << "\",\n";
		ss << "      \"uvOffset\": [" << obj.uvOffset.x << ", " << obj.uvOffset.y << "],\n";
		ss << "      \"uvTiling\": [" << obj.uvTiling.x << ", " << obj.uvTiling.y << "],\n";
		ss << "      \"uvRotation\": " << obj.uvRotation << ",\n";

		// Colliders
		ss << "      \"hasBoxCollider\": " << (obj.hasBoxCollider ? "true" : "false") << ",\n";
		ss << "      \"boxColliderEnabled\": " << (obj.boxColliderEnabled ? "true" : "false") << ",\n";
		ss << "      \"isTrigger\": " << (obj.isTrigger ? "true" : "false") << ",\n";
		ss << "      \"providesContacts\": " << (obj.providesContacts ? "true" : "false") << ",\n";
		ss << "      \"boxCenter\": [" << obj.boxCenter.x << ", " << obj.boxCenter.y << ", " << obj.boxCenter.z << "],\n";
		ss << "      \"boxSize\": [" << obj.boxSize.x << ", " << obj.boxSize.y << ", " << obj.boxSize.z << "],\n";
		ss << "      \"hasSphereCollider\": " << (obj.hasSphereCollider ? "true" : "false") << ",\n";
		ss << "      \"sphereColliderEnabled\": " << (obj.sphereColliderEnabled ? "true" : "false") << ",\n";
		ss << "      \"sphereIsTrigger\": " << (obj.sphereIsTrigger ? "true" : "false") << ",\n";
		ss << "      \"sphereCenter\": [" << obj.sphereCenter.x << ", " << obj.sphereCenter.y << ", " << obj.sphereCenter.z << "],\n";
		ss << "      \"sphereRadius\": " << obj.sphereRadius << ",\n";

		// Rigidbody
		ss << "      \"hasRigidbody\": " << (obj.hasRigidbody ? "true" : "false") << ",\n";
		ss << "      \"rbEnabled\": " << (obj.rbEnabled ? "true" : "false") << ",\n";
		ss << "      \"mass\": " << obj.mass << ",\n";
		ss << "      \"drag\": " << obj.drag << ",\n";
		ss << "      \"gravityScale\": " << obj.gravityScale << ",\n";
		ss << "      \"useGravity\": " << (obj.useGravity ? "true" : "false") << ",\n";
		ss << "      \"isKinematic\": " << (obj.isKinematic ? "true" : "false") << ",\n";
		ss << "      \"bounciness\": " << obj.bounciness << ",\n";

		// Camera
		ss << "      \"hasCamera\": " << (obj.hasCamera ? "true" : "false") << ",\n";
		ss << "      \"cameraEnabled\": " << (obj.cameraEnabled ? "true" : "false") << ",\n";
		ss << "      \"fov\": " << obj.fov << ",\n";
		ss << "      \"nearClip\": " << obj.nearClip << ",\n";
		ss << "      \"farClip\": " << obj.farClip << ",\n";
		ss << "      \"isOrthographic\": " << (obj.isOrthographic ? "true" : "false") << ",\n";

		// Light
		ss << "      \"hasLight\": " << (obj.hasLight ? "true" : "false") << ",\n";
		ss << "      \"lightEnabled\": " << (obj.lightEnabled ? "true" : "false") << ",\n";
		ss << "      \"lightType\": " << static_cast<int>(obj.lightType) << ",\n";
		ss << "      \"lightColor\": [" << obj.lightColor.x << ", " << obj.lightColor.y << ", " << obj.lightColor.z << "],\n";
		ss << "      \"lightIntensity\": " << obj.lightIntensity << ",\n";
		ss << "      \"lightDirection\": [" << obj.lightDirection.x << ", " << obj.lightDirection.y << ", " << obj.lightDirection.z << "],\n";
		ss << "      \"lightRange\": " << obj.lightRange << ",\n";
		ss << "      \"lightDecay\": " << obj.lightDecay << ",\n";
		ss << "      \"lightSpotAngle\": " << obj.lightSpotAngle << ",\n";
		ss << "      \"lightSpotFalloffStart\": " << obj.lightSpotFalloffStart << ",\n";
		ss << "      \"lightCastShadow\": " << (obj.lightCastShadow ? "true" : "false") << ",\n";
		ss << "      \"lightShadowBias\": " << obj.lightShadowBias << ",\n";
		ss << "      \"lightShadowStrength\": " << obj.lightShadowStrength << ",\n";
		ss << "      \"lightShadowNormalBias\": " << obj.lightShadowNormalBias << ",\n";

		// Particle System
		ss << "      \"hasParticleSystem\": " << (obj.hasParticleSystem ? "true" : "false") << ",\n";
		ss << "      \"attachedParentId\": " << obj.attachedParentId << ",\n";
		ss << "      \"localOffset\": [" << obj.localOffset.x << ", " << obj.localOffset.y << ", " << obj.localOffset.z << "],\n";
		ss << "      \"particlePreset\": " << obj.particlePreset << ",\n";
		ss << "      \"particleShape\": " << obj.particleShape << ",\n";
		ss << "      \"particleSpawnRadius\": " << obj.particleSpawnRadius << ",\n";
		ss << "      \"particleBoxExtents\": [" << obj.particleBoxExtents.x << ", " << obj.particleBoxExtents.y << ", " << obj.particleBoxExtents.z << "],\n";
		ss << "      \"particleConeAngle\": " << obj.particleConeAngle << ",\n";
		ss << "      \"particleSpawnRate\": " << obj.particleSpawnRate << ",\n";
		ss << "      \"particleMaxParticles\": " << obj.particleMaxParticles << ",\n";
		ss << "      \"particleBurstCount\": " << obj.particleBurstCount << ",\n";
		ss << "      \"particleDirection\": [" << obj.particleDirection.x << ", " << obj.particleDirection.y << ", " << obj.particleDirection.z << "],\n";
		ss << "      \"particleSpreadAngle\": " << obj.particleSpreadAngle << ",\n";
		ss << "      \"particleSpeedMin\": " << obj.particleSpeedMin << ",\n";
		ss << "      \"particleSpeedMax\": " << obj.particleSpeedMax << ",\n";
		ss << "      \"particleGravity\": " << obj.particleGravity << ",\n";
		ss << "      \"particleDrag\": " << obj.particleDrag << ",\n";
		ss << "      \"particleWindEnabled\": " << (obj.particleWindEnabled ? "true" : "false") << ",\n";
		ss << "      \"particleWindAcceleration\": [" << obj.particleWindAcceleration.x << ", " << obj.particleWindAcceleration.y << ", " << obj.particleWindAcceleration.z << "],\n";
		ss << "      \"particleLifetimeMin\": " << obj.particleLifetimeMin << ",\n";
		ss << "      \"particleLifetimeMax\": " << obj.particleLifetimeMax << ",\n";
		ss << "      \"particleStartScale\": " << obj.particleStartScale << ",\n";
		ss << "      \"particleEndScale\": " << obj.particleEndScale << ",\n";
		ss << "      \"particleStartColor\": [" << obj.particleStartColor.x << ", " << obj.particleStartColor.y << ", " << obj.particleStartColor.z << ", " << obj.particleStartColor.w << "],\n";
		ss << "      \"particleEndColor\": [" << obj.particleEndColor.x << ", " << obj.particleEndColor.y << ", " << obj.particleEndColor.z << ", " << obj.particleEndColor.w << "],\n";
		ss << "      \"particleBlendMode\": " << obj.particleBlendMode << ",\n";
		ss << "      \"particleBillboardMode\": " << obj.particleBillboardMode << ",\n";

		// Mapchip, Audio, Animation & Scripts
		ss << "      \"hasMapchip\": " << (obj.hasMapchip ? "true" : "false") << ",\n";
		ss << "      \"hasAudioSource\": " << (obj.hasAudioSource ? "true" : "false") << ",\n";
		ss << "      \"audioClipName\": \"" << obj.audioClipName << "\",\n";
		ss << "      \"audioVolume\": " << obj.audioVolume << ",\n";
		ss << "      \"audioPlayOnAwake\": " << (obj.audioPlayOnAwake ? "true" : "false") << ",\n";
		ss << "      \"audioLoop\": " << (obj.audioLoop ? "true" : "false") << ",\n";
		ss << "      \"animationIndex\": " << obj.animationIndex << ",\n";
		ss << "      \"animationSpeed\": " << obj.animationSpeed << ",\n";
		ss << "      \"animationLoop\": " << (obj.animationLoop ? "true" : "false") << ",\n";
		ss << "      \"animationPlaying\": " << (obj.animationPlaying ? "true" : "false") << ",\n";
		ss << "      \"hasScript\": " << ((!obj.scripts.empty() || obj.hasScript) ? "true" : "false") << ",\n";
		ss << "      \"scriptClassName\": \"" << (!obj.scripts.empty() ? obj.scripts.front().className : obj.scriptClassName) << "\",\n";
		ss << "      \"scripts\": [";
		for (size_t sIdx = 0; sIdx < obj.scripts.size(); ++sIdx) {
			ss << "\"" << obj.scripts[sIdx].className << "\"" << (sIdx + 1 < obj.scripts.size() ? ", " : "");
		}
		ss << "],\n";
		ss << "      \"scriptComponents\": [";
		for (size_t sIdx = 0; sIdx < obj.scripts.size(); ++sIdx) {
			const auto& sc = obj.scripts[sIdx];
			std::map<std::string, GameObject::ScriptSavedFieldValue> fieldsToWrite = sc.savedFields;
			if (sc.instance) {
				int fCount = scriptModule_.GetFieldCount(sc.instance.get());
				for (int fIdx = 0; fIdx < fCount; ++fIdx) {
					ScriptFieldInfo info{};
					if (!scriptModule_.GetFieldInfo(sc.instance.get(), fIdx, info) || !info.dataPtr || !info.name || info.name[0] == '\0') continue;
					GameObject::ScriptSavedFieldValue val{};
					val.type = info.type;
					switch (info.type) {
					case ScriptFieldType::Float: val.floatVal = *static_cast<const float*>(info.dataPtr); break;
					case ScriptFieldType::Int: val.intVal = *static_cast<const int*>(info.dataPtr); break;
					case ScriptFieldType::Bool: val.boolVal = *static_cast<const bool*>(info.dataPtr); break;
					case ScriptFieldType::Vec3: val.vec3Val = *static_cast<const Vector3*>(info.dataPtr); break;
					case ScriptFieldType::CharBuffer: val.strVal = static_cast<const char*>(info.dataPtr); break;
					}
					fieldsToWrite[info.name] = val;
				}
			}

			ss << "{\"className\":\"" << sc.className << "\",\"fields\":[";
			size_t fWritten = 0;
			for (const auto& [fName, fVal] : fieldsToWrite) {
				if (fWritten > 0) ss << ",";
				ss << "{\"name\":\"" << fName << "\",\"type\":" << static_cast<int>(fVal.type);
				switch (fVal.type) {
				case ScriptFieldType::Float:
					ss << ",\"v0\":" << fVal.floatVal << "}";
					break;
				case ScriptFieldType::Int:
					ss << ",\"v0\":" << fVal.intVal << "}";
					break;
				case ScriptFieldType::Bool:
					ss << ",\"v0\":" << (fVal.boolVal ? 1 : 0) << "}";
					break;
				case ScriptFieldType::Vec3:
					ss << ",\"v0\":" << fVal.vec3Val.x << ",\"v1\":" << fVal.vec3Val.y << ",\"v2\":" << fVal.vec3Val.z << "}";
					break;
				case ScriptFieldType::CharBuffer: {
					std::string escaped;
					for (char ch : fVal.strVal) {
						if (ch == '\"' || ch == '\\') escaped.push_back('\\');
						escaped.push_back(ch);
					}
					ss << ",\"str\":\"" << escaped << "\"}";
					break;
				}
				}
				++fWritten;
			}
			ss << "]}" << (sIdx + 1 < obj.scripts.size() ? "," : "");
		}
		ss << "]\n";

		ss << "    }" << (i < gameObjects_.size() - 1 ? "," : "") << "\n";
	}

	ss << "  ],\n";

	// Mapchip Data
	ss << "  \"mapchip\": {\n";
	ss << "    \"rows\": " << mapRows_ << ",\n";
	ss << "    \"cols\": " << mapCols_ << ",\n";
	ss << "    \"tileSize\": " << mapTileSize_ << ",\n";
	ss << "    \"data\": [\n";
	for (int r = 0; r < mapRows_; ++r) {
		ss << "      [";
		for (int c = 0; c < mapCols_; ++c) {
			ss << mapGrid_[r][c] << (c < mapCols_ - 1 ? ", " : "");
		}
		ss << "]" << (r < mapRows_ - 1 ? "," : "") << "\n";
	}
	ss << "    ]\n";
	ss << "  }\n";

	ss << "}\n";
	return ss.str();
}

bool EditorSuite::DeserializeSceneFromString(const std::string& content)
{
	if (content.empty()) return false;

	// Destroy any active script instances before clearing scene objects
	RefreshAllScriptObjectViews();
	const bool inPlay = (sceneState_ == EditorSceneState::Play || sceneState_ == EditorSceneState::Pause || IsStandaloneMode());
	for (auto& obj : gameObjects_) {
		for (auto& sc : obj.scripts) {
			if (sc.instance) {
				if (inPlay) {
					sc.instance->SetOwner(GetOrUpdateScriptViewById(obj.id), obj.id);
					sc.instance->SetEngineAPI(&engineApi_);
					sc.instance->SetHost(this, this);
					sc.instance->OnDestroy();
				}
				sc.instance.reset();
			}
		}
	}
	gameObjects_.clear();
	pendingSpawns_.clear();

	// Read sceneName with fallback to SampleScene
	size_t namePos = content.find("\"sceneName\": \"");
	if (namePos != std::string::npos) {
		size_t endPos = content.find("\"", namePos + 14);
		if (endPos != std::string::npos) {
			currentSceneName_ = content.substr(namePos + 14, endPos - (namePos + 14));
		}
	} else {
		currentSceneName_ = "SampleScene";
	}

	// Helper lambdas for robust fallback parsing
	auto GetStringField = [](const std::string& block, const std::string& key, std::string& outVal) {
		std::string token = "\"" + key + "\": \"";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			size_t ep = block.find("\"", p + token.length());
			if (ep != std::string::npos) outVal = block.substr(p + token.length(), ep - (p + token.length()));
		}
	};

	auto GetBoolField = [](const std::string& block, const std::string& key, bool& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			if (block.substr(p + token.length(), 4) == "true") outVal = true;
			else if (block.substr(p + token.length(), 5) == "false") outVal = false;
		}
	};

	auto GetIntField = [](const std::string& block, const std::string& key, int& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%d", &outVal);
		}
	};

	auto GetFloatField = [](const std::string& block, const std::string& key, float& outVal) {
		std::string token = "\"" + key + "\": ";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f", &outVal);
		}
	};

	auto GetVec2Field = [](const std::string& block, const std::string& key, Vector2& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f", &outVal.x, &outVal.y);
		}
	};

	auto GetVec3Field = [](const std::string& block, const std::string& key, Vector3& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f, %f", &outVal.x, &outVal.y, &outVal.z);
		}
	};

	auto GetVec4Field = [](const std::string& block, const std::string& key, Vector4& outVal) {
		std::string token = "\"" + key + "\": [";
		size_t p = block.find(token);
		if (p != std::string::npos) {
			sscanf_s(block.c_str() + p + token.length(), "%f, %f, %f, %f", &outVal.x, &outVal.y, &outVal.z, &outVal.w);
		}
	};

	// Parse Skybox block
	size_t skyboxPos = content.find("\"skybox\": {");
	if (skyboxPos != std::string::npos) {
		size_t skyboxEnd = content.find("}", skyboxPos);
		if (skyboxEnd != std::string::npos) {
			std::string skyBlock = content.substr(skyboxPos, skyboxEnd - skyboxPos + 1);
			int modeInt = 0;
			GetIntField(skyBlock, "mode", modeInt);
			skybox_.SetMode(static_cast<SkyboxMode>(modeInt));

			Vector4 top = skybox_.GetTopColor();
			GetVec4Field(skyBlock, "topColor", top);
			skybox_.SetTopColor(top);

			Vector4 horiz = skybox_.GetHorizonColor();
			GetVec4Field(skyBlock, "horizonColor", horiz);
			skybox_.SetHorizonColor(horiz);

			Vector4 bot = skybox_.GetBottomColor();
			GetVec4Field(skyBlock, "bottomColor", bot);
			skybox_.SetBottomColor(bot);

			Vector4 solid = skybox_.GetSolidColor();
			GetVec4Field(skyBlock, "solidColor", solid);
			skybox_.SetSolidColor(solid);

			std::string cubemap = skybox_.GetCubemapPath();
			GetStringField(skyBlock, "cubemapPath", cubemap);
			skybox_.SetCubemapPath(cubemap);
			if (skybox_.GetMode() == SkyboxMode::Cubemap) {
				skybox_.LoadCubemap(cubemap);
			}

			float rot = skybox_.GetRotation();
			GetFloatField(skyBlock, "rotation", rot);
			skybox_.SetRotation(rot);

			float exp = skybox_.GetExposure();
			GetFloatField(skyBlock, "exposure", exp);
			skybox_.SetExposure(exp);
		}
	}

	// Parse individual GameObjects
	size_t objStart = content.find("    {");
	while (objStart != std::string::npos) {
		size_t objEnd = content.find("\n    }", objStart);
		if (objEnd != std::string::npos) {
			objEnd += 1; // Advance past '\n' to "    }"
		} else {
			objEnd = content.find("    }", objStart);
		}
		if (objEnd == std::string::npos) break;

		std::string block = content.substr(objStart, objEnd - objStart);

		// Default-initialized GameObject guarantees safe fallback for any missing field
		GameObject obj;
		obj.id = nextObjectId_++;

		int typeInt = 0;
		GetIntField(block, "type", typeInt);
		obj.type = static_cast<GameObjectType>(typeInt);

		GetStringField(block, "name", obj.name);
		GetBoolField(block, "isActive", obj.isActive);
		GetBoolField(block, "isStatic", obj.isStatic);
		GetStringField(block, "tag", obj.tag);

		int layerInt = 0;
		GetIntField(block, "layer", layerInt);
		obj.layer = static_cast<uint32_t>(layerInt);

		GetIntField(block, "parentId", obj.parentId);

		// Transform
		GetVec3Field(block, "translation", obj.translation);
		GetVec3Field(block, "rotation", obj.rotation);
		GetVec3Field(block, "scale", obj.scale);

		Vector4 qVec = { 0.0f, 0.0f, 0.0f, 1.0f };
		GetVec4Field(block, "rotationQuat", qVec);
		obj.rotationQuat = { qVec.x, qVec.y, qVec.z, qVec.w };
		if (obj.rotationQuat.x == 0.0f && obj.rotationQuat.y == 0.0f && obj.rotationQuat.z == 0.0f && obj.rotationQuat.w == 0.0f) {
			obj.rotationQuat = Quaternion::FromEuler(obj.rotation);
		}

		// Mesh
		GetBoolField(block, "hasMesh", obj.hasMesh);
		GetBoolField(block, "meshEnabled", obj.meshEnabled);
		int meshTypeInt = 0;
		GetIntField(block, "meshType", meshTypeInt);
		obj.meshType = static_cast<GameObject::MeshType>(meshTypeInt);
		GetVec4Field(block, "materialColor", obj.materialColor);
		GetBoolField(block, "isLit", obj.isLit);
		GetIntField(block, "lightingType", obj.lightingType);
		GetFloatField(block, "shininess", obj.shininess);
		GetVec3Field(block, "specularColor", obj.specularColor);
		GetBoolField(block, "castShadow", obj.castShadow);
		GetBoolField(block, "receiveShadow", obj.receiveShadow);
		GetBoolField(block, "useGeometryShader", obj.useGeometryShader);
		GetIntField(block, "geometryShaderEffect", obj.geometryShaderEffect);
		GetFloatField(block, "explodeAmount", obj.explodeAmount);
		if (obj.geometryShaderEffect == 0 && obj.useGeometryShader) {
			obj.geometryShaderEffect = 1;
		}
		GetStringField(block, "texturePath", obj.texturePath);
		if (!obj.texturePath.empty()) {
			obj.textureHandle = TextureManager::GetInstance()->LoadTexture(ResolveAssetPath(obj.texturePath));
		}
		GetStringField(block, "modelPath", obj.modelPath);
		GetVec2Field(block, "uvOffset", obj.uvOffset);
		GetVec2Field(block, "uvTiling", obj.uvTiling);
		GetFloatField(block, "uvRotation", obj.uvRotation);

		// Box Collider
		GetBoolField(block, "hasBoxCollider", obj.hasBoxCollider);
		GetBoolField(block, "boxColliderEnabled", obj.boxColliderEnabled);
		GetBoolField(block, "isTrigger", obj.isTrigger);
		GetBoolField(block, "providesContacts", obj.providesContacts);
		GetVec3Field(block, "boxCenter", obj.boxCenter);
		GetVec3Field(block, "boxSize", obj.boxSize);

		// Sphere Collider
		GetBoolField(block, "hasSphereCollider", obj.hasSphereCollider);
		GetBoolField(block, "sphereColliderEnabled", obj.sphereColliderEnabled);
		GetBoolField(block, "sphereIsTrigger", obj.sphereIsTrigger);
		GetVec3Field(block, "sphereCenter", obj.sphereCenter);
		GetFloatField(block, "sphereRadius", obj.sphereRadius);

		// Rigidbody
		GetBoolField(block, "hasRigidbody", obj.hasRigidbody);
		GetBoolField(block, "rbEnabled", obj.rbEnabled);
		GetFloatField(block, "mass", obj.mass);
		GetFloatField(block, "drag", obj.drag);
		GetFloatField(block, "gravityScale", obj.gravityScale);
		GetBoolField(block, "useGravity", obj.useGravity);
		GetBoolField(block, "isKinematic", obj.isKinematic);
		GetFloatField(block, "bounciness", obj.bounciness);

		// Camera
		GetBoolField(block, "hasCamera", obj.hasCamera);
		GetBoolField(block, "cameraEnabled", obj.cameraEnabled);
		GetFloatField(block, "fov", obj.fov);
		GetFloatField(block, "nearClip", obj.nearClip);
		GetFloatField(block, "farClip", obj.farClip);
		GetBoolField(block, "isOrthographic", obj.isOrthographic);

		// Light
		GetBoolField(block, "hasLight", obj.hasLight);
		GetBoolField(block, "lightEnabled", obj.lightEnabled);
		int lightTypeInt = 0;
		GetIntField(block, "lightType", lightTypeInt);
		obj.lightType = static_cast<GameObject::LightType>(lightTypeInt);
		GetVec3Field(block, "lightColor", obj.lightColor);
		GetFloatField(block, "lightIntensity", obj.lightIntensity);
		GetVec3Field(block, "lightDirection", obj.lightDirection);
		GetFloatField(block, "lightRange", obj.lightRange);
		GetFloatField(block, "lightDecay", obj.lightDecay);
		GetFloatField(block, "lightSpotAngle", obj.lightSpotAngle);
		GetFloatField(block, "lightSpotFalloffStart", obj.lightSpotFalloffStart);
		GetBoolField(block, "lightCastShadow", obj.lightCastShadow);
		GetFloatField(block, "lightShadowBias", obj.lightShadowBias);
		GetFloatField(block, "lightShadowStrength", obj.lightShadowStrength);
		GetFloatField(block, "lightShadowNormalBias", obj.lightShadowNormalBias);
		if (obj.hasLight && (obj.lightType == GameObject::LightType::Directional || obj.lightType == GameObject::LightType::Spot)) {
			if (std::abs(obj.rotation.x) < 0.0001f && std::abs(obj.rotation.y) < 0.0001f && std::abs(obj.rotation.z) < 0.0001f) {
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
		}

		// Particle System
		GetBoolField(block, "hasParticleSystem", obj.hasParticleSystem);
		GetIntField(block, "attachedParentId", obj.attachedParentId);
		GetVec3Field(block, "localOffset", obj.localOffset);
		GetIntField(block, "particlePreset", obj.particlePreset);
		GetIntField(block, "particleShape", obj.particleShape);
		GetFloatField(block, "particleSpawnRadius", obj.particleSpawnRadius);
		GetVec3Field(block, "particleBoxExtents", obj.particleBoxExtents);
		GetFloatField(block, "particleConeAngle", obj.particleConeAngle);
		GetFloatField(block, "particleSpawnRate", obj.particleSpawnRate);
		GetIntField(block, "particleMaxParticles", obj.particleMaxParticles);
		GetIntField(block, "particleBurstCount", obj.particleBurstCount);
		GetVec3Field(block, "particleDirection", obj.particleDirection);
		GetFloatField(block, "particleSpreadAngle", obj.particleSpreadAngle);
		GetFloatField(block, "particleSpeedMin", obj.particleSpeedMin);
		GetFloatField(block, "particleSpeedMax", obj.particleSpeedMax);
		GetFloatField(block, "particleGravity", obj.particleGravity);
		GetFloatField(block, "particleDrag", obj.particleDrag);
		GetBoolField(block, "particleWindEnabled", obj.particleWindEnabled);
		GetVec3Field(block, "particleWindAcceleration", obj.particleWindAcceleration);
		GetFloatField(block, "particleLifetimeMin", obj.particleLifetimeMin);
		GetFloatField(block, "particleLifetimeMax", obj.particleLifetimeMax);
		GetFloatField(block, "particleStartScale", obj.particleStartScale);
		GetFloatField(block, "particleEndScale", obj.particleEndScale);
		GetVec4Field(block, "particleStartColor", obj.particleStartColor);
		GetVec4Field(block, "particleEndColor", obj.particleEndColor);
		GetIntField(block, "particleBlendMode", obj.particleBlendMode);
		GetIntField(block, "particleBillboardMode", obj.particleBillboardMode);

		// Mapchip, Audio, Animation & Scripts
		GetBoolField(block, "hasMapchip", obj.hasMapchip);
		GetBoolField(block, "hasAudioSource", obj.hasAudioSource);
		GetStringField(block, "audioClipName", obj.audioClipName);
		GetFloatField(block, "audioVolume", obj.audioVolume);
		GetBoolField(block, "audioPlayOnAwake", obj.audioPlayOnAwake);
		GetBoolField(block, "audioLoop", obj.audioLoop);
		GetIntField(block, "animationIndex", obj.animationIndex);
		GetFloatField(block, "animationSpeed", obj.animationSpeed);
		GetBoolField(block, "animationLoop", obj.animationLoop);
		GetBoolField(block, "animationPlaying", obj.animationPlaying);
		GetBoolField(block, "hasScript", obj.hasScript);
		GetStringField(block, "scriptClassName", obj.scriptClassName);

		bool parsedScriptComponents = false;
		size_t scPos = block.find("\"scriptComponents\": [");
		if (scPos != std::string::npos) {
			parsedScriptComponents = true;
			size_t cursor = scPos + 21;
			while (cursor < block.size()) {
				size_t clsTok = block.find("{\"className\":\"", cursor);
				if (clsTok == std::string::npos) break;
				size_t clsEnd = block.find("\"", clsTok + 14);
				if (clsEnd == std::string::npos) break;
				std::string clsName = block.substr(clsTok + 14, clsEnd - (clsTok + 14));

				GameObject::ScriptComponent sc(clsName);
				sc.isMissingScript = !scriptModule_.IsRegistered(clsName);
				if (sc.isMissingScript) {
					LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving saved field data.", clsName, obj.name), 1);
				}

				size_t fieldsTok = block.find("\"fields\":[", clsEnd);
				size_t fieldsClose = (fieldsTok != std::string::npos) ? block.find("]}", fieldsTok + 10) : std::string::npos;
				if (fieldsTok != std::string::npos && fieldsClose != std::string::npos) {
					std::string fieldsSub = block.substr(fieldsTok + 10, fieldsClose - (fieldsTok + 10));
					size_t fCur = 0;
					while (fCur < fieldsSub.size()) {
						size_t fOpen = fieldsSub.find("{\"name\":\"", fCur);
						if (fOpen == std::string::npos) break;
						size_t fEnd = fieldsSub.find("}", fOpen);
						if (fEnd == std::string::npos) break;
						std::string fEntry = fieldsSub.substr(fOpen, fEnd - fOpen + 1);

						size_t nEnd = fEntry.find("\"", 9);
						if (nEnd != std::string::npos) {
							std::string fName = fEntry.substr(9, nEnd - 9);
							int fTypeInt = 0;
							size_t tPos = fEntry.find("\"type\":");
							if (tPos != std::string::npos) {
								sscanf_s(fEntry.c_str() + tPos + 7, "%d", &fTypeInt);
							}
							GameObject::ScriptSavedFieldValue fVal{};
							fVal.type = static_cast<ScriptFieldType>(fTypeInt);
							switch (fVal.type) {
							case ScriptFieldType::Float: {
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%f", &fVal.floatVal);
								break;
							}
							case ScriptFieldType::Int: {
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%d", &fVal.intVal);
								break;
							}
							case ScriptFieldType::Bool: {
								int bInt = 0;
								size_t v0Pos = fEntry.find("\"v0\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%d", &bInt);
								fVal.boolVal = (bInt != 0);
								break;
							}
							case ScriptFieldType::Vec3: {
								size_t v0Pos = fEntry.find("\"v0\":");
								size_t v1Pos = fEntry.find("\"v1\":");
								size_t v2Pos = fEntry.find("\"v2\":");
								if (v0Pos != std::string::npos) sscanf_s(fEntry.c_str() + v0Pos + 5, "%f", &fVal.vec3Val.x);
								if (v1Pos != std::string::npos) sscanf_s(fEntry.c_str() + v1Pos + 5, "%f", &fVal.vec3Val.y);
								if (v2Pos != std::string::npos) sscanf_s(fEntry.c_str() + v2Pos + 5, "%f", &fVal.vec3Val.z);
								break;
							}
							case ScriptFieldType::CharBuffer: {
								size_t sPos = fEntry.find("\"str\":\"");
								if (sPos != std::string::npos) {
									std::string unesc;
									for (size_t si = sPos + 7; si < fEntry.size(); ++si) {
										if (fEntry[si] == '\\' && si + 1 < fEntry.size()) {
											unesc.push_back(fEntry[++si]);
										} else if (fEntry[si] == '\"') {
											break;
										} else {
											unesc.push_back(fEntry[si]);
										}
									}
									fVal.strVal = unesc;
								}
								break;
							}
							}
							sc.savedFields[fName] = fVal;
						}
						fCur = fEnd + 1;
					}
					cursor = fieldsClose + 2;
				} else {
					cursor = clsEnd + 1;
				}

				if (!clsName.empty()) {
					obj.scripts.push_back(std::move(sc));
				}
			}
		}

		if (!parsedScriptComponents) {
			size_t scriptsArrPos = block.find("\"scripts\": [");
			if (scriptsArrPos != std::string::npos) {
				size_t scriptsArrEnd = block.find("]", scriptsArrPos + 12);
				if (scriptsArrEnd != std::string::npos) {
					std::string arrContent = block.substr(scriptsArrPos + 12, scriptsArrEnd - (scriptsArrPos + 12));
					size_t qStart = arrContent.find("\"");
					while (qStart != std::string::npos) {
						size_t qEnd = arrContent.find("\"", qStart + 1);
						if (qEnd == std::string::npos) break;
						std::string clsName = arrContent.substr(qStart + 1, qEnd - (qStart + 1));
						if (!clsName.empty()) {
							GameObject::ScriptComponent sc(clsName);
							sc.isMissingScript = !scriptModule_.IsRegistered(clsName);
							if (sc.isMissingScript) {
								LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving script name on save.", clsName, obj.name), 1);
							}
							obj.scripts.push_back(std::move(sc));
						}
						qStart = arrContent.find("\"", qEnd + 1);
					}
				}
			} else if (obj.hasScript && !obj.scriptClassName.empty()) {
				GameObject::ScriptComponent sc(obj.scriptClassName);
				sc.isMissingScript = !scriptModule_.IsRegistered(obj.scriptClassName);
				if (sc.isMissingScript) {
					LogMessage(std::format("[Script] Warning: Missing Script '{}' on GameObject '{}'. Preserving script name on save.", obj.scriptClassName, obj.name), 1);
				}
				obj.scripts.push_back(std::move(sc));
			}
		}
		obj.hasScript = !obj.scripts.empty();
		if (!obj.scripts.empty()) {
			obj.scriptClassName = obj.scripts.front().className;
		}

		if (obj.hasParticleSystem) {
			EnsureParticleQuadInitialized(obj);
		}

		gameObjects_.push_back(std::move(obj));
		objStart = content.find("    {", objEnd);
	}

	// Sync parentIndex, childrenIds, and attachToIndex for loaded objects
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		gameObjects_[i].childrenIds.clear();
		gameObjects_[i].parentIndex = -1;
		gameObjects_[i].attachToIndex = -1;
	}
	for (size_t i = 0; i < gameObjects_.size(); ++i) {
		if (gameObjects_[i].parentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].parentId)) {
					gameObjects_[i].parentIndex = static_cast<int>(p);
					gameObjects_[p].childrenIds.push_back(gameObjects_[i].id);
					break;
				}
			}
		}
		if (gameObjects_[i].attachedParentId != -1) {
			for (size_t p = 0; p < gameObjects_.size(); ++p) {
				if (gameObjects_[p].id == static_cast<uint32_t>(gameObjects_[i].attachedParentId)) {
					gameObjects_[i].attachToIndex = static_cast<int>(p);
					break;
				}
			}
		}
	}

	// Parse Mapchip section if present
	size_t mapPos = content.find("\"mapchip\": {");
	if (mapPos != std::string::npos) {
		std::string mapBlock = content.substr(mapPos);
		GetIntField(mapBlock, "rows", mapRows_);
		GetIntField(mapBlock, "cols", mapCols_);
		GetFloatField(mapBlock, "tileSize", mapTileSize_);

		size_t dataPos = mapBlock.find("\"data\": [");
		if (dataPos != std::string::npos) {
			mapGrid_.assign(mapRows_, std::vector<int>(mapCols_, 0));
			size_t rowStart = mapBlock.find("[", dataPos + 9);
			for (int r = 0; r < mapRows_ && rowStart != std::string::npos; ++r) {
				size_t rowEnd = mapBlock.find("]", rowStart);
				if (rowEnd == std::string::npos) break;
				std::string rowStr = mapBlock.substr(rowStart + 1, rowEnd - (rowStart + 1));
				std::stringstream rss(rowStr);
				std::string cell;
				int c = 0;
				while (std::getline(rss, cell, ',') && c < mapCols_) {
					try { mapGrid_[r][c++] = std::stoi(cell); } catch (...) {}
				}
				rowStart = mapBlock.find("[", rowEnd);
			}
		}
	}

	EnsureEditModeScriptInstances();
	selectedObjectIndex_ = gameObjects_.empty() ? -1 : 0;
	return true;
}

bool EditorSuite::SaveScene(const std::string& filePath)
{
	std::string targetPath = filePath.empty() ? currentScenePath_ : filePath;
	if (targetPath.empty()) {
		showSaveSceneAsModal_ = true;
		return false;
	}

	std::filesystem::path p(targetPath);
	if (p.has_parent_path()) {
		std::filesystem::create_directories(p.parent_path());
	}

	std::ofstream file(targetPath);
	if (!file.is_open()) return false;

	file << SerializeSceneToString();
	file.close();

	isSceneDirty_ = false;
	currentScenePath_ = targetPath;
	currentSceneName_ = SceneNameFromPath(p);
	LogMessage(std::format("[Scene] Saved scene to: {}", targetPath), 0);
	return true;
}

bool EditorSuite::LoadScene(const std::string& filePath)
{
	if (sceneState_ != EditorSceneState::Edit && !isStandaloneGameMode_) {
		StopPlayMode();
	}

	std::ifstream file(filePath);
	if (!file.is_open()) return false;

	std::stringstream buffer;
	buffer << file.rdbuf();
	file.close();

	if (!DeserializeSceneFromString(buffer.str())) return false;

	currentScenePath_ = filePath;
	currentSceneName_ = SceneNameFromPath(filePath);
	isSceneDirty_ = false;

	undoStack_.clear();
	redoStack_.clear();
	PushUndoSnapshot();
	isSceneDirty_ = false;

	LogMessage(std::format("[Scene] Loaded scene from: {} ({} objects)", filePath, gameObjects_.size()), 0);
	return true;
}

void EditorSuite::NewScene()
{
	if (sceneState_ != EditorSceneState::Edit) {
		StopPlayMode();
	}
	pendingDeletes_.clear();
	pendingSpawns_.clear();
	pendingDuplicateIndex_ = -1;
	CollisionManager::GetInstance()->Clear();
	gameObjects_.clear();
	currentSceneName_ = "SampleScene";
	currentScenePath_ = (GetScenesDirectory() / "SampleScene.scene.json").string();
	isSceneDirty_ = false;

	skybox_.SetMode(SkyboxMode::Gradient);
	skybox_.SetTopColor({ 0.22f, 0.45f, 0.78f, 1.0f });
	skybox_.SetHorizonColor({ 0.72f, 0.82f, 0.92f, 1.0f });
	skybox_.SetBottomColor({ 0.35f, 0.37f, 0.40f, 1.0f });
	skybox_.SetSolidColor({ 0.157f, 0.157f, 0.157f, 1.0f });
	skybox_.SetCubemapPath("Resources/skybox.dds");
	skybox_.SetRotation(0.0f);
	skybox_.SetExposure(1.0f);

	// 1. Main Camera
	{
		auto& cam = CreateGameObject("Main Camera", GameObjectType::Camera);
		cam.tag = "MainCamera";
		cam.translation = gameCamera_ ? gameCamera_->translation : Vector3{ 0.0f, 1.5f, -8.0f };
		cam.rotation = gameCamera_ ? gameCamera_->rotation : Vector3{ 0.15f, 0.0f, 0.0f };
		cam.hasCamera = true;
		cam.fov = 45.0f;
		cam.nearClip = 0.1f;
		cam.farClip = 1000.0f;
	}

	// 2. Directional Light
	{
		auto& light = CreateGameObject("Directional Light", GameObjectType::DirectionalLight);
		light.translation = { 0.0f, 10.0f, 0.0f };
		light.hasLight = true;
		light.lightType = GameObject::LightType::Directional;
		light.lightDirection = { 0.5f, -1.0f, 0.5f };
		{
			Vector3 d = light.lightDirection;
			float dLen = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
			if (dLen > 0.0001f) { d.x /= dLen; d.y /= dLen; d.z /= dLen; }
			light.rotation = { std::asin(std::clamp(-d.y, -1.0f, 1.0f)), std::atan2(d.x, d.z), 0.0f };
			light.rotationQuat = Quaternion::FromEuler(light.rotation);
		}
		light.lightColor = { 1.0f, 0.95f, 0.9f };
		light.lightIntensity = 1.0f;
		light.lightCastShadow = true;
		light.lightShadowBias = 0.002f;
		light.lightShadowStrength = 1.0f;
		light.lightShadowNormalBias = 0.02f;
	}

	// 3. Ground Plane
	{
		auto& plane = CreatePlane("Ground Plane", { 0.0f, -0.5f, 0.0f });
		plane.scale = { 20.0f, 1.0f, 20.0f };
		plane.isStatic = true;
		plane.materialColor = { 0.25f, 0.27f, 0.30f, 1.0f };
		plane.castShadow = false;
		plane.receiveShadow = true;
	}

	selectedObjectIndex_ = 2; // Ground Plane
	undoStack_.clear();
	redoStack_.clear();
	PushUndoSnapshot();
	isSceneDirty_ = false;

	LogMessage("[Scene] Created Clean Default Scene (SampleScene).", 0);
}

void EditorSuite::DiscardUnsavedChanges()
{
	if (std::filesystem::exists(currentScenePath_)) {
		LoadScene(currentScenePath_);
	} else {
		NewScene();
	}
	LogMessage("[Scene] Discarded unsaved changes.", 0);
}

void EditorSuite::PushUndoSnapshot()
{
	std::string stateJson = SerializeSceneToString();
	if (!undoStack_.empty() && undoStack_.back() == stateJson) {
		return;
	}

	undoStack_.push_back(stateJson);
	if (undoStack_.size() > maxUndoStates_) {
		undoStack_.erase(undoStack_.begin());
	}
	redoStack_.clear();
}

void EditorSuite::Undo()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput) return; // Must not run while text field is active
#endif
	if (sceneState_ == EditorSceneState::Play) return; // Disabled in Play mode

	if (undoStack_.size() > 1) {
		redoStack_.push_back(undoStack_.back());
		undoStack_.pop_back();

		DeserializeSceneFromString(undoStack_.back());
		isSceneDirty_ = true;
		LogMessage("[Undo] Reverted to previous scene state.", 0);
	}
}

void EditorSuite::Redo()
{
#ifdef USE_IMGUI
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantTextInput) return;
#endif
	if (sceneState_ == EditorSceneState::Play) return;

	if (!redoStack_.empty()) {
		std::string nextState = redoStack_.back();
		redoStack_.pop_back();
		undoStack_.push_back(nextState);

		DeserializeSceneFromString(nextState);
		isSceneDirty_ = true;
		LogMessage("[Redo] Restored undone scene state.", 0);
	}
}

