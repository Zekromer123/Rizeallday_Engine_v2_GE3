#pragma once
// Lightweight CPU/GPU profiler. Compiled in every configuration (cheap); only the editor panel is ImGui-only.
//
// CPU scopes   : QueryPerformanceCounter, nestable, published once per frame (see FinishCpuFrame).
// GPU scopes   : D3D12 timestamp queries, one query heap + readback buffer per in-flight frame slot. The results of
//                a frame are read back when its slot is reused, i.e. they are kFramesInFlight frames old.
// Counters     : draw calls / instances, per-frame upload memory, SRV descriptor usage, frame-time history.
//
// All calls must come from the main (render) thread. Scope names must be string literals (only the pointer is stored).
#include "base/DirectXCommon.h"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <wrl.h>

struct ProfilerScopeResult {
	const char* name = "";
	uint32_t depth = 0;   ///< Nesting depth (0 = top level), used for indentation
	float ms = 0.0f;      ///< Duration in the last completed frame (same-named scopes of equal depth are summed)
	float avgMs = 0.0f;   ///< Exponential moving average (alpha = 0.1)
};

struct ProfilerSnapshot {
	static constexpr uint32_t kFrameHistorySize = 240;

	std::vector<ProfilerScopeResult> cpu;
	std::vector<ProfilerScopeResult> gpu;

	float frameMs = 0.0f;       ///< CPU time between the last two BeginFrame calls
	float frameAvgMs = 0.0f;    ///< EMA of frameMs
	float fps = 0.0f;           ///< 1000 / frameAvgMs
	bool gpuValid = false;      ///< true once a "GPU Frame" result has been read back
	float gpuFrameMs = 0.0f;    ///< Last "GPU Frame" scope
	float gpuFrameAvgMs = 0.0f; ///< EMA of gpuFrameMs

	uint32_t drawCalls = 0;
	uint32_t instances = 0;
	size_t uploadBytesUsed = 0;
	size_t uploadCapacity = 0;
	uint32_t srvUsed = 0;       ///< kMaxSrvDescriptors - free descriptors (fixed slots count as used)
	uint32_t srvCapacity = 0;

	// Ring buffer of frame times (ms). The oldest sample is at frameHistoryOffset (ready for ImGui::PlotLines).
	float frameHistory[kFrameHistorySize] = {};
	uint32_t frameHistoryOffset = 0;
	uint32_t frameHistoryCount = 0;
};

class Profiler
{
public:
	static constexpr uint32_t kMaxGpuScopes = 32;
	static constexpr uint32_t kMaxCpuScopes = 256;
	static constexpr uint32_t kInvalidToken = 0xFFFFFFFFu;

	static Profiler* GetInstance();

	/// Needs the device + queue. If the query heaps cannot be created the GPU part is disabled (CPU part keeps working).
	void Initialize(DirectXCommon* dx);
	/// Defers the release of the query heaps / readback buffers. Call before DirectXCommon::Finalize.
	void Finalize();

	// ---- Frame hooks (called by DirectXCommon) ----
	/// PreDraw, after the slot's fence was waited on: reads back the GPU timestamps of the frame that last used the slot,
	/// publishes the CPU frame that just ended and starts a new one.
	void BeginFrame(uint32_t frameSlot);
	/// PostDraw, before the command list is closed: samples counters and resolves the timestamps of this frame.
	void EndFrame(ID3D12GraphicsCommandList* cmdList);
	/// Publishes the CPU scopes / draw-call counters recorded since the previous call into GetLastFrame(), then clears them.
	/// BeginFrame calls it internally; it is public so tests can run without a frame loop.
	void FinishCpuFrame();

	// ---- CPU scopes ----
	uint32_t BeginCpu(const char* name);
	void EndCpu(uint32_t token);

	// ---- GPU scopes (ignored beyond kMaxGpuScopes or outside BeginFrame/EndFrame) ----
	uint32_t BeginGpu(ID3D12GraphicsCommandList* cmdList, const char* name);
	void EndGpu(ID3D12GraphicsCommandList* cmdList, uint32_t token);

