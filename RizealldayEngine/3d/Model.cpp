#include "3d/Model.h"
#include "base/DirectXCommon.h"
#include "base/TextureManager.h"
#include "base/Log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

DirectXCommon* Model::sDxCommon_ = nullptr;
std::unordered_map<std::string, std::unique_ptr<Model>> Model::sCache_;

void Model::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

Model* Model::CreateFromFile(const std::string& directoryPath, const std::string& filename) {
    std::string fullPath = directoryPath + "/" + filename;
    
    auto it = sCache_.find(fullPath);
    if (it != sCache_.end()) {
        return it->second.get();
    }
    
    std::unique_ptr<Model> model = std::make_unique<Model>();
    
    size_t extPos = filename.find_last_of('.');
    std::string ext;
    if (extPos != std::string::npos) {
        ext = filename.substr(extPos + 1);
    }
    
    // convert ext to lowercase
    for (auto& c : ext) {
        c = static_cast<char>(std::tolower(c));
    }
    
    if (ext == "obj") {
        model->LoadOBJ(directoryPath, filename);
    } else {
        model->LoadAssimpModel(fullPath);
    }
    
    model->CreateGPUBuffers();
    
    Model* rawPtr = model.get();
    sCache_[fullPath] = std::move(model);
    return rawPtr;
}

static inline Matrix4x4 AiToRizeMatrix(const aiMatrix4x4& ai) {
    Matrix4x4 m;
    m.m[0][0] = ai.a1; m.m[0][1] = ai.b1; m.m[0][2] = ai.c1; m.m[0][3] = ai.d1;
    m.m[1][0] = ai.a2; m.m[1][1] = ai.b2; m.m[1][2] = ai.c2; m.m[1][3] = ai.d2;
    m.m[2][0] = ai.a3; m.m[2][1] = ai.b3; m.m[2][2] = ai.c3; m.m[2][3] = ai.d3;
    m.m[3][0] = ai.a4; m.m[3][1] = ai.b4; m.m[3][2] = ai.c4; m.m[3][3] = ai.d4;
    return m;
}

