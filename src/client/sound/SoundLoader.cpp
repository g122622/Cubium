#include "client/sound/SoundLoader.hpp"

#include "common/resource/IResourcePack.hpp"
#include "common/resource/ResourcePackList.hpp"

#include <spdlog/spdlog.h>

#include <limits>

// stb_vorbis 错误码（避免直接包含 stb_vorbis.h 导致与 fmt 库冲突）
#define VORBIS__no_error 0
#define VORBIS_invalid_api_mixing 1
#define VORBIS_outofmem 2
#define VORBIS_too_many_channels 3
#define VORBIS_file_open_failure 4
#define VORBIS_seek_without_length 5
#define VORBIS_unexpected_eof 6
#define VORBIS_seek_invalid 7
#define VORBIS_invalid_setup 8
#define VORBIS_invalid_stream 9
#define VORBIS_missing_capture_pattern 10
#define VORBIS_continued_packet_flag_invalid 11
#define VORBIS_incorrect_stream_serial_number 12
#define VORBIS_need_more_data 13

// stb_vorbis 稳定包装函数声明（实现在 StbVorbisImpl.cpp）
extern "C" {
struct stb_vorbis;
typedef struct stb_vorbis stb_vorbis;

stb_vorbis* mc_stb_vorbis_open_memory(const unsigned char* data, int len, int* error);
void mc_stb_vorbis_close(stb_vorbis* v);
int mc_stb_vorbis_get_info(stb_vorbis* v, unsigned int* sampleRate, int* channels);
int mc_stb_vorbis_stream_length_in_samples(stb_vorbis* v);
int mc_stb_vorbis_get_samples_short_interleaved(stb_vorbis* v, int channels, short* output, int numSamples);
}

namespace mc::client::sound {

SoundLoader::SoundLoader(ResourcePackList& resourcePacks)
    : m_resourcePacks(resourcePacks)
{}

Result<AudioData> SoundLoader::load(const ResourceLocation& location)
{
    // 构建音频文件路径
    std::string audioPath = toAudioPath(location);

    // 从资源包加载
    auto result = m_resourcePacks.readResource(audioPath);
    if (!result.success()) {
        return Error(ErrorCode::ResourceNotFound, fmt::format("Failed to load audio: {}", audioPath));
    }

    auto& data = result.value();

    // 解码 OGG Vorbis
    return decode(data.data(), data.size());
}

Result<AudioData> SoundLoader::decode(const u8* data, size_t size)
{
    if (data == nullptr || size == 0) {
        return Error(ErrorCode::InvalidData, "Empty audio data");
    }

    if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return Error(ErrorCode::InvalidData, fmt::format("Audio data too large for stb_vorbis: {} bytes", size));
    }

    // 使用 stb_vorbis 解码
    int error = VORBIS__no_error;
    stb_vorbis* vorbis = mc_stb_vorbis_open_memory(data, static_cast<int>(size), &error);

    if (!vorbis) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to decode OGG Vorbis, error code: {}", error));
    }

    // 获取音频信息
    unsigned int sampleRateRaw = 0;
    int channelsRaw = 0;
    if (mc_stb_vorbis_get_info(vorbis, &sampleRateRaw, &channelsRaw) == 0) {
        mc_stb_vorbis_close(vorbis);
        return Error(ErrorCode::InvalidData, "Failed to read OGG metadata");
    }

    if (channelsRaw <= 0 || channelsRaw > 2) {
        mc_stb_vorbis_close(vorbis);
        return Error(ErrorCode::Unsupported, fmt::format("Unsupported channel count: {}", channelsRaw));
    }

    u32 sampleRate = sampleRateRaw;
    u16 channels = static_cast<u16>(channelsRaw);

    // 计算样本总数
    int totalSamples = mc_stb_vorbis_stream_length_in_samples(vorbis);
    if (totalSamples <= 0) {
        mc_stb_vorbis_close(vorbis);
        return Error(ErrorCode::InvalidData, "Audio has no samples");
    }

    // 分配缓冲区（16-bit 样本）
    size_t totalFrames = static_cast<size_t>(totalSamples);
    size_t bufferSize = totalFrames * channels * sizeof(i16);
    std::vector<u8> samples(bufferSize);

    // 解码所有样本
    i16* output = reinterpret_cast<i16*>(samples.data());
    int framesDecoded =
        mc_stb_vorbis_get_samples_short_interleaved(vorbis, channels, output, static_cast<int>(totalFrames * channels));

    mc_stb_vorbis_close(vorbis);

    if (framesDecoded <= 0) {
        return Error(ErrorCode::InvalidData, "Failed to decode any audio samples");
    }

    // 调整缓冲区大小（如果解码的帧数少于预期）
    size_t actualSize = static_cast<size_t>(framesDecoded) * channels * sizeof(i16);
    if (actualSize < bufferSize) {
        samples.resize(actualSize);
    }

    // 创建音频数据
    AudioFormat format;
    format.sampleRate = sampleRate;
    format.channels = channels;
    format.bitsPerSample = 16;

    AudioData audioData(format, std::move(samples));

    spdlog::debug("[SoundLoader] Decoded audio: {} Hz, {} channels, {:.2f}s, {} bytes",
        sampleRate,
        channels,
        audioData.duration,
        actualSize);

    return audioData;
}

std::string SoundLoader::toAudioPath(const ResourceLocation& location)
{
    // minecraft:sounds/dig/stone1 -> assets/minecraft/sounds/dig/stone1.ogg
    return fmt::format("assets/{}/sounds/{}.ogg", location.namespace_(), location.path());
}

} // namespace mc::client::sound
