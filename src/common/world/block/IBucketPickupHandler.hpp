/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, WHETHER
 * ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
class Item;
namespace fluid {
class Fluid;
}

/**
 * @brief 桶拾取处理器接口
 *
 * 实现此接口的方块可以被空桶从中取出内容物。
 * 当玩家使用空桶右键方块时，会调用 pickupFluid 或 pickupItem 方法。
 *
 * 流体方块（水、岩浆）实现 pickupFluid，返回对应的流体。
 * 非流体方块（细雪）实现 pickupItem，返回对应的桶物品。
 */
class IBucketPickupHandler {
public:
    virtual ~IBucketPickupHandler() = default;

    /**
     * @brief 从方块中取出流体
     *
     * 流体方块（水、岩浆）重写此方法，返回被取出的流体。
     * 非流体方块（如细雪）应返回 nullptr，并改为重写 pickupItem。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 被取出的流体，如果无法取出则返回 nullptr
     */
    [[nodiscard]] virtual fluid::Fluid* pickupFluid(IWorld& world, const BlockPos& pos, const BlockState& state) = 0;

    /**
     * @brief 从方块中取出内容物并返回桶物品
     *
     * 非流体方块（如细雪）重写此方法，返回拾取后应给予玩家的桶物品。
     * 默认实现返回 nullptr，表示方块不支持非流体拾取。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 拾取后应给予的桶物品，如果无法拾取则返回 nullptr
     */
    [[nodiscard]] virtual const Item* pickupItem(IWorld& world, const BlockPos& pos, const BlockState& state)
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return nullptr;
    }

    /**
     * @brief 获取拾取时播放的音效资源位置
     *
     * 默认实现返回 nullptr。流体方块使用 BucketItem 中硬编码的音效，
     * 非流体方块（如细雪）应重写此方法返回特定的拾取音效。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 音效资源位置，如果不需要特定音效则返回 nullptr
     */
    [[nodiscard]] virtual const ResourceLocation* getPickupSound(
        IWorld& world, const BlockPos& pos, const BlockState& state)
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return nullptr;
    }
};

} // namespace mc