void Model::LoadAssimpModel(const std::string& filePath) {
    // Resolve path if not found directly
    std::string actualPath = filePath;
    if (!std::filesystem::exists(actualPath)) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::filesystem::path pExe(exePath);
        std::filesystem::path candidate = pExe.parent_path() / filePath;
        if (std::filesystem::exists(candidate)) {
            actualPath = candidate.string();
        } else {
            candidate = pExe.parent_path().parent_path() / "RizealldaySandbox" / filePath;
            if (std::filesystem::exists(candidate)) {
                actualPath = candidate.string();
            }
        }
    }

    Log::GetInstance()->logOutput("Loading model: " + actualPath + "\n");

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(actualPath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights);

    
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::string err = importer.GetErrorString();
        Log::GetInstance()->logOutput("Assimp Load Error: " + err + "\n");
        return;
    }

    // 1. Build Node Hierarchy
    rootNode_ = BuildNodeHierarchy(scene->mRootNode);
    globalInverseTransform_ = Mathematics::Inverse(AiToRizeMatrix(scene->mRootNode->mTransformation));

    // 2. Load Meshes & Bones
    std::string directory = actualPath.substr(0, actualPath.find_last_of('/'));
    if (directory == actualPath) directory = actualPath.substr(0, actualPath.find_last_of('\\'));
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        if (!mesh || mesh->mNumVertices == 0 || mesh->mNumFaces == 0) {
            continue;
        }

        MeshData meshData;
        meshData.vertices.resize(mesh->mNumVertices);
        
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            VertexData& vertex = meshData.vertices[v];
            vertex.position = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z, 1.0f };
            if (mesh->mNormals) {
                vertex.normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
            } else {
                vertex.normal = { 0.0f, 1.0f, 0.0f };
            }
            if (mesh->mTextureCoords[0]) {
                vertex.texcoord = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
            } else {
                vertex.texcoord = { 0.0f, 0.0f };
            }
            vertex.boneIndices[0] = vertex.boneIndices[1] = vertex.boneIndices[2] = vertex.boneIndices[3] = 0;
            vertex.boneWeights[0] = vertex.boneWeights[1] = vertex.boneWeights[2] = vertex.boneWeights[3] = 0.0f;
        }

        // Read bones
        if (mesh->mNumBones > 0) {
            std::vector<std::vector<std::pair<uint32_t, float>>> vertexBones(mesh->mNumVertices);
            for (unsigned int b = 0; b < mesh->mNumBones; b++) {
                aiBone* bone = mesh->mBones[b];
                std::string boneName = bone->mName.C_Str();

                uint32_t boneId = 0;
                auto it = boneInfoMap_.find(boneName);
                if (it == boneInfoMap_.end()) {
                    boneId = static_cast<uint32_t>(boneList_.size());
                    BoneInfo info;
                    info.name = boneName;
                    info.id = boneId;
                    info.offsetMatrix = AiToRizeMatrix(bone->mOffsetMatrix);
                    boneInfoMap_[boneName] = info;
                    boneList_.push_back(info);
                } else {
                    boneId = it->second.id;
                }

                for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                    uint32_t vId = bone->mWeights[w].mVertexId;
                    float weight = bone->mWeights[w].mWeight;
                    if (vId < mesh->mNumVertices && weight > 0.0001f) {
                        vertexBones[vId].push_back({ boneId, weight });
                    }
                }
            }

            for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
                auto& bList = vertexBones[v];
                std::sort(bList.begin(), bList.end(), [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
                float totalWeight = 0.0f;
                for (size_t k = 0; k < 4 && k < bList.size(); k++) {
                    meshData.vertices[v].boneIndices[k] = bList[k].first;
                    meshData.vertices[v].boneWeights[k] = bList[k].second;
                    totalWeight += bList[k].second;
                }
                if (totalWeight > 0.0001f) {
                    for (size_t k = 0; k < 4; k++) {
                        meshData.vertices[v].boneWeights[k] /= totalWeight;
                    }
                }
            }
        }
        
        // Indices
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace& face = mesh->mFaces[f];
            for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
                meshData.indices.push_back(face.mIndices[idx]);
            }
        }
        
        // Texture
        if (mesh->mMaterialIndex >= 0 && scene->mMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string rawTex = texPath.C_Str();
                size_t slash = rawTex.find_last_of("\\/");
                std::string filenameOnly = (slash != std::string::npos) ? rawTex.substr(slash + 1) : rawTex;
                std::string texFilePath = directory + "/" + filenameOnly;
                meshData.textureHandle = TextureManager::GetInstance()->LoadTexture(texFilePath);
            }
        }
        if (meshData.textureHandle == 0) {
            meshData.textureHandle = TextureManager::GetInstance()->GetDefaultWhiteTexture();
        }
        
        meshes_.push_back(std::move(meshData));
    }

    // 3. Load Animations
    for (unsigned int a = 0; a < scene->mNumAnimations; a++) {
        aiAnimation* anim = scene->mAnimations[a];
        AnimationClip clip;
        clip.name = anim->mName.C_Str();
        clip.duration = static_cast<float>(anim->mDuration);
        clip.ticksPerSecond = static_cast<float>(anim->mTicksPerSecond != 0.0 ? anim->mTicksPerSecond : 25.0);
        
        for (unsigned int c = 0; c < anim->mNumChannels; c++) {
            aiNodeAnim* channel = anim->mChannels[c];
            NodeAnimTrack track;
            
            track.positions.reserve(channel->mNumPositionKeys);
            for (unsigned int p = 0; p < channel->mNumPositionKeys; p++) {
                KeyVector3 k;
                k.time = static_cast<float>(channel->mPositionKeys[p].mTime);
                k.value = { channel->mPositionKeys[p].mValue.x, channel->mPositionKeys[p].mValue.y, channel->mPositionKeys[p].mValue.z };
                track.positions.push_back(k);
            }
            
            track.rotations.reserve(channel->mNumRotationKeys);
            for (unsigned int r = 0; r < channel->mNumRotationKeys; r++) {
                KeyQuat k;
                k.time = static_cast<float>(channel->mRotationKeys[r].mTime);
                k.x = channel->mRotationKeys[r].mValue.x;
                k.y = channel->mRotationKeys[r].mValue.y;
                k.z = channel->mRotationKeys[r].mValue.z;
                k.w = channel->mRotationKeys[r].mValue.w;
                track.rotations.push_back(k);
            }
            
            track.scalings.reserve(channel->mNumScalingKeys);
            for (unsigned int s = 0; s < channel->mNumScalingKeys; s++) {
                KeyVector3 k;
                k.time = static_cast<float>(channel->mScalingKeys[s].mTime);
                k.value = { channel->mScalingKeys[s].mValue.x, channel->mScalingKeys[s].mValue.y, channel->mScalingKeys[s].mValue.z };
                track.scalings.push_back(k);
            }
            
            clip.channels[channel->mNodeName.C_Str()] = std::move(track);
        }
        animations_.push_back(std::move(clip));
    }

    Log::GetInstance()->logOutput(std::format("Model loaded: {} meshes, {} bones, {} animations.\n",
        meshes_.size(), boneList_.size(), animations_.size()));
}

