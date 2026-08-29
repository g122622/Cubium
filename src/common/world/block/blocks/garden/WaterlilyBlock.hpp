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
 */

#pragma once

#include "../agricultural/BushBlock.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 睡莲方块
 *
 * 水生植物，自然生成于沼泽水面。下方须为水或冰，且上方（睡莲自身位置）无流体。
 * 继承 BushBlock 并重写 canSustain 复刻 vanilla WaterlilyBlock.mayPlaceOn，
 * 使 SimpleBlockFeature 的 canSurvive 终判生效，避免世界生成时浮空。
 *
 * MC ID: minecraft:lily_pad
 */
class WaterlilyBlock : public BushBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit WaterlilyBlock(const BlockProperties& properties);

    ~WaterlilyBlock() noexcept override = default;

protected:
    /**
     * @brief 检查下方是否可支撑
     *
     * 下方须为水（任意水位）或冰方块，且睡莲自身位置上方无流体。
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;

    /**
     * @brief 获取植物类型 - 水生植物返回 PlantType::Water
     *
     * 睡莲为水生植物，须返回 PlantType::Water 而非继承自 BushBlock 的默认值 PlantType::Plains。
     * Block::canSustainPlant 对 Water 类型返回 false（由植物自身 canSustain/mayPlaceOn 决定支撑），
     * 若误返回 Plains 则泥土类方块会误判可支撑睡莲（与 vanilla 睡莲仅在水面/冰面存活相悖）。
     * 对齐 vanilla WaterlilyBlock 的水生语义（vanilla 1.21.11 已移除 PlantType 体系，改用
     * mayPlaceOn 直接判定；Cubium 保留 PlantType 体系故须在此声明为 Water）。
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
};

} // namespace blocks
} // namespace mc
