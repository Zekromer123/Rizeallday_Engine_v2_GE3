#include "base/Profiler.h"
#include "base/Log.h"
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <format>

namespace {
	constexpr float kEmaAlpha = 0.1f;

	int64_t QpcNow()
	{
		LARGE_INTEGER value{};
		QueryPerformanceCounter(&value);
		return value.QuadPart;
	}

	bool SameName(const char* a, const char* b)
	{
		return a == b || std::strcmp(a, b) == 0;
	}
}

Profiler* Profiler::GetInstance()
{
	static Profiler instance;
	return &instance;
}

void Profiler::Initialize(DirectXCommon* dx)
{
	dx_ = dx;

	LARGE_INTEGER freq{};
	QueryPerformanceFrequency(&freq);
	qpcFrequency_ = (std::max<int64_t>)(freq.QuadPart, 1);
	lastFrameQpc_ = 0;
	cpuCount_ = 0;
	cpuDepth_ = 0;
	snapshot_.uploadCapacity = DirectXCommon::GetFrameUploadCapacity();
	snapshot_.srvCapacity = DirectXCommon::kMaxSrvDescriptors;

	gpuEnabled_ = false;
	if (!dx_ || !dx_->GetDevice() || !dx_->GetCommandQueue()) {
		return;
	}

	ID3D12Device* device = dx_->GetDevice();
	if (FAILED(dx_->GetCommandQueue()->GetTimestampFrequency(&gpuFrequency_)) || gpuFrequency_ == 0) {
		Log::GetInstance()->logOutput("[Profiler] Timestamp frequency unavailable, GPU profiling disabled.\n");
		return;
	}

	const UINT queryCount = kMaxGpuScopes * 2;
	const UINT64 readbackBytes = static_cast<UINT64>(queryCount) * sizeof(uint64_t);

	D3D12_QUERY_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	heapDesc.Count = queryCount;
	heapDesc.NodeMask = 0;

	D3D12_HEAP_PROPERTIES readbackProps{};
	readbackProps.Type = D3D12_HEAP_TYPE_READBACK;
	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = readbackBytes;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	for (uint32_t i = 0; i < DirectXCommon::kFramesInFlight; ++i) {
		GpuSlot& slot = gpuSlots_[i];
		slot = GpuSlot{};
		HRESULT hr = device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&slot.heap));
		if (FAILED(hr)) {
			Log::GetInstance()->logOutput(std::format("[Profiler] CreateQueryHeap failed (0x{:08X}), GPU profiling disabled.\n", static_cast<uint32_t>(hr)));
			Finalize();
			return;
		}
		hr = device->CreateCommittedResource(&readbackProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&slot.readback));
		if (FAILED(hr)) {
			Log::GetInstance()->logOutput(std::format("[Profiler] Readback buffer creation failed (0x{:08X}), GPU profiling disabled.\n", static_cast<uint32_t>(hr)));
			Finalize();
			return;
		}
	}
	gpuEnabled_ = true;
}

void Profiler::Finalize()
{
	gpuEnabled_ = false;
	frameOpen_ = false;
	for (GpuSlot& slot : gpuSlots_) {
		if (dx_) {
			if (slot.heap) dx_->DeferRelease(slot.heap);
			if (slot.readback) dx_->DeferRelease(slot.readback);
		}
		slot = GpuSlot{};
	}
}

float Profiler::UpdateEma(std::vector<EmaEntry>& table, const char* name, float ms)
{
	for (EmaEntry& entry : table) {
		if (SameName(entry.name, name)) {
			entry.avg += kEmaAlpha * (ms - entry.avg);
			return entry.avg;
		}
	}
	table.push_back({ name, ms });
	return ms;
}

void Profiler::AddResult(std::vector<ProfilerScopeResult>& list, const char* name, uint32_t depth, float ms)
{
	// A scope entered several times per frame (e.g. in a loop) is reported as one summed line
	for (ProfilerScopeResult& r : list) {
		if (r.depth == depth && SameName(r.name, name)) {
			r.ms += ms;
			return;
		}
	}
	ProfilerScopeResult result;
	result.name = name;
	result.depth = depth;
	result.ms = ms;
	list.push_back(result);
}