void Model::LoadOBJ(const std::string& directoryPath, const std::string& filename) {
    MeshData meshData;
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    if (!file.is_open()) {
        Log::GetInstance()->logOutput("Failed to open OBJ file: " + directoryPath + "/" + filename + "\n");
        return;
    }

    while (std::getline(file, line)) {
        std::string identifier;
        std::istringstream s(line);
        s >> identifier;

        if (identifier == "v") {
            Vector4 position;
            s >> position.x >> position.y >> position.z;
            position.w = 1.0f;
            positions.push_back(position);
        }
        else if (identifier == "vt") {
            Vector2 texcoord;
            s >> texcoord.x >> texcoord.y;
            texcoords.push_back({ texcoord.x, 1.0f - texcoord.y });
        }
        else if (identifier == "vn") {
            Vector3 normal;
            s >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (identifier == "f") {
            VertexData triangle[3];
            for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
                std::string vertexDefinition;
                s >> vertexDefinition;

                std::istringstream v(vertexDefinition);
                uint32_t elementIndices[3] = {0, 0, 0};
                for (int32_t element = 0; element < 3; ++element) {
                    std::string index;
                    std::getline(v, index, '/');
                    if (!index.empty()) {
                        elementIndices[element] = std::stoi(index);
                    }
                }
                
                Vector4 position = {0,0,0,1};
                Vector2 texcoord = {0,0};
                Vector3 normal = {0,0,0};
                
                if (elementIndices[0] > 0 && elementIndices[0] <= positions.size()) position = positions[elementIndices[0] - 1];
                if (elementIndices[1] > 0 && elementIndices[1] <= texcoords.size()) texcoord = texcoords[elementIndices[1] - 1];
                if (elementIndices[2] > 0 && elementIndices[2] <= normals.size()) normal = normals[elementIndices[2] - 1];
                
                position.x *= -1.0f;
                normal.x *= -1.0f;
                
                VertexData vData{};
                vData.position = position;
                vData.texcoord = texcoord;
                vData.normal = normal;
                triangle[faceVertex] = vData;
            }
            meshData.vertices.push_back(triangle[2]);
            meshData.vertices.push_back(triangle[1]);
            meshData.vertices.push_back(triangle[0]);
        }
    }
    
    for (uint32_t i = 0; i < meshData.vertices.size(); i++) {
        meshData.indices.push_back(i);
    }
    
    meshData.textureHandle = TextureManager::GetInstance()->GetDefaultWhiteTexture();
    meshes_.push_back(std::move(meshData));
}

void Model::CreateGPUBuffers() {
    for (auto& mesh : meshes_) {
        if (mesh.vertices.empty() || mesh.indices.empty()) {
            continue;
        }

        // Vertex Buffer
        size_t vSizeInBytes = sizeof(VertexData) * mesh.vertices.size();
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = vSizeInBytes;
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        HRESULT hr = sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vertexBuffer));
        if (FAILED(hr)) {
            Log::GetInstance()->logOutput("Failed to create vertex buffer\n");
            continue;
        }
        
        void* mappedVData = nullptr;
        mesh.vertexBuffer->Map(0, nullptr, &mappedVData);
        memcpy(mappedVData, mesh.vertices.data(), vSizeInBytes);
        mesh.vertexBuffer->Unmap(0, nullptr);
        
        mesh.vbView.BufferLocation = mesh.vertexBuffer->GetGPUVirtualAddress();
        mesh.vbView.SizeInBytes = static_cast<UINT>(vSizeInBytes);
        mesh.vbView.StrideInBytes = sizeof(VertexData);
        
        // Index Buffer
        size_t iSizeInBytes = sizeof(uint32_t) * mesh.indices.size();
        desc.Width = iSizeInBytes;
        hr = sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.indexBuffer));
        if (FAILED(hr)) {
            Log::GetInstance()->logOutput("Failed to create index buffer\n");
            continue;
        }
        
        void* mappedIData = nullptr;
        mesh.indexBuffer->Map(0, nullptr, &mappedIData);
        memcpy(mappedIData, mesh.indices.data(), iSizeInBytes);
        mesh.indexBuffer->Unmap(0, nullptr);
        
        mesh.ibView.BufferLocation = mesh.indexBuffer->GetGPUVirtualAddress();
        mesh.ibView.SizeInBytes = static_cast<UINT>(iSizeInBytes);
        mesh.ibView.Format = DXGI_FORMAT_R32_UINT;
    }
}

