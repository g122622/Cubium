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

/**
 * @file path_scene.hpp
 * @brief 寻路差分探针共用的「可写方块表 + 受控实体」场景。
 *
 * 与原版 oracle 的关系：
 *   - 两侧用**同一份**方块描述字符串（原版命令的方块状态语法，如
 *     `minecraft:oak_stairs[facing=east,half=top]`）布置场景；
 *   - 两侧用**同一套**实体参数（尺寸、抬脚高度、代价表、能力开关）；
 *   - 于是差异只可能来自被测的求值器 / 寻路器本身。
 *
 * 这里的 `SceneRegion` 刻意**不**复用生产 `WorldRegion`：后者要求一个完整 `IWorld`，
 * 而差分只需要「按坐标查方块」。用内存方块表可以让场景完全受控且构建极快，
 * 同时仍然走生产的 `WalkNodeEvaluator`，不引入任何被测代码的替身。
 */

#include "common/command/StringReader.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/NodeEvaluator.hpp"
#include "common/entity/ai/pathfinding/PathType.hpp"
#include "common/entity/ai/pathfinding/Region.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {

/**
 * @brief 内存方块表区域
 *
 * 未显式设置的格子一律是空气。碰撞查询按每个方块的碰撞形状逐格求交——与生产路径
 * 用的是同一份形状数据，因此「能不能站上去」「能不能穿过去」与游戏内一致。
 */
