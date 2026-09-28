#include "3d/Model.h"
#include "base/DirectXCommon.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cassert>
#include <fstream>
#include <sstream>

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
        c = std::tolower(c);
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

void Model::LoadAssimpModel(const std::string& filePath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices);
    
    assert(scene && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) && scene->mRootNode);
    
    std::string directory = filePath.substr(0, filePath.find_last_of('/'));
    if (directory == filePath) directory = filePath.substr(0, filePath.find_last_of('\\'));
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        MeshData meshData;
        
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            VertexData vertex;
            vertex.position = {mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z, 1.0f};
            if (mesh->mNormals) {
                vertex.normal = {mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z};
            }
            if (mesh->mTextureCoords[0]) {
                vertex.texcoord = {mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y};
            }
            meshData.vertices.push_back(vertex);
        }
        
        for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
            aiFace& face = mesh->mFaces[f];
            for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
                meshData.indices.push_back(face.mIndices[idx]);
            }
        }
        
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
                std::string texFilePath = directory + "/" + texPath.C_Str();
                // We'll handle texture loading separately
            }
        }
        
        meshes_.push_back(std::move(meshData));
    }
}

void Model::LoadOBJ(const std::string& directoryPath, const std::string& filename) {
    MeshData meshData;
    std::vector<Vector4> positions;
    std::vector<Vector3> normals;
    std::vector<Vector2> texcoords;
    std::string line;

    std::ifstream file(directoryPath + "/" + filename);
    assert(file.is_open());

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
                
                triangle[faceVertex] = { position, texcoord, normal };
            }
            meshData.vertices.push_back(triangle[2]);
            meshData.vertices.push_back(triangle[1]);
            meshData.vertices.push_back(triangle[0]);
        }
    }
    
    for (uint32_t i = 0; i < meshData.vertices.size(); i++) {
        meshData.indices.push_back(i);
    }
    
    meshes_.push_back(std::move(meshData));
}

void Model::CreateGPUBuffers() {
    for (auto& mesh : meshes_) {
        // Vertex Buffer
        size_t vSizeInBytes = sizeof(VertexData) * mesh.vertices.size();
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = vSizeInBytes;
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.vertexBuffer));
        
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
        sDxCommon_->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mesh.indexBuffer));
        
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
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &mesh.vbView);
        cmdList->IASetIndexBuffer(&mesh.ibView);
        cmdList->DrawIndexedInstanced(static_cast<UINT>(mesh.indices.size()), 1, 0, 0, 0);
    }
}
