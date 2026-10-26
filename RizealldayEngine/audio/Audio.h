#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#pragma comment(lib, "xaudio2.lib")

class Audio
{
public:
	static Audio* GetInstance();

	struct SoundData {
		WAVEFORMATEX wfex{};
		std::vector<BYTE> buffer;
		unsigned int bufferSize = 0;
		std::string filePath;
	};

	void Initialize();
	void Update();
	void Finalize();

	/// Load audio file (.wav, .mp3, .m4a, .wma) via Media Foundation (cached: loads from disk only once per file)
	uint32_t LoadAudio(const std::string& filename);

	/// Play cached audio by handle (no file I/O; uses in-memory decoded PCM buffer)
	/// @return Active voice handle (can be passed to StopAudio / SetVolume / IsPlaying)
	uint32_t PlayAudio(uint32_t soundHandle, bool loop = false, float volume = 1.0f);

	/// Stop a specific playing voice
	void StopAudio(uint32_t voiceHandle);

	/// Stop all playing voices
	void StopAllAudio();

	/// Adjust volume of a playing voice (0.0f ~ 1.0f+)
	void SetVolume(uint32_t voiceHandle, float volume);

	/// Check if a voice handle is currently playing
	bool IsPlaying(uint32_t voiceHandle) const;

	/// Load audio file into SoundData (uses Media Foundation + internal cache)
	SoundData SoundLoadWave(const std::string& filename);

	/// Play audio from SoundData
	uint32_t SoundPlayWave(const SoundData& soundData, bool loop = false, float volume = 1.0f);

	/// Release SoundData buffer
	void SoundUnload(SoundData* soundData);

private:
	Audio() = default;
	~Audio() = default;
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	struct ActiveVoice {
		uint32_t voiceHandle = 0;
		uint32_t soundHandle = 0;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		bool loop = false;
	};

	static std::string NormalizePath(const std::string& filePath);
	static std::string ResolveFilePath(const std::string& filename);
	bool DecodeWithMediaFoundation(const std::string& resolvedPath, SoundData& outData);
	bool DecodeWaveFallback(const std::string& resolvedPath, SoundData& outData);

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
	bool mfInitialized_ = false;

	uint32_t nextSoundHandle_ = 1;
	uint32_t nextVoiceHandle_ = 1;
	std::unordered_map<std::string, uint32_t> soundCache_;
	std::unordered_map<uint32_t, SoundData> soundDataMap_;
	std::vector<ActiveVoice> activeVoices_;
};