void Model::Draw(ID3D12GraphicsCommandList* cmdList) {
    for (const auto& mesh : meshes_) {
        if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indices.empty()) {
            continue;
        }

        uint32_t texHandle = mesh.textureHandle;
        if (texHandle == 0) {
            texHandle = TextureManager::GetInstance()->GetDefaultWhiteTexture();
        }
        cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetGPUHandle(texHandle));
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &mesh.vbView);
        cmdList->IASetIndexBuffer(&mesh.ibView);
        cmdList->DrawIndexedInstanced(static_cast<UINT>(mesh.indices.size()), 1, 0, 0, 0);
    }
}

NodeData Model::BuildNodeHierarchy(void* pAiNode) {
    aiNode* node = static_cast<aiNode*>(pAiNode);
    NodeData data;
    data.name = node->mName.C_Str();
    data.transform = AiToRizeMatrix(node->mTransformation);

    aiVector3D s, p;
    aiQuaternion r;
    node->mTransformation.Decompose(s, r, p);
    data.defaultScaling = { s.x, s.y, s.z };
    data.defaultRotation = { 0.0f, r.x, r.y, r.z, r.w };
    data.defaultPosition = { p.x, p.y, p.z };

    data.children.reserve(node->mNumChildren);
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        data.children.push_back(BuildNodeHierarchy(node->mChildren[i]));
    }
    return data;
}

void Model::ReadNodeHierarchy(float animTimeTicks, const AnimationClip* pAnim, const NodeData& node, const Matrix4x4& parentTransform, std::vector<Matrix4x4>& outMatrices) {
    Matrix4x4 nodeTransform = node.transform;

    if (pAnim) {
        auto it = pAnim->channels.find(node.name);
        if (it != pAnim->channels.end()) {
            const NodeAnimTrack& track = it->second;
            Vector3 scaling = InterpolateScaling(animTimeTicks, track, node.defaultScaling);
            KeyQuat rot = InterpolateRotation(animTimeTicks, track, node.defaultRotation);
            Vector3 translation = InterpolatePosition(animTimeTicks, track, node.defaultPosition);

            if (lockRootMotion_ && (node.name.find("Hips") != std::string::npos || node.name.find("Root") != std::string::npos)) {
                if (!track.positions.empty()) {
                    translation.x = track.positions[0].value.x;
                    translation.z = track.positions[0].value.z;
                } else {
                    translation.x = node.defaultPosition.x;
                    translation.z = node.defaultPosition.z;
                }
            }

            aiQuaternion aiRot(rot.w, rot.x, rot.y, rot.z);
            aiMatrix4x4 aiMat(aiVector3D(scaling.x, scaling.y, scaling.z), aiRot, aiVector3D(translation.x, translation.y, translation.z));
            nodeTransform = AiToRizeMatrix(aiMat);

        }
    }

    Matrix4x4 globalTransform = Mathematics::Multiply(nodeTransform, parentTransform);

    auto boneIt = boneInfoMap_.find(node.name);
    if (boneIt != boneInfoMap_.end()) {
        uint32_t boneId = boneIt->second.id;
        if (boneId < outMatrices.size()) {
            Matrix4x4 toRoot = Mathematics::Multiply(globalTransform, globalInverseTransform_);
            outMatrices[boneId] = Mathematics::Multiply(boneIt->second.offsetMatrix, toRoot);
        }
    }


    for (const auto& child : node.children) {
        ReadNodeHierarchy(animTimeTicks, pAnim, child, globalTransform, outMatrices);
    }
}

