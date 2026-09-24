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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/util/math/random/Random.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/SimpleBlock.hpp"
#include "common/world/gen/feature/nether/FungusType.hpp"
#include <functional>

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 下界菌方块（绯红菌/诡异菌）
 *
 * 可种植在对应菌岩上。对其使用骨粉时有 40% 概率长成巨型真菌
 * （HugeFungusFeature）。
 *
 * wiki tech_下界菌.txt#骨粉：
 *   对种植在对应菌岩上的下界菌使用骨粉，可使其长成巨型真菌。
 *
 * 参考: net.minecraft.world.level.block.FungusBlock
 */
class FungusBlock : public SimpleBlock, public IGrowable {
public:
    /**
     * @brief 巨型真菌生成器函数类型
     *
     * 接收 IWorld& —— 实际运行时由 ServerWorld::createFeatureRegion()
     * 返回的 WorldGenRegion（继承自 IWorld）。lambda 内部在 server 侧
     * 将 IWorld& static_cast 为 WorldGenRegion& 后调用 HugeFungusFeature::place()。
     *
     * @param world 临时构建的区域（以 IWorld 接口暴露）
     * @param pos 真菌位置
     * @param random 随机数生成器
     */
    using FungusGrower = std::function<void(IWorld&, const BlockPos&, math::IRandom&)>;

    /**
     * @brief 构造函数
     * @param fungusType 真菌类型（绯红/诡异）
     * @param properties 方块属性
     */
    FungusBlock(FungusType fungusType, const BlockProperties& properties);

    ~FungusBlock() override = default;

    /**
     * @brief 后置注入巨型真菌生成器
     *
     * common 层方块注册时无法引用 server gen 的 HugeFungusFeature，
     * 因此构造时 fungusGrower 为空，由 server 侧初始化阶段调用此方法
     * 注入真实的 FungusGrower（捕获 FungusType 并调用 HugeFungusFeature::place()）。
     */
    void setFungusGrower(FungusGrower fungusGrower) { m_fungusGrower = std::move(fungusGrower); }

    // ========== IGrowable 接口 ==========

    /**
     * @brief 检查是否可以骨粉生长
     *
     * 条件：下方方块为对应菌岩（绯红菌→绯红菌岩，诡异菌→诡异菌岩）。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 检查骨粉是否成功
     *
     * wiki :骨粉 40% 概率成功（与 Java 版 isBonemealSuccess 一致）。
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉长成巨型真菌
     *
     * 通过 IWorld::createFeatureRegion() 构建临时区域（以 IWorld 接口返回），
     * 然后调用 FungusGrower 进行巨型真菌生成。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

private:
    /// 真菌类型
    FungusType m_fungusType;

    /// 巨型真菌生成器回调（由 server 侧后置注入）
    FungusGrower m_fungusGrower;
};

} // namespace blocks
} // namespace mc
