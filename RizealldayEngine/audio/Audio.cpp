#include "audio/Audio.h"
#include "base/ConvertString.h"
#include "base/Log.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <cassert>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <format>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

Audio* Audio::GetInstance()
{
	static Audio instance;
	return &instance;
}

void Audio::Initialize()
{
	HRESULT hrMf = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	if (SUCCEEDED(hrMf)) {
		mfInitialized_ = true;
	} else {
		Log::GetInstance()->logOutput(std::format("[Audio] MFStartup failed (HRESULT=0x{:08X}).\n", static_cast<uint32_t>(hrMf)));
	}

	// Machines without an audio device (CI runners, remote sessions) must still run: audio is disabled
	// instead of asserting. PlayAudio() already returns 0 when xAudio2_ is null.
	HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(result) || !xAudio2_) {
		Log::GetInstance()->logOutput(std::format("[Audio] XAudio2Create failed (HRESULT=0x{:08X}); audio disabled.\n", static_cast<uint32_t>(result)));
		xAudio2_.Reset();
		return;
	}
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	if (FAILED(result) || !masterVoice_) {
		Log::GetInstance()->logOutput(std::format("[Audio] No audio output device (HRESULT=0x{:08X}); audio disabled.\n", static_cast<uint32_t>(result)));
		masterVoice_ = nullptr;
		xAudio2_.Reset();
	}
}

void Audio::Update()
{
	if (activeVoices_.empty()) {
		return;
	}

	for (auto it = activeVoices_.begin(); it != activeVoices_.end(); ) {
		if (!it->sourceVoice) {
			it = activeVoices_.erase(it);
			continue;
		}
		XAUDIO2_VOICE_STATE state{};
		it->sourceVoice->GetState(&state);
		if (state.BuffersQueued == 0 && !it->loop) {
			it->sourceVoice->Stop(0);
			it->sourceVoice->DestroyVoice();
			it = activeVoices_.erase(it);
		} else {
			++it;
		}
	}
}

void Audio::Finalize()
{
	StopAllAudio();
	soundCache_.clear();
	soundDataMap_.clear();

	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}
	if (xAudio2_) {
		xAudio2_.Reset();
	}
	if (mfInitialized_) {
		MFShutdown();
		mfInitialized_ = false;
	}
}

std::string Audio::NormalizePath(const std::string& filePath)
{
	std::string norm;
	norm.reserve(filePath.size());
	char prev = 0;
	for (char ch : filePath) {
		char c = (ch == '\\') ? '/' : static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
		if (c == '/' && prev == '/') {
			continue;
		}
		norm.push_back(c);
		prev = c;
	}
	if (norm.rfind("./", 0) == 0) {
		norm.erase(0, 2);
	}
	return norm;
}

std::string Audio::ResolveFilePath(const std::string& filename)
{
	if (filename.empty()) {
		return "";
	}
	if (std::filesystem::exists(filename)) {
		return filename;
	}

	wchar_t exePath[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path pExe(exePath);

	std::vector<std::filesystem::path> candidates = {
		pExe.parent_path() / filename,
		pExe.parent_path().parent_path() / "RizealldaySandbox" / filename,
		pExe.parent_path() / "Resources" / filename,
		pExe.parent_path() / "Resources" / "Sound" / filename,
		std::filesystem::path("Resources") / "Sound" / filename
	};

	for (const auto& candidate : candidates) {
		if (std::filesystem::exists(candidate)) {
			return candidate.string();
		}
	}
	return filename;
}

bool Audio::DecodeWithMediaFoundation(const std::string& resolvedPath, SoundData& outData)
{
	if (!mfInitialized_) {
		return false;
	}

	ConvertStrings converter;
	std::wstring wPath = converter.ConvertString(resolvedPath);

	Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
	HRESULT hr = MFCreateSourceReaderFromURL(wPath.c_str(), nullptr, &sourceReader);
	if (FAILED(hr) || !sourceReader) {
		return false;
	}

	// Select only the first audio stream
	sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
	hr = sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);
	if (FAILED(hr)) {
		return false;
	}

	// Request uncompressed PCM output from Media Foundation
	Microsoft::WRL::ComPtr<IMFMediaType> partialType;
	hr = MFCreateMediaType(&partialType);
	if (FAILED(hr)) {
		return false;
	}
	partialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	partialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

	hr = sourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, partialType.Get());
	if (FAILED(hr)) {
		return false;
	}

	// Retrieve the complete uncompressed output media type
	Microsoft::WRL::ComPtr<IMFMediaType> uncompressedType;
	hr = sourceReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &uncompressedType);
	if (FAILED(hr)) {
		return false;
	}

	WAVEFORMATEX* pWaveFormat = nullptr;
	UINT32 waveFormatSize = 0;
	hr = MFCreateWaveFormatExFromMFMediaType(uncompressedType.Get(), &pWaveFormat, &waveFormatSize);
	if (FAILED(hr) || !pWaveFormat) {
		return false;
	}

	outData.wfex = *pWaveFormat;
	outData.wfex.cbSize = 0;
	CoTaskMemFree(pWaveFormat);

	outData.buffer.clear();

	// Read all decoded PCM samples into memory
	while (true) {
		DWORD streamFlags = 0;
		Microsoft::WRL::ComPtr<IMFSample> sample;
		hr = sourceReader->ReadSample(
			static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM),
			0,
			nullptr,
			&streamFlags,
			nullptr,
			&sample
		);

		if (FAILED(hr)) {
			break;
		}
		if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}
		if (!sample) {
			continue;
		}

		Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
		hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
		if (FAILED(hr) || !mediaBuffer) {
			continue;
		}

		BYTE* pAudioData = nullptr;
		DWORD currentLength = 0;
		hr = mediaBuffer->Lock(&pAudioData, nullptr, &currentLength);
		if (SUCCEEDED(hr) && pAudioData && currentLength > 0) {
			size_t oldSize = outData.buffer.size();
			outData.buffer.resize(oldSize + currentLength);
			std::memcpy(outData.buffer.data() + oldSize, pAudioData, currentLength);
			mediaBuffer->Unlock();
		}
	}

	outData.bufferSize = static_cast<unsigned int>(outData.buffer.size());
	outData.filePath = resolvedPath;
	return outData.bufferSize > 0;
}

