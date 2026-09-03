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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/PlantType.hpp"
#include <functional>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class WorldGenRegion;

namespace blocks {

/**
 * @brief 蘑菇方块基类
 *
 * 可放置在草地、泥土、菌岩等上的小型蘑菇。
 * 在黑暗环境中可以生长成巨型蘑菇。
 *
 * wiki tech_蘑菇.txt#巨型蘑菇（:84-87）：
 *   蘑菇满足以下条件时，对其使用骨粉有概率使之成长为对应的巨型蘑菇：
 *   - 种植在适当的方块（泥土、砂土、草方块）上且亮度等级低于13，
 *     或在任意亮度等级下种植在菌丝体、灰化土或菌岩上。
 *   - 生长空间充足（长宽各7格，高6-8格）。
 *
 * 参考: net.minecraft.block.MushroomBlock
 */
class MushroomBlock : public Block, public IPlantable, public IGrowable {
public:
    /**
     * @brief 巨型蘑菇生成器函数类型
     *
     * 接收 WorldGenRegion& 而非 IWorld&，以便直接调用
     * BigMushroomFeature::place() 等需要 WorldGenRegion 的生成方法。
     *
     * @param world 临时构建的 WorldGenRegion
     * @param pos 蘑菇位置
     * @param random 随机数生成器
     */
    using BigMushroomGenerator = std::function<void(WorldGenRegion&, const BlockPos&, math::Random&)>;

    /**
     * @brief 构造函数
     * @param bigMushroomGenerator 巨型蘑菇生成器回调
     * @param properties 方块属性
     */
    MushroomBlock(BigMushroomGenerator bigMushroomGenerator, const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~MushroomBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口 ==========

    /**
     * @brief 检查是否可以生长（骨粉可用）
     *
     * wiki tech_蘑菇.txt#巨型蘑菇（:84-87）：骨粉有概率生成巨型蘑菇。
     * 条件：下方为有效地面且生长空间充足。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 检查是否可以使用骨粉
     *
     * wiki :84 骨粉有概率生成巨型蘑菇（约40%概率）。
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长成巨型蘑菇
     *
     * wiki :84 满足条件时骨粉生成巨型蘑菇。
     * 通过 IWorld::createFeatureRegion() 构建临时 WorldGenRegion，
     * 然后调用 BigMushroomGenerator 进行巨型蘑菇生成。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

protected:
    /// 巨型蘑菇生成器回调
    BigMushroomGenerator m_bigMushroomGenerator;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

protected:
    /// 蘑菇形状
    CollisionShape m_shape;

    // ========== IPlantable 接口实现 ==========

    /**
     * @brief 获取植物类型 - 蘑菇返回 PlantType::Cave
     */
    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 获取植物方块状态
     */
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;
};

/**
 * @brief 巨型蘑菇方块
 *
 * 巨型蘑菇的组成方块，有不同的纹理面。
 * 用于棕色和红色巨型蘑菇。
 *
 * 状态属性：
 * - DOWN/UP/NORTH/SOUTH/EAST/WEST: 各面的纹理类型
 *
 * 参考: net.minecraft.block.HugeMushroomBlock
 */
class HugeMushroomBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit HugeMushroomBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~HugeMushroomBlock() override = default;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;
};

} // namespace blocks
} // namespace mc
