/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "client/sound/SoundLoader.hpp"

#include "client/sound/backend/AudioBuffer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/PackRepository.hpp"

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <fmt/format.h>

namespace {

using mc::i32;

// stb_vorbis 错误码（避免直接包含 stb_vorbis.h 导致与 fmt 库冲突）
constexpr i32 kVorbisNoError = 0;
constexpr i32 kVorbisInvalidApiMixing = 1;
constexpr i32 kVorbisOutofmem = 2;
constexpr i32 kVorbisTooManyChannels = 3;
constexpr i32 kVorbisFileOpenFailure = 4;
constexpr i32 kVorbisSeekWithoutLength = 5;
constexpr i32 kVorbisUnexpectedEof = 6;
constexpr i32 kVorbisSeekInvalid = 7;
constexpr i32 kVorbisInvalidSetup = 8;
constexpr i32 kVorbisInvalidStream = 9;
constexpr i32 kVorbisMissingCapturePattern = 10;
constexpr i32 kVorbisContinuedPacketFlagInvalid = 11;
constexpr i32 kVorbisIncorrectStreamSerialNumber = 12;
constexpr i32 kVorbisNeedMoreData = 13;

} // namespace

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

SoundLoader::SoundLoader(PackRepository& resourcePacks)
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

    // 使用 stb_vorbis 解码（变量类型需匹配 C ABI）
    int error = kVorbisNoError;
    stb_vorbis* vorbis = mc_stb_vorbis_open_memory(data, static_cast<int>(size), &error);

    if (!vorbis) {
        return Error(ErrorCode::InvalidData, fmt::format("Failed to decode OGG Vorbis, error code: {}", error));
    }

    // 获取音频信息（变量类型需匹配 C ABI）
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

    return audioData;
}

std::string SoundLoader::toAudioPath(const ResourceLocation& location)
{
    // 返回相对于 PackType 根目录的路径（不含 "assets/" 前缀）
    // PackRepository::readResource 会自动添加 PackType 目录前缀
    // minecraft:sounds/dig/stone1 -> minecraft/sounds/dig/stone1.ogg
    return fmt::format("{}/sounds/{}.ogg", location.namespace_(), location.path());
}

} // namespace mc::client::sound
