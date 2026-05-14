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

namespace mc::sound {

/**
 * @brief 声音类型（文件或事件引用）
 *
 * 描述声音资源的来源类型。
 *
 * 参考: net.minecraft.client.audio.Sound.Type
 */
enum class SoundType : u8 {
    File, ///< 直接文件引用（OGG 文件）
    Event ///< 引用另一个声音事件
};

/**
 * @brief 声音衰减类型
 *
 * 定义声音如何随距离衰减。
 *
 * 参考: net.minecraft.client.audio.ISound.AttenuationType
 */
enum class AttenuationType : u8 {
    None,  ///< 无衰减（全局声音，如背景音乐）
    Linear ///< 线性衰减（基于距离）
};

/**
 * @brief 声音实例 ID 类型
 *
 * 用于唯一标识正在播放的声音实例。
 * ID 从 1 开始，0 表示无效 ID。
 */
using SoundInstanceId = u64;

/**
 * @brief 无效的声音实例 ID
 */
constexpr SoundInstanceId INVALID_SOUND_INSTANCE_ID = 0;

/**
 * @brief 音频缓冲区 ID 类型
 *
 * 用于标识 OpenAL 音频缓冲区。
 */
using AudioBufferId = u32;

/**
 * @brief 音频源 ID 类型
 *
 * 用于标识 OpenAL 音频源。
 */
using AudioSourceId = u32;

/**
 * @brief 音频源状态
 */
enum class AudioSourceState : u8 {
    Initial, ///< 初始状态（未播放）
    Playing, ///< 正在播放
    Paused,  ///< 已暂停
    Stopped  ///< 已停止
};

/**
 * @brief 默认衰减距离
 *
 * 声音默认的可听距离（格）。
 */
constexpr f32 DEFAULT_ATTENUATION_DISTANCE = 16.0f;

/**
 * @brief 最大同时播放声音数
 */
constexpr u32 MAX_CONCURRENT_SOUNDS = 256;

/**
 * @brief 音频缓冲区大小（字节）
 *
 * 用于流式播放的单个缓冲区大小。
 */
constexpr u32 STREAM_BUFFER_SIZE = 65536; // 64KB

/**
 * @brief 音频采样率
 */
constexpr u32 AUDIO_SAMPLE_RATE = 44100;

/**
 * @brief 音频通道数
 */
constexpr u32 AUDIO_CHANNELS = 2; // 立体声

/**
 * @brief 音频位深度
 */
constexpr u32 AUDIO_BITS_PER_SAMPLE = 16;

} // namespace mc::sound