void Profiler::ReadBackSlot(GpuSlot& slot)
{
	const uint32_t count = slot.resolvedCount;
	if (!slot.readback || count == 0) {
		return;
	}
	const D3D12_RANGE readRange{ 0, static_cast<SIZE_T>(count) * 2 * sizeof(uint64_t) };
	void* mapped = nullptr;
	if (FAILED(slot.readback->Map(0, &readRange, &mapped)) || !mapped) {
		return;
	}
	const uint64_t* stamps = static_cast<const uint64_t*>(mapped);

	snapshot_.gpu.clear();
	for (uint32_t i = 0; i < count; ++i) {
		if (!slot.ended[i]) {
			continue;
		}
		const uint64_t begin = stamps[i * 2];
		const uint64_t end = stamps[i * 2 + 1];
		const float ms = (end > begin) ? static_cast<float>(static_cast<double>(end - begin) * 1000.0 / static_cast<double>(gpuFrequency_)) : 0.0f;
		AddResult(snapshot_.gpu, slot.names[i], slot.depths[i], ms);
	}
	const D3D12_RANGE writtenNothing{ 0, 0 };
	slot.readback->Unmap(0, &writtenNothing);

	for (ProfilerScopeResult& r : snapshot_.gpu) {
		r.avgMs = UpdateEma(gpuEma_, r.name, r.ms);
		if (r.depth == 0 && std::strcmp(r.name, "GPU Frame") == 0) {
			snapshot_.gpuValid = true;
			snapshot_.gpuFrameMs = r.ms;
			snapshot_.gpuFrameAvgMs = r.avgMs;
		}
	}
}

void Profiler::BeginFrame(uint32_t frameSlot)
{
	// 1. GPU: the slot's previous frame has finished (the caller waited on its fence)
	if (gpuEnabled_ && frameSlot < DirectXCommon::kFramesInFlight) {
		GpuSlot& slot = gpuSlots_[frameSlot];
		ReadBackSlot(slot);
		slot.count = 0;
		slot.resolvedCount = 0;
		currentSlot_ = frameSlot;
		gpuDepth_ = 0;
		frameOpen_ = true;
	}

	// 2. CPU: frame time between consecutive BeginFrame calls, then publish the frame that just ended
	const int64_t now = QpcNow();
	if (lastFrameQpc_ != 0) {
		const float ms = static_cast<float>(static_cast<double>(now - lastFrameQpc_) * 1000.0 / static_cast<double>(qpcFrequency_));
		snapshot_.frameMs = ms;
		snapshot_.frameAvgMs = (snapshot_.frameAvgMs <= 0.0f) ? ms : snapshot_.frameAvgMs + kEmaAlpha * (ms - snapshot_.frameAvgMs);
		snapshot_.fps = (snapshot_.frameAvgMs > 0.0f) ? 1000.0f / snapshot_.frameAvgMs : 0.0f;

		snapshot_.frameHistory[snapshot_.frameHistoryOffset] = ms;
		snapshot_.frameHistoryOffset = (snapshot_.frameHistoryOffset + 1) % ProfilerSnapshot::kFrameHistorySize;
		snapshot_.frameHistoryCount = (std::min)(snapshot_.frameHistoryCount + 1, ProfilerSnapshot::kFrameHistorySize);
	}
	lastFrameQpc_ = now;
	FinishCpuFrame();
}

