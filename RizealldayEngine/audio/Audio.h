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
		WAVEFORMATEX wfex;
		BYTE* pBuffer;
		unsigned int bufferSize;
	};

	void Initialize();
	void Finalize();

	/// WAV音声読み込み
	SoundData SoundLoadWave(const std::string& filename);
	/// 音声再生
	void SoundPlayWave(const SoundData& soundData);
	/// 音声データ解放
	void SoundUnload(SoundData* soundData);

private:
	Audio() = default;
	~Audio() = default;
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;
};
