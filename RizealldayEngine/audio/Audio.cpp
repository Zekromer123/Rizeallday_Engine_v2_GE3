#include "audio/Audio.h"
#include <cassert>
#include <fstream>
#include <cstring>

Audio* Audio::GetInstance()
{
	static Audio instance;
	return &instance;
}

void Audio::Initialize()
{
	HRESULT result;
	result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
	if (xAudio2_)
	{
		xAudio2_.Reset();
	}
	masterVoice_ = nullptr;
}

Audio::SoundData Audio::SoundLoadWave(const std::string& filename)
{
	struct ChunkHeader { char id[4]; int32_t size; };
	struct RiffHeader { ChunkHeader chunk; char type[4]; };
	struct FormatChunk { ChunkHeader chunk; WAVEFORMATEX fmt; };

	std::ifstream file;
	file.open(filename, std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	assert(std::strncmp(riff.chunk.id, "RIFF", 4) == 0);
	assert(std::strncmp(riff.type, "WAVE", 4) == 0);

	FormatChunk format = {};
	ChunkHeader chunkHeader;
	while (file.read((char*)&chunkHeader, sizeof(ChunkHeader)))
	{
		if (std::strncmp(chunkHeader.id, "fmt ", 4) == 0)
		{
			format.chunk = chunkHeader;
			assert(format.chunk.size <= sizeof(format.fmt));
			file.read((char*)&format.fmt, format.chunk.size);
			break;
		}
		else
		{
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}
	}

	ChunkHeader data = {};
	while (file.read((char*)&chunkHeader, sizeof(ChunkHeader)))
	{
		if (std::strncmp(chunkHeader.id, "data", 4) == 0)
		{
			data = chunkHeader;
			break;
		}
		else
		{
			file.seekg(chunkHeader.size, std::ios_base::cur);
		}
	}

	SoundData soundData{};
	soundData.wfex = format.fmt;
	if (data.size > 0)
	{
		soundData.buffer.resize(static_cast<size_t>(data.size));
		file.read(reinterpret_cast<char*>(soundData.buffer.data()), data.size);
	}
	file.close();
	soundData.bufferSize = static_cast<unsigned int>(soundData.buffer.size());
	return soundData;
}

void Audio::SoundPlayWave(const SoundData& soundData)
{
	HRESULT result;
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.buffer.data();
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	result = pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = pSourceVoice->Start();
	assert(SUCCEEDED(result));
}

void Audio::SoundUnload(SoundData* soundData)
{
	if (!soundData) return;
	soundData->buffer.clear();
	soundData->buffer.shrink_to_fit();
	soundData->bufferSize = 0;
	soundData->wfex = {};
}
