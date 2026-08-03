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
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundTypes.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::AttenuationType;
using ::mc::sound::SoundCategory;
using ::mc::sound::SoundInstanceId;

// 前向声明
class SoundHandler;

/**
 * @brief 声音实例接口
 *
 * 定义正在播放或待播放的声音的所有属性。
 * 每个声音实例代表一个独立的可播放声音。
 *
 * 使用示例:
 * @code
 * // 创建全局声音
 * auto sound = SoundInstance::createGlobal(
 *     ResourceLocation("minecraft:music.game"),
 *     SoundCategory::Music,
 *     1.0f, 1.0f
 * );
 *
 * // 创建位置声音
 * auto sound = SoundInstance::createLocated(
 *     ResourceLocation("minecraft:block.stone.break"),
 *     SoundCategory::Blocks,
 *     pos.x, pos.y, pos.z
 * );
 *
 * engine->play(std::move(sound));
 * @endcode
 */
class ISoundInstance {
public:
    virtual ~ISoundInstance() = default;

    // ========================================================================
    // 基础属性
    // ========================================================================

    /**
     * @brief 获取声音事件ID
     *
     * 声音事件ID用于在 SoundRegistry 中查找声音定义。
     */
    [[nodiscard]] virtual const ResourceLocation& getSoundEventId() const = 0;

    /**
     * @brief 获取声音类别
     *
     * 用于音量控制和分类。
     */
    [[nodiscard]] virtual SoundCategory getCategory() const = 0;

    // ========================================================================
    // 音量和音调
    // ========================================================================

    /**
     * @brief 获取音量
     *
     * 音量范围 [0.0, ...]，1.0 为正常音量。
     * 实际音量 = 音量 * 类别音量 * 主音量。
     */
    [[nodiscard]] virtual f32 getVolume() const = 0;

    /**
     * @brief 获取音调
     *
     * 音调范围 [0.5, 2.0]，1.0 为正常音调。
     */
    [[nodiscard]] virtual f32 getPitch() const = 0;

    /**
     * @brief 设置音量
     *
     * 用于动态调整音量（如音乐淡入淡出）。
     * 音量范围 [0.0, ...]，1.0 为正常音量。
     */
    virtual void setVolume(f32 volume) = 0;

    /**
     * @brief 设置音调
     *
     * 用于动态调整音调。
     * 音调范围 [0.5, 2.0]，1.0 为正常音调。
     */
    virtual void setPitch(f32 pitch) = 0;

    // ========================================================================
    // 位置
    // ========================================================================

    /**
     * @brief 获取 X 坐标
     */
    [[nodiscard]] virtual f32 getX() const = 0;

    /**
     * @brief 获取 Y 坐标
     */
    [[nodiscard]] virtual f32 getY() const = 0;

    /**
     * @brief 获取 Z 坐标
     */
    [[nodiscard]] virtual f32 getZ() const = 0;

    /**
     * @brief 获取位置向量
     */
    [[nodiscard]] glm::vec3 getPosition() const { return glm::vec3(getX(), getY(), getZ()); }

    // ========================================================================
    // 循环和延迟
    // ========================================================================

    /**
     * @brief 是否循环播放
     */
    [[nodiscard]] virtual bool isLooping() const = 0;

    /**
     * @brief 获取重复延迟（游戏 ticks）
     *
     * 仅对非循环声音有效，播放后延迟指定 ticks 再播放。
     */
    [[nodiscard]] virtual u32 getRepeatDelay() const = 0;

    // ========================================================================
    // 衰减和全局
    // ========================================================================

    /**
     * @brief 获取衰减类型
     */
    [[nodiscard]] virtual AttenuationType getAttenuationType() const = 0;

    /**
     * @brief 是否为全局声音
     *
     * 全局声音不受听者位置影响。
     */
    [[nodiscard]] virtual bool isGlobal() const = 0;

    /**
     * @brief 获取衰减距离
     *
     * 声音可听的最大距离。
     */
    [[nodiscard]] virtual f32 getAttenuationDistance() const = 0;

    // ========================================================================
    // 状态
    // ========================================================================

    /**
     * @brief 获取声音实例ID
     */
    [[nodiscard]] virtual SoundInstanceId getId() const = 0;

    /**
     * @brief 设置声音实例ID
     *
     * 由 SoundEngine 在播放时设置。
     */
    virtual void setId(SoundInstanceId id) = 0;

    /**
     * @brief 是否已完成播放
     */
    [[nodiscard]] virtual bool isDone() const = 0;

    /**
     * @brief 是否可以静音播放
     *
     * 用于 TickableSound 的声音切换检查。
     * 如果返回 true，声音可以在音量为 0 时播放。
     */
    [[nodiscard]] virtual bool canBeSilent() const { return false; }

    // ========================================================================
    // 更新
    // ========================================================================

    /**
     * @brief 每帧更新
     *
     * 用于可更新的声音（如 TickableSound）。
     */
    virtual void tick() {}

    // TODO: 实现声音访问器，用于延迟解析声音事件到具体的声音文件
    // [[nodiscard]] virtual Result<SoundAccessor> createAccessor(SoundHandler& handler) = 0;
};

} // namespace mc::client::sound
