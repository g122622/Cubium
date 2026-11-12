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

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/blocks/container/CopperChestBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {

class IWorld;
class BlockPos;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 涂蜡铜箱子方块
 *
 * 涂蜡后的铜箱子不会氧化。重写 isWaxed() 返回 true，
 * 用于双箱合并时判断涂蜡状态。
 *
 */
class WaxedCopperChestBlock : public CopperChestBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param oxidationLevel 对应的氧化等级（涂蜡变体记录氧化等级，用于除蜡后恢复）
     * @param openSound 开箱音效事件（涂蜡变体复用对应氧化等级的声音）
     * @param closeSound 关箱音效事件（涂蜡变体复用对应氧化等级的声音）
     */
    WaxedCopperChestBlock(const BlockProperties& properties,
        BlockStateProperties::OxidationLevel oxidationLevel,
        const ResourceLocation& openSound,
        const ResourceLocation& closeSound)
        : CopperChestBlock(properties, oxidationLevel, openSound, closeSound)
    {}

    ~WaxedCopperChestBlock() override = default;

    /**
     * @brief 是否为涂蜡变体
     * @return 始终返回 true
     */
    [[nodiscard]] bool isWaxed() const noexcept override
    {
        return true;
    }
};

} // namespace blocks
} // namespace mc