void Profiler::EndFrame(ID3D12GraphicsCommandList* cmdList)
{
	if (dx_) {
		pendingUploadBytes_ = dx_->GetFrameUploadBytesUsed();
		pendingSrvUsed_ = DirectXCommon::kMaxSrvDescriptors - (std::min)(dx_->GetFreeSrvCount(), DirectXCommon::kMaxSrvDescriptors);
	}

	if (!gpuEnabled_ || !frameOpen_ || !cmdList) {
		frameOpen_ = false;
		return;
	}
	GpuSlot& slot = gpuSlots_[currentSlot_];
	// Close scopes the caller forgot to end so that every resolved query holds a written timestamp
	for (uint32_t i = 0; i < slot.count; ++i) {
		if (!slot.ended[i]) {
			cmdList->EndQuery(slot.heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, i * 2 + 1);
			slot.ended[i] = true;
		}
	}
	if (slot.count > 0) {
		cmdList->ResolveQueryData(slot.heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, slot.count * 2, slot.readback.Get(), 0);
	}
	slot.resolvedCount = slot.count;
	frameOpen_ = false;
}

void Profiler::FinishCpuFrame()
{
	const int64_t now = QpcNow();
	snapshot_.cpu.clear();
	for (uint32_t i = 0; i < cpuCount_; ++i) {
		CpuRecord& rec = cpuRecords_[i];
		const int64_t end = rec.ended ? rec.end : now;
		const float ms = static_cast<float>(static_cast<double>(end - rec.begin) * 1000.0 / static_cast<double>(qpcFrequency_));
		AddResult(snapshot_.cpu, rec.name, rec.depth, (std::max)(ms, 0.0f));
	}
	for (ProfilerScopeResult& r : snapshot_.cpu) {
		r.avgMs = UpdateEma(cpuEma_, r.name, r.ms);
	}
	cpuCount_ = 0;
	cpuDepth_ = 0;

	snapshot_.drawCalls = pendingDrawCalls_;
	snapshot_.instances = pendingInstances_;
	snapshot_.uploadBytesUsed = pendingUploadBytes_;
	snapshot_.srvUsed = pendingSrvUsed_;
	pendingDrawCalls_ = 0;
	pendingInstances_ = 0;
}

uint32_t Profiler::BeginCpu(const char* name)
{
	if (cpuCount_ >= kMaxCpuScopes) {
		return kInvalidToken;
	}
	if (qpcFrequency_ == 0) {
		LARGE_INTEGER freq{};
		QueryPerformanceFrequency(&freq);
		qpcFrequency_ = (std::max<int64_t>)(freq.QuadPart, 1);
	}
	const uint32_t token = cpuCount_++;
	CpuRecord& rec = cpuRecords_[token];
	rec.name = name ? name : "";
	rec.depth = cpuDepth_++;
	rec.ended = false;
	rec.end = 0;
	rec.begin = QpcNow();
	return token;
}

void Profiler::EndCpu(uint32_t token)
{
	if (token >= cpuCount_) {
		return;
	}
	CpuRecord& rec = cpuRecords_[token];
	if (rec.ended) {
		return;
	}
	rec.end = QpcNow();
	rec.ended = true;
	cpuDepth_ = rec.depth; // also repairs the depth after a mismatched Begin/End pair
}

uint32_t Profiler::BeginGpu(ID3D12GraphicsCommandList* cmdList, const char* name)
{
	if (!gpuEnabled_ || !frameOpen_ || !cmdList) {
		return kInvalidToken;
	}
	GpuSlot& slot = gpuSlots_[currentSlot_];
	if (slot.count >= kMaxGpuScopes) {
		return kInvalidToken;
	}
	const uint32_t token = slot.count++;
	slot.names[token] = name ? name : "";
	slot.depths[token] = gpuDepth_++;
	slot.ended[token] = false;
	cmdList->EndQuery(slot.heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, token * 2);
	return token;
}

void Profiler::EndGpu(ID3D12GraphicsCommandList* cmdList, uint32_t token)
{
	if (!gpuEnabled_ || !frameOpen_ || !cmdList || token == kInvalidToken) {
		return;
	}
	GpuSlot& slot = gpuSlots_[currentSlot_];
	if (token >= slot.count || slot.ended[token]) {
		return;
	}
	cmdList->EndQuery(slot.heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, token * 2 + 1);
	slot.ended[token] = true;
	gpuDepth_ = slot.depths[token];
}
