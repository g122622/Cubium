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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundTypes.hpp"
#include "common/util/math/random/Random.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::SoundType;

/**
 * @brief 单个声音定义
 *
 * 描述 sounds.json 中一个声音条目的属性。
 * 一个声音事件可以包含多个声音定义，播放时根据权重随机选择。
 *
 * 参考: net.minecraft.client.audio.Sound
 *
 * sounds.json 示例:
 * @code
 * {
 *   "block.stone.break": {
 *     "sounds": [
 *       "dig/stone1",
 *       "dig/stone2",
 *       "dig/stone3",
 *       {
 *         "name": "dig/stone4",
 *         "volume": 0.8,
 *         "pitch": 1.2,
 *         "weight": 2
 *       }
 *     ],
 *     "subtitle": "subtitles.block.generic.break"
 *   }
 * }
 * @endcode
 */
struct SoundDefinition {
    /// 声音文件位置或事件ID
    ResourceLocation location;

    /// 声音类型（文件引用或事件引用）
    SoundType type = SoundType::File;

    /// 音量倍率（默认 1.0）
    /// 超过 1.0 会增大可听距离
    f32 volume = 1.0f;

    /// 音调倍率（默认 1.0，范围 0.5-2.0）
    f32 pitch = 1.0f;

    /// 随机选择权重（默认 1，更大的值更可能被选中）
    u32 weight = 1;

    /// 是否流式播放（用于长音频，如唱片机音乐）
    /// 流式播放的声音从磁盘实时读取，不预加载到内存
    bool stream = false;

    /// 是否预加载（启动时加载到内存）
    bool preload = false;

    /// 距离衰减距离（格）
    /// 声音在此距离内可以听到
    u32 attenuationDistance = 16;

    /**
     * @brief 默认构造函数
     */
    SoundDefinition() = default;

    /**
     * @brief 从文件路径构造
     *
     * @param path 声音文件路径（相对于 assets/<namespace>/sounds/）
     */
    explicit SoundDefinition(std::string_view path);

    /**
     * @brief 从资源位置构造
     *
     * @param loc 资源位置
     */
    explicit SoundDefinition(const ResourceLocation& loc);

    /**
     * @brief 转换为OGG文件路径
     *
     * 将声音位置转换为实际的文件路径。
     * 例如: "minecraft:dig/stone1" -> "minecraft:sounds/dig/stone1.ogg"
     *
     * @return OGG 文件的资源位置
     */
    [[nodiscard]] ResourceLocation toOggLocation() const;

    /**
     * @brief 从JSON解析声音定义
     *
     * 支持两种格式：
     * 1. 简单字符串: "dig/stone1"
     * 2. 对象: {"name": "dig/stone1", "volume": 0.8, ...}
     *
     * @param json JSON值（字符串或对象）
     * @param namespace 默认命名空间
     * @return 解析结果
     */
    [[nodiscard]] static Result<SoundDefinition> parse(const nlohmann::json& json, std::string_view namespace_);
};

/**
 * @brief 声音事件定义（sounds.json 条目）
 *
 * 一个声音事件可以包含多个声音定义，播放时根据权重随机选择。
 * 支持替换已有定义和添加字幕。
 *
 * 参考: net.minecraft.client.audio.SoundList
 *
 * sounds.json 示例:
 * @code
 * {
 *   "block.stone.break": {
 *     "replace": true,
 *     "subtitle": "subtitles.block.generic.break",
 *     "sounds": [
 *       "dig/stone1",
 *       "dig/stone2",
 *       "dig/stone3"
 *     ]
 *   }
 * }
 * @endcode
 */
struct SoundEventDefinition {
    /// 声音事件ID（如 "minecraft:block.stone.break"）
    ResourceLocation location;

    /// 声音列表
    std::vector<SoundDefinition> sounds;

    /// 是否替换已有定义（默认 false，追加到已有定义）
    bool replace = false;

    /// 字幕键（可选，用于显示声音字幕）
    std::optional<std::string> subtitle;

    /**
     * @brief 默认构造函数
     */
    SoundEventDefinition() = default;

    /**
     * @brief 构造声音事件定义
     *
     * @param location 声音事件位置
     */
    explicit SoundEventDefinition(ResourceLocation location);

    /**
     * @brief 从字符串ID构造声音事件定义
     *
     * @param eventId 声音事件ID字符串（如 "minecraft:block.stone.break"）
     */
    explicit SoundEventDefinition(std::string_view eventId);

    /**
     * @brief 从JSON解析声音事件定义
     *
     * @param eventId 声音事件ID
     * @param json JSON对象
     * @param namespace 默认命名空间
     * @return 解析结果
     */
    [[nodiscard]] static Result<SoundEventDefinition> parse(
        std::string_view eventId, const nlohmann::json& json, std::string_view namespace_);

    /**
     * @brief 计算总权重
     *
     * @return 所有声音权重的总和
     */
    [[nodiscard]] u32 totalWeight() const noexcept;

    /**
     * @brief 根据权重随机选择一个声音
     *
     * @param rng 随机数生成器
     * @return 选中的声音定义，如果列表为空返回 nullptr
     */
    [[nodiscard]] const SoundDefinition* selectSound(mc::math::Random& rng) const noexcept;
};

} // namespace mc::client::sound
