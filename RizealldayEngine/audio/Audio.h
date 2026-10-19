#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <string>
#include <vector>
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
	};

	void Initialize();
	void Finalize();

	/// Load WAV audio file
	SoundData SoundLoadWave(const std::string& filename);
	/// Play audio
	void SoundPlayWave(const SoundData& soundData);
	/// Release audio data
	void SoundUnload(SoundData* soundData);

private:
	Audio() = default;
	~Audio() = default;
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
};