	// ---- Counters ----
	void CountDrawCall(uint32_t instances = 1)
	{
		++pendingDrawCalls_;
		pendingInstances_ += instances;
	}

	const ProfilerSnapshot& GetLastFrame() const { return snapshot_; }
	bool IsGpuEnabled() const { return gpuEnabled_; }

	struct CpuScope {
		explicit CpuScope(const char* name) : token_(GetInstance()->BeginCpu(name)) {}
		~CpuScope() { GetInstance()->EndCpu(token_); }
		CpuScope(const CpuScope&) = delete;
		CpuScope& operator=(const CpuScope&) = delete;
	private:
		uint32_t token_;
	};

	struct GpuScope {
		GpuScope(ID3D12GraphicsCommandList* cmdList, const char* name)
			: cmdList_(cmdList), token_(GetInstance()->BeginGpu(cmdList, name)) {}
		~GpuScope() { GetInstance()->EndGpu(cmdList_, token_); }
		GpuScope(const GpuScope&) = delete;
		GpuScope& operator=(const GpuScope&) = delete;
	private:
		ID3D12GraphicsCommandList* cmdList_;
		uint32_t token_;
	};

private:
	Profiler() = default;
	Profiler(const Profiler&) = delete;
	Profiler& operator=(const Profiler&) = delete;

	struct CpuRecord {
		const char* name = "";
		uint32_t depth = 0;
		int64_t begin = 0;
		int64_t end = 0;
		bool ended = false;
	};
	struct EmaEntry {
		const char* name = "";
		float avg = 0.0f;
	};
	struct GpuSlot {
		Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap;
		Microsoft::WRL::ComPtr<ID3D12Resource> readback;
		const char* names[kMaxGpuScopes] = {};
		uint32_t depths[kMaxGpuScopes] = {};
		bool ended[kMaxGpuScopes] = {};
		uint32_t count = 0;          ///< Scopes recorded this frame
		uint32_t resolvedCount = 0;  ///< Scopes resolved into the readback buffer (valid once the GPU finished the frame)
	};

	static float UpdateEma(std::vector<EmaEntry>& table, const char* name, float ms);
	static void AddResult(std::vector<ProfilerScopeResult>& list, const char* name, uint32_t depth, float ms);
	void ReadBackSlot(GpuSlot& slot);

	DirectXCommon* dx_ = nullptr;
	bool gpuEnabled_ = false;
	bool frameOpen_ = false;
	uint32_t currentSlot_ = 0;
	uint64_t gpuFrequency_ = 0;
	GpuSlot gpuSlots_[DirectXCommon::kFramesInFlight];
	uint32_t gpuDepth_ = 0;

	int64_t qpcFrequency_ = 0;
	int64_t lastFrameQpc_ = 0;
	CpuRecord cpuRecords_[kMaxCpuScopes];
	uint32_t cpuCount_ = 0;
	uint32_t cpuDepth_ = 0;

	uint32_t pendingDrawCalls_ = 0;
	uint32_t pendingInstances_ = 0;
	size_t pendingUploadBytes_ = 0;
	uint32_t pendingSrvUsed_ = 0;

	std::vector<EmaEntry> cpuEma_;
	std::vector<EmaEntry> gpuEma_;
	ProfilerSnapshot snapshot_;
};

#define RIZE_PROFILE_CONCAT_INNER(a, b) a##b
#define RIZE_PROFILE_CONCAT(a, b) RIZE_PROFILE_CONCAT_INNER(a, b)
/// Times the rest of the enclosing block on the CPU. `name` must be a string literal.
#define RIZE_PROFILE_CPU(name) Profiler::CpuScope RIZE_PROFILE_CONCAT(rizeCpuScope_, __LINE__)(name)
/// Times the commands recorded into `cmdList` for the rest of the enclosing block on the GPU.
#define RIZE_PROFILE_GPU(cmdList, name) Profiler::GpuScope RIZE_PROFILE_CONCAT(rizeGpuScope_, __LINE__)((cmdList), (name))