bool Audio::DecodeWaveFallback(const std::string& resolvedPath, SoundData& outData)
{
	struct ChunkHeader { char id[4]; int32_t size; };
	struct RiffHeader { ChunkHeader chunk; char type[4]; };

	std::ifstream file(resolvedPath, std::ios_base::binary);
	if (!file.is_open()) {
		return false;
	}

	RiffHeader riff{};
	file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
	if (std::strncmp(riff.chunk.id, "RIFF", 4) != 0 || std::strncmp(riff.type, "WAVE", 4) != 0) {
		return false;
	}

	WAVEFORMATEX fmt{};
	ChunkHeader chunkHeader{};
	bool foundFmt = false;
	bool foundData = false;
	int32_t dataSize = 0;

	while (file.read(reinterpret_cast<char*>(&chunkHeader), sizeof(ChunkHeader))) {
		if (std::strncmp(chunkHeader.id, "fmt ", 4) == 0) {
			int32_t readBytes = (std::min)(chunkHeader.size, static_cast<int32_t>(sizeof(WAVEFORMATEX)));
			file.read(reinterpret_cast<char*>(&fmt), readBytes);
			if (chunkHeader.size > readBytes) {
				file.seekg(chunkHeader.size - readBytes, std::ios_base::cur);
			}
			fmt.cbSize = 0;
			foundFmt = true;
		} else if (std::strncmp(chunkHeader.id, "data", 4) == 0) {
			dataSize = chunkHeader.size;
			if (dataSize > 0) {
				outData.buffer.resize(static_cast<size_t>(dataSize));
				file.read(reinterpret_cast<char*>(outData.buffer.data()), dataSize);
				foundData = true;
			}
			break;
		} else {
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}
	}
	file.close();

	if (!foundFmt || !foundData) {
		return false;
	}

	outData.wfex = fmt;
	outData.bufferSize = static_cast<unsigned int>(outData.buffer.size());
	outData.filePath = resolvedPath;
	return outData.bufferSize > 0;
}

uint32_t Audio::LoadAudio(const std::string& filename)
{
	if (filename.empty() || filename == "None") {
		return 0;
	}

	std::string resolvedPath = ResolveFilePath(filename);
	std::string normKey = NormalizePath(resolvedPath);

	// Check cache first: avoid re-reading file if already loaded
	auto cacheIt = soundCache_.find(normKey);
	if (cacheIt != soundCache_.end()) {
		return cacheIt->second;
	}

	if (!std::filesystem::exists(resolvedPath)) {
		Log::GetInstance()->logOutput(std::format("[Audio] Audio file not found: {}\n", filename));
		return 0;
	}

	SoundData soundData{};
	bool decoded = DecodeWithMediaFoundation(resolvedPath, soundData);
	if (!decoded) {
		decoded = DecodeWaveFallback(resolvedPath, soundData);
	}

	if (!decoded || soundData.bufferSize == 0) {
		Log::GetInstance()->logOutput(std::format("[Audio] Failed to decode audio file: {}\n", resolvedPath));
		return 0;
	}

	uint32_t handle = nextSoundHandle_++;
	soundDataMap_[handle] = std::move(soundData);
	soundCache_[normKey] = handle;

	Log::GetInstance()->logOutput(std::format("[Audio] Loaded and cached audio '{}' (handle={}, {} bytes).\n",
		resolvedPath, handle, soundDataMap_[handle].bufferSize));
	return handle;
}

