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

#pragma once

#include "common/core/Types.hpp"
#include "common/sound/SoundTypes.hpp"

#include <cstddef>
#include <vector>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::AudioBufferId;

/**
 * @brief 音频格式
 */
struct AudioFormat {
    u32 sampleRate = 44100; ///< 采样率 (Hz)
    u16 channels = 2;       ///< 通道数 (1=单声道, 2=立体声)
    u16 bitsPerSample = 16; ///< 每样本位数 (8, 16)

    /**
     * @brief 获取字节率
     * @return 每秒字节数
     */
    [[nodiscard]] u32 byteRate() const noexcept { return sampleRate * channels * (bitsPerSample / 8); }

    /**
     * @brief 获取块对齐
     * @return 一个样本块的字节数
     */
    [[nodiscard]] u16 blockAlign() const noexcept { return channels * (bitsPerSample / 8); }

    /**
     * @brief 检查格式是否有效
     */
    [[nodiscard]] bool isValid() const noexcept
    {
        return sampleRate > 0 && (channels == 1 || channels == 2) && (bitsPerSample == 8 || bitsPerSample == 16);
    }
};

/**
 * @brief 音频数据
 *
 * 包含解码后的 PCM 音频数据。
 * 支持 8-bit 和 16-bit 单声道/立体声格式。
 */
struct AudioData {
    AudioFormat format;      ///< 音频格式
    std::vector<u8> samples; ///< PCM 样本数据
    f32 duration = 0.0f;     ///< 时长（秒）

    /**
     * @brief 默认构造函数
     */
    AudioData() = default;

    /**
     * @brief 构造音频数据
     *
     * @param format 音频格式
     * @param data PCM 数据
     */
    AudioData(AudioFormat format, std::vector<u8> data);

    /**
     * @brief 检查数据是否有效
     */
    [[nodiscard]] bool isValid() const noexcept { return format.isValid() && !samples.empty(); }

    /**
     * @brief 获取样本数量
     * @return 总样本数（每个通道）
     */
    [[nodiscard]] size_t sampleCount() const noexcept;

    /**
     * @brief 获取时长（秒）
     */
    [[nodiscard]] f32 calculateDuration() const noexcept;
};

/**
 * @brief 音频缓冲区接口
 *
 * 表示一个已加载到音频后端的音频缓冲区。
 * 缓冲区包含不可变的音频数据，可以被多个音频源共享。
 *
 * 注意：这是音频后端资源的抽象接口。
 * 具体实现由 IAudioBackend::createBuffer() 创建。
 */
class IAudioBuffer {
public:
    virtual ~IAudioBuffer() = default;

    /**
     * @brief 获取缓冲区 ID
     *
     * 返回后端特定的缓冲区标识符。
     *
     * @return 缓冲区 ID
     */
    [[nodiscard]] virtual AudioBufferId getId() const noexcept = 0;

    /**
     * @brief 获取音频格式
     */
    [[nodiscard]] virtual const AudioFormat& getFormat() const noexcept = 0;

    /**
     * @brief 获取时长（秒）
     */
    [[nodiscard]] virtual f32 getDuration() const noexcept = 0;

    /**
     * @brief 获取样本数量
     */
    [[nodiscard]] virtual size_t getSampleCount() const noexcept = 0;

    /**
     * @brief 检查缓冲区是否有效
     */
    [[nodiscard]] virtual bool isValid() const noexcept = 0;
};

} // namespace mc::client::sound