class SceneRegion final : public Region {
public:
    static u64 key(i32 x, i32 y, i32 z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 42) ^ (static_cast<u64>(static_cast<u32>(y)) << 21) ^
            static_cast<u64>(static_cast<u32>(z));
    }

    /**
     * @brief 设置一格的方块
     * @param block 方块描述字符串（原版命令方块状态语法）
     */
    void setBlock(i32 x, i32 y, i32 z, const std::string& block)
    {
        command::StringReader reader(block);
        m_blocks[key(x, y, z)] = command::BlockStateArgumentType::blockState()->parse(reader).state();
    }

    /// 清空全部格子（把区域还原成纯空气）。
    void clear()
    {
        m_blocks.clear();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(key(x, y, z));
        return found != m_blocks.end() ? found->second : &VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    /**
     * @brief 世界坐标盒子是否与方块碰撞形状相交
     *
     * 原版 `noCollision` 只看方块碰撞形状（不含实体互相碰撞），这里与之对应。
     */
    [[nodiscard]] bool hasCollision(const void* /*entity*/, const AxisAlignedBB& box) const override
    {
        const i32 minX = static_cast<i32>(std::floor(box.minX));
        const i32 maxX = static_cast<i32>(std::ceil(box.maxX) - 1.0f);
        const i32 minY = static_cast<i32>(std::floor(box.minY));
        const i32 maxY = static_cast<i32>(std::ceil(box.maxY) - 1.0f);
        const i32 minZ = static_cast<i32>(std::floor(box.minZ));
        const i32 maxZ = static_cast<i32>(std::ceil(box.maxZ) - 1.0f);
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 y = minY; y <= maxY; ++y) {
                for (i32 z = minZ; z <= maxZ; ++z) {
                    const BlockState* state = getBlockState(x, y, z);
                    if (state->isAir()) {
                        continue;
                    }
                    const CollisionShape& shape = state->getCollisionShape();
                    for (const AxisAlignedBB& part : shape.boxes()) {
                        const AxisAlignedBB world(part.minX + static_cast<f32>(x),
                            part.minY + static_cast<f32>(y),
                            part.minZ + static_cast<f32>(z),
                            part.maxX + static_cast<f32>(x),
                            part.maxY + static_cast<f32>(y),
                            part.maxZ + static_cast<f32>(z));
                        if (box.intersects(world)) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    [[nodiscard]] i32 minBuildHeight() const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] i32 maxBuildHeight() const override
    {
        return world::MAX_BUILD_HEIGHT;
    }

private:
    std::unordered_map<u64, const BlockState*> m_blocks;
};

/**
 * @brief 受控寻路实体
 *
 * 所有属性都来自场景输入；`pathfindingMalus` 与原版 `Mob#getPathfindingMalus`
 * 同构：显式设置过就返回设置值，否则回退到 `PathType` 的默认代价。
 */
class SceneMob final : public entity::ai::pathfinding::PathfindingEntity {
public:
    SceneMob(f32 width, f32 height, f32 stepHeight, bool onGround, bool inWater, i32 maxFallDistance)
        : m_width(width)
        , m_height(height)
        , m_stepHeight(stepHeight)
        , m_onGround(onGround)
        , m_inWater(inWater)
        , m_maxFallDistance(maxFallDistance)
    {
        m_malus.fill(UNSET);
    }

    void setPosition(f64 x, f64 y, f64 z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    /// 按 `PathType` 设代价；值为 `NaN` 表示「回退到默认代价」。
    void setMalus(entity::ai::pathfinding::PathType type, f32 value)
    {
        m_malus[static_cast<size_t>(type)] = value;
    }

    [[nodiscard]] f32 pathfindingWidth() const override
    {
        return m_width;
    }

    [[nodiscard]] f32 pathfindingHeight() const override
    {
        return m_height;
    }

    [[nodiscard]] f32 pathfindingStepHeight() const override
    {
        return m_stepHeight;
    }

    [[nodiscard]] i32 pathfindingMaxFallDistance() const override
    {
        // 与原版 Mob#getMaxFallDistance 同构：输入未给（-1）时回退到「舒适下落距离」
        // （LivingEntity#getComfortableFallDistance(0) = floor(0 + 3)）。
        return m_maxFallDistance >= 0 ? m_maxFallDistance : static_cast<i32>(std::floor(0.0f + 3.0f));
    }

    [[nodiscard]] f32 pathfindingMalus(entity::ai::pathfinding::PathType type) const override
    {
        const f32 stored = m_malus[static_cast<size_t>(type)];
        return stored == UNSET ? entity::ai::pathfinding::getPathMalus(type) : stored;
    }

    [[nodiscard]] AxisAlignedBB pathfindingBoundingBox() const override
    {
        const f32 half = m_width / 2.0f;
        return AxisAlignedBB(static_cast<f32>(m_x) - half,
            static_cast<f32>(m_y),
            static_cast<f32>(m_z) - half,
            static_cast<f32>(m_x) + half,
            static_cast<f32>(m_y) + m_height,
            static_cast<f32>(m_z) + half);
    }

    [[nodiscard]] bool pathfindingOnGround() const override
    {
        return m_onGround;
    }

    [[nodiscard]] bool pathfindingInWater() const override
    {
        return m_inWater;
    }

    [[nodiscard]] bool pathfindingCanStandOnFluid(const fluid::FluidState* /*fluidState*/) const override
    {
        return false;
    }

private:
    /// 「未设置」哨兵：与任何真实代价都不相等（包括 NaN 与负值）。
    static constexpr f32 UNSET = 1.0e30f;

    f32 m_width;
    f32 m_height;
    f32 m_stepHeight;
    bool m_onGround;
    bool m_inWater;
    i32 m_maxFallDistance;
    f64 m_x = 0.0;
    f64 m_y = 0.0;
    f64 m_z = 0.0;
    std::array<f32, entity::ai::pathfinding::pathTypeCount()> m_malus{};
};

/**
 * @brief 从 JSON 读一个三维坐标（相对场景原点）
 */
inline void readRelative(const nlohmann::json& object, const char* key, i32& outX, i32& outY, i32& outZ)
{
    const auto& value = object.at(key);
    outX = value.at("x").get<i32>();
    outY = value.at("y").get<i32>();
    outZ = value.at("z").get<i32>();
}

/**
 * @brief 解析 `PathType` 名字（小写蛇形，与原版枚举名一致）
 *
 * 两侧枚举次序不同，名字才是稳定口径；这里按名字反查本项目的枚举值。
 */
inline entity::ai::pathfinding::PathType parsePathType(const std::string& name)
{
    for (size_t index = 0; index < entity::ai::pathfinding::pathTypeCount(); ++index) {
        const auto type = static_cast<entity::ai::pathfinding::PathType>(index);
        if (name == entity::ai::pathfinding::getPathTypeName(type)) {
            return type;
        }
    }
    throw std::invalid_argument("未知的路径类型名：" + name);
}

/**
 * @brief 把场景输入里的方块摆进区域，并构造实体
 *
 * 两个 oracle/探针共用同一段布局逻辑，保证「读同一份 JSON 得到同一个场景」。
 */
struct Scene {
    SceneRegion region;
    std::unique_ptr<SceneMob> mob;
    i32 originX = 0;
    i32 originY = 0;
    i32 originZ = 0;
    i32 sizeX = 0;
    i32 sizeY = 0;
    i32 sizeZ = 0;
    bool canPassDoors = true;
    bool canOpenDoors = false;
    bool canFloat = false;
    bool canWalkOverFences = false;
    Vector3d mobPosition;
};

} // namespace mc::diff