uint32_t Audio::PlayAudio(uint32_t soundHandle, bool loop, float volume)
{
	if (!xAudio2_ || soundHandle == 0) {
		return 0;
	}

	auto it = soundDataMap_.find(soundHandle);
	if (it == soundDataMap_.end() || it->second.buffer.empty()) {
		return 0;
	}

	const SoundData& soundData = it->second;
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	HRESULT result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	if (FAILED(result) || !pSourceVoice) {
		return 0;
	}

	pSourceVoice->SetVolume(std::clamp(volume, 0.0f, 4.0f));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	if (loop) {
		buf.LoopCount = XAUDIO2_LOOP_INFINITE;
	}

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	if (FAILED(result)) {
		pSourceVoice->DestroyVoice();
		return 0;
	}

	result = pSourceVoice->Start();
	if (FAILED(result)) {
		pSourceVoice->DestroyVoice();
		return 0;
	}

	uint32_t voiceHandle = nextVoiceHandle_++;
	activeVoices_.push_back({ voiceHandle, soundHandle, pSourceVoice, loop });
	return voiceHandle;
}

void Audio::StopAudio(uint32_t voiceHandle)
{
	if (voiceHandle == 0) {
		return;
	}
	for (auto it = activeVoices_.begin(); it != activeVoices_.end(); ++it) {
		if (it->voiceHandle == voiceHandle) {
			if (it->sourceVoice) {
				it->sourceVoice->Stop(0);
				it->sourceVoice->FlushSourceBuffers();
				it->sourceVoice->DestroyVoice();
			}
			activeVoices_.erase(it);
			return;
		}
	}
}

void Audio::StopAllAudio()
{
	for (auto& active : activeVoices_) {
		if (active.sourceVoice) {
			active.sourceVoice->Stop(0);
			active.sourceVoice->FlushSourceBuffers();
			active.sourceVoice->DestroyVoice();
		}
	}
	activeVoices_.clear();
}

void Audio::SetVolume(uint32_t voiceHandle, float volume)
{
	if (voiceHandle == 0) {
		return;
	}
	for (auto& active : activeVoices_) {
		if (active.voiceHandle == voiceHandle && active.sourceVoice) {
			active.sourceVoice->SetVolume(std::clamp(volume, 0.0f, 4.0f));
			return;
		}
	}
}

bool Audio::IsPlaying(uint32_t voiceHandle) const
{
	if (voiceHandle == 0) {
		return false;
	}
	for (const auto& active : activeVoices_) {
		if (active.voiceHandle == voiceHandle && active.sourceVoice) {
			XAUDIO2_VOICE_STATE state{};
			active.sourceVoice->GetState(&state);
			return state.BuffersQueued > 0;
		}
	}
	return false;
}

Audio::SoundData Audio::SoundLoadWave(const std::string& filename)
{
	uint32_t handle = LoadAudio(filename);
	if (handle != 0) {
		auto it = soundDataMap_.find(handle);
		if (it != soundDataMap_.end()) {
			return it->second;
		}
	}
	return SoundData{};
}

uint32_t Audio::SoundPlayWave(const SoundData& soundData, bool loop, float volume)
{
	if (!xAudio2_ || soundData.buffer.empty() || soundData.bufferSize == 0) {
		return 0;
	}

	if (!soundData.filePath.empty()) {
		std::string normKey = NormalizePath(soundData.filePath);
		auto cacheIt = soundCache_.find(normKey);
		if (cacheIt != soundCache_.end()) {
			return PlayAudio(cacheIt->second, loop, volume);
		}
	}

	// Register in soundDataMap_ so the buffer stays valid for the lifetime of the voice
	uint32_t handle = nextSoundHandle_++;
	soundDataMap_[handle] = soundData;
	if (!soundData.filePath.empty()) {
		soundCache_[NormalizePath(soundData.filePath)] = handle;
	}
	return PlayAudio(handle, loop, volume);
}

void Audio::SoundUnload(SoundData* soundData)
{
	if (!soundData) return;
	soundData->buffer.clear();
	soundData->buffer.shrink_to_fit();
	soundData->bufferSize = 0;
	soundData->wfex = {};
}