Vector3 Model::InterpolatePosition(float animTimeTicks, const NodeAnimTrack& track, const Vector3& defaultPos) {
    if (track.positions.empty()) return defaultPos;
    if (track.positions.size() == 1) return track.positions[0].value;

    size_t index = 0;
    for (size_t i = 0; i < track.positions.size() - 1; i++) {
        if (animTimeTicks < track.positions[i + 1].time) {
            index = i;
            break;
        }
    }
    size_t nextIndex = index + 1;
    float t1 = track.positions[index].time;
    float t2 = track.positions[nextIndex].time;
    float factor = (t2 > t1) ? (animTimeTicks - t1) / (t2 - t1) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);
    const Vector3& p1 = track.positions[index].value;
    const Vector3& p2 = track.positions[nextIndex].value;
    return {
        p1.x + factor * (p2.x - p1.x),
        p1.y + factor * (p2.y - p1.y),
        p1.z + factor * (p2.z - p1.z)
    };
}

KeyQuat Model::InterpolateRotation(float animTimeTicks, const NodeAnimTrack& track, const KeyQuat& defaultRot) {
    if (track.rotations.empty()) return defaultRot;
    if (track.rotations.size() == 1) return track.rotations[0];

    size_t index = 0;
    for (size_t i = 0; i < track.rotations.size() - 1; i++) {
        if (animTimeTicks < track.rotations[i + 1].time) {
            index = i;
            break;
        }
    }
    size_t nextIndex = index + 1;
    float t1 = track.rotations[index].time;
    float t2 = track.rotations[nextIndex].time;
    float factor = (t2 > t1) ? (animTimeTicks - t1) / (t2 - t1) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);

    const KeyQuat& r1 = track.rotations[index];
    const KeyQuat& r2 = track.rotations[nextIndex];

    aiQuaternion q1(r1.w, r1.x, r1.y, r1.z);
    aiQuaternion q2(r2.w, r2.x, r2.y, r2.z);
    aiQuaternion qOut;
    aiQuaternion::Interpolate(qOut, q1, q2, factor);
    qOut.Normalize();

    return { 0.0f, qOut.x, qOut.y, qOut.z, qOut.w };
}

Vector3 Model::InterpolateScaling(float animTimeTicks, const NodeAnimTrack& track, const Vector3& defaultScale) {
    if (track.scalings.empty()) return defaultScale;
    if (track.scalings.size() == 1) return track.scalings[0].value;

    size_t index = 0;
    for (size_t i = 0; i < track.scalings.size() - 1; i++) {
        if (animTimeTicks < track.scalings[i + 1].time) {
            index = i;
            break;
        }
    }
    size_t nextIndex = index + 1;
    float t1 = track.scalings[index].time;
    float t2 = track.scalings[nextIndex].time;
    float factor = (t2 > t1) ? (animTimeTicks - t1) / (t2 - t1) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);
    const Vector3& s1 = track.scalings[index].value;
    const Vector3& s2 = track.scalings[nextIndex].value;
    return {
        s1.x + factor * (s2.x - s1.x),
        s1.y + factor * (s2.y - s1.y),
        s1.z + factor * (s2.z - s1.z)
    };
}

void Model::CalculateBoneTransforms(float timeInSeconds, int animIndex, std::vector<Matrix4x4>& outMatrices) {
    outMatrices.assign(128, Mathematics::MakeIdentity4x4());

    if (boneInfoMap_.empty()) {
        return;
    }

    const AnimationClip* pAnim = nullptr;
    float timeInTicks = 0.0f;
    if (animIndex >= 0 && animIndex < static_cast<int>(animations_.size())) {
        pAnim = &animations_[animIndex];
        float tps = pAnim->ticksPerSecond > 0.0f ? pAnim->ticksPerSecond : 25.0f;
        timeInTicks = timeInSeconds * tps;
        if (pAnim->duration > 0.0f) {
            timeInTicks = std::fmod(timeInTicks, pAnim->duration);
        }
    }

    ReadNodeHierarchy(timeInTicks, pAnim, rootNode_, Mathematics::MakeIdentity4x4(), outMatrices);
}

float Model::GetAnimationDuration(int animIndex) const {
    if (animIndex >= 0 && animIndex < static_cast<int>(animations_.size())) {
        float tps = animations_[animIndex].ticksPerSecond > 0.0f ? animations_[animIndex].ticksPerSecond : 25.0f;
        return animations_[animIndex].duration / tps;
    }
    return 0.0f;
}
