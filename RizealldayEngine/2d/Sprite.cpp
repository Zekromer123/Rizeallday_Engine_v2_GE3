#include "2d/Sprite.h"
#include "base/DirectXCommon.h"
#include "3d/DirectionalLight.h"

void Sprite::Initialize(DirectXCommon* dxCommon)
{
	vertexResourceSprite = dxCommon->CreateBufferResource(sizeof(VertexData) * 6);
	this->srvDescriptorHeap = dxCommon->GetSrvDescriptorHeap();
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	indexResourceSprite = dxCommon->CreateBufferResource(sizeof(uint32_t) * 6);
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	VertexData* vertexDataSprite = nullptr;
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	std::memset(vertexDataSprite, 0, sizeof(VertexData) * 6);
	// First triangle
	vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // Bottom-left
	vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	vertexDataSprite[0].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // Top-left
	vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[1].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // Bottom-right
	vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[2].normal = { 0.0f, 0.0f, -1.0f };
	// Second triangle
	vertexDataSprite[3].position = { 0.0f, 0.0f, 0.0f, 1.0f }; // Top-left
	vertexDataSprite[3].texcoord = { 0.0f, 0.0f };
	vertexDataSprite[3].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[4].position = { 640.0f, 0.0f, 0.0f, 1.0f }; // Top-right
	vertexDataSprite[4].texcoord = { 1.0f, 0.0f };
	vertexDataSprite[4].normal = { 0.0f, 0.0f, -1.0f };
	vertexDataSprite[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // Bottom-right
	vertexDataSprite[5].texcoord = { 1.0f, 1.0f };
	vertexDataSprite[5].normal = { 0.0f, 0.0f, -1.0f };

	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 4;
	indexDataSprite[5] = 2;

	// Lighting
	materialResourceSprite = dxCommon->CreateBufferResource(sizeof(Material));
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	materialDataSprite->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialDataSprite->enableLighting = 0;
	materialDataSprite->lightingType = 0;

	directionalLightResource = dxCommon->CreateBufferResource(sizeof(DirectionalLightData));
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;
	directionalLightData->active = 1;

	transformationMatrixResourceSprite = dxCommon->CreateBufferResource((sizeof(TransformationMatrix) + 255) & ~255);
	transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));
	transformationMatrixDataSprite->WVP = Mathematics::MakeIdentity4x4();
	transformationMatrixDataSprite->World = Mathematics::MakeIdentity4x4();
}

void Sprite::Update()
{
	Matrix4x4 worldMatrixSprite = Mathematics::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	Matrix4x4 viewMatrixSprite = Mathematics::MakeIdentity4x4();
	Matrix4x4 projectionMatrixSprite = Mathematics::MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
	Matrix4x4 worldViewProjectionMatrixSprite = Mathematics::Multiply(worldMatrixSprite, Mathematics::Multiply(viewMatrixSprite, projectionMatrixSprite));
	transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
	transformationMatrixDataSprite->World = worldMatrixSprite;
}

void Sprite::Draw(ID3D12GraphicsCommandList* commandList)
{
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPUSprite);
	commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
	commandList->IASetIndexBuffer(&indexBufferViewSprite);

	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
