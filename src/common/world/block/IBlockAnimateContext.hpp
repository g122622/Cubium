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
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc::client::renderer::trident::particle {
enum class ParticleTypeId : u16;
}

namespace mc {

class BlockState;
class BlockPos;

/**
 * @brief 方块动画 tick 上下文接口
 *
 * 为 Block::animateTick 提供轻量级的客户端操作接口。
 * 不依赖完整的 IWorld 接口，仅包含 animateTick 所需的功能。
 *
 * ClientWorld 实现此接口，在 animateTick 调度时传入自身引用。
 */
class IBlockAnimateContext {
public:
    virtual ~IBlockAnimateContext() = default;

    /**
     * @brief 生成粒子
     *
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     */
    virtual void addAnimateParticle(
        client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) = 0;

    /**
     * @brief 播放本地音效
     *
     * 在客户端本地播放音效，不走服务端广播。
     *
     * @param soundEventId 声音事件 ID
     * @param category 声音类别
     * @param position 音效位置
     * @param volume 音量
     * @param pitch 音调
     */
    virtual void playLocalSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) = 0;

    /**
     * @brief 获取指定位置的方块状态
     *
     * @param x 方块 X 坐标
     * @param y 方块 Y 坐标
     * @param z 方块 Z 坐标
     * @return 方块状态指针，如果位置无效或为空气则返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getBlockState(i32 x, i32 y, i32 z) const = 0;
};

} // namespace mc
