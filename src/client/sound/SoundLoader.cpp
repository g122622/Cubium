#include "client/sound/SoundLoader.hpp"

#include "common/resource/ResourcePackList.hpp"
#include "common/resource/IResourcePack.hpp"

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include <spdlog/spdlog.h>

namespace mc::client::sound {

SoundLoader::SoundLoader(ResourcePackList& resourcePacks)
    : m_resourcePacks(resourcePacks)
{
}

Result<AudioData> SoundLoader::load(const ResourceLocation& location) {
    // 构建音频文件路径
    String audioPath = toAudioPath(location);

    // 从资源包加载
    auto result = m_resourcePacks.getResource(audioPath);
    if (!result.success()) {
        return Error(ErrorCode::ResourceNotFound,
                     fmt::format("Failed to load audio: {}", audioPath));
    }

    auto resource = result.value();
    auto dataResult = resource->readAll();
    if (!dataResult.success()) {
        return Error(ErrorCode::FileReadFailed,
                     fmt::format("Failed to read audio data: {}", audioPath));
    }

    auto& data = dataResult.value();

    // 解码 OGG Vorbis
    return decode(std::span<const u8>(data.data(), data.size()));
}

Result<AudioData> SoundLoader::decode(std::span<const u8> data) {
    if (data.empty()) {
        return Error(ErrorCode::InvalidData, "Empty audio data");
    }

    // 使用 stb_vorbis 解码
    int error = VORBIS__no_error;
    stb_vorbis* vorbis = stb_vorbis_open_memory(
        data.data(),
        static_cast<int>(data.size()),
        &error,
        nullptr
    );

    if (!vorbis) {
        String errorMsg;
        switch (error) {
            case VORBIS_need_more_data:
                errorMsg = "Need more data";
                break;
            case VORBIS_invalid_api_mixing:
                errorMsg = "Invalid API mixing";
                break;
            case VORBIS_outofmem:
                errorMsg = "Out of memory";
                break;
            case VORBIS_too_many_channels:
                errorMsg = "Too many channels";
                break;
            case VORBIS_file_open_failure:
                errorMsg = "File open failure";
                break;
            case VORBIS_seek_without_length:
                errorMsg = "Seek without length";
                break;
            case VORBIS_unexpected_eof:
                errorMsg = "Unexpected EOF";
                break;
            case VORBIS_seek_invalid:
                errorMsg = "Seek invalid";
                break;
            case VORBIS_invalid_setup:
                errorMsg = "Invalid setup";
                break;
            case VORBIS_invalid_stream:
                errorMsg = "Invalid stream";
                break;
            case VORBIS_missing_capture_pattern:
                errorMsg = "Missing capture pattern";
                break;
            case VORBIS_invalid_packet_structure:
                errorMsg = "Invalid packet structure";
                break;
            case VORBIS_continued_packet_flag_invalid:
                errorMsg = "Continued packet flag invalid";
                break;
            case VORBIS_incorrect_stream_serial_number:
                errorMsg = "Incorrect stream serial number";
                break;
            case VORBIS_incorrect_page_number:
                errorMsg = "Incorrect page number";
                break;
            case VORBIS_checksum_failed:
                errorMsg = "Checksum failed";
                break;
            case VORBIS_invalid_bitstream:
                errorMsg = "Invalid bitstream";
                break;
            default:
                errorMsg = fmt::format("Unknown error: {}", error);
                break;
        }

        return Error(ErrorCode::InvalidData,
                     fmt::format("Failed to decode OGG Vorbis: {}", errorMsg));
    }

    // 获取音频信息
    stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    u32 sampleRate = static_cast<u32>(info.sample_rate);
    u16 channels = static_cast<u16>(info.channels);

    // 计算样本总数
    int totalSamples = stb_vorbis_stream_length_in_samples(vorbis);
    if (totalSamples <= 0) {
        stb_vorbis_close(vorbis);
        return Error(ErrorCode::InvalidData, "Audio has no samples");
    }

    // 分配缓冲区（16-bit 样本）
    size_t totalFrames = static_cast<size_t>(totalSamples);
    size_t bufferSize = totalFrames * channels * sizeof(i16);
    std::vector<u8> samples(bufferSize);

    // 解码所有样本
    i16* output = reinterpret_cast<i16*>(samples.data());
    int framesDecoded = stb_vorbis_get_samples_short_interleaved(
        vorbis,
        channels,
        output,
        static_cast<int>(totalFrames * channels)
    );

    stb_vorbis_close(vorbis);

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
                  sampleRate, channels, audioData.duration, actualSize);

    return audioData;
}

String SoundLoader::toAudioPath(const ResourceLocation& location) {
    // minecraft:sounds/dig/stone1 -> assets/minecraft/sounds/dig/stone1.ogg
    return fmt::format("assets/{}/sounds/{}.ogg",
                       location.getNamespace(), location.getPath());
}

} // namespace mc::client::sound
