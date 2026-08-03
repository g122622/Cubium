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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <cstddef>
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockRaycastResult;
class Player;
class Entity;
enum class Hand : u8;
class BlockEntity;
enum class ActionResultType : u8;

namespace blocks {

/**
 * @brief 钟方块
 *
 * 可以被敲响的功能方块，会发出声音和动画。
 * 可以附着在墙、地面或天花板上。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST) - 墙面附着
 * - ATTACHMENT: 附着类型 (FLOOR, CEILING, SINGLE_WALL, DOUBLE_WALL)
 * - POWERED: 是否被激活
 *
 * 交互行为：
 * - 玩家右键敲钟：根据点击方向与附着类型判定是否为有效敲击
 * - 投射物击中：自动敲响
 * - 红石信号变化：触发自动敲响并切换 POWERED 状态
 * - 支撑失效：方块掉落为物品
 *
 * 参考: net.minecraft.world.level.block.BellBlock
 */
class BellBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit BellBlock(const BlockProperties& properties);
    ~BellBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 方块实体 ==========

    /**
     * @brief 钟有对应的方块实体
     * @return true
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建钟方块实体
     * @param pos 方块位置
     * @return 新创建的 BellBlockEntity
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键敲钟
     *
     * 判定敲击方向是否为"有效敲击"（isProperHit）：
     * - Y 轴点击且点击位置 Y > 0.8124：无效
     * - FLOOR 附着：点击方向轴 == 朝向轴 为有效
     * - SINGLE_WALL/DOUBLE_WALL 附着：点击方向轴 != 朝向轴 为有效
     * - CEILING 附着：任意水平方向点击都为有效
     *
     * 参考: net.minecraft.world.level.block.BellBlock#useWithoutItem, onHit, isProperHit
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    /**
     * @brief 投射物击中钟时自动敲响
     *
     * 参考: net.minecraft.world.level.block.BellBlock#onProjectileHit
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    /**
     * @brief 邻居方块更新（红石信号检测）
     *
     * 当红石信号变化时：
     * - 从无到有：自动敲响钟（attemptToRing）
     * - 切换 POWERED 状态
     *
     * 参考: net.minecraft.world.level.block.BellBlock#neighborChanged
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    // ========== 敲钟接口 ==========

    /**
     * @brief 尝试敲响钟（无方向参数，使用方块的 HORIZONTAL_FACING）
     *
     * 参考: net.minecraft.world.level.block.BellBlock#attemptToRing(Level, BlockPos, Direction)
     *
     * @param world 世界引用
     * @param pos 钟方块位置
     * @param direction 敲击方向，若为 Direction::Down 则使用方块的 HORIZONTAL_FACING
     * @return 如果成功敲响（服务端且方块实体存在）返回 true
     */
    bool attemptToRing(IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 尝试敲响钟（指定玩家，用于统计 BELL_RING）
     *
     * 参考: net.minecraft.world.level.block.BellBlock#attemptToRing(Entity, Level, BlockPos, Direction)
     *
     * @param player 敲钟玩家（可为 nullptr）
     * @param world 世界引用
     * @param pos 钟方块位置
     * @param direction 敲击方向
     * @return 如果成功敲响返回 true
     */
    bool attemptToRing(Player* player, IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 敲击钟（内部统一入口）
     *
     * 判定是否为有效敲击方向，若是则敲响钟并给玩家加 BELL_RING 统计。
     *
     * 参考: net.minecraft.world.level.block.BellBlock#onHit
     *
     * @param world 世界引用
     * @param state 方块状态
     * @param hit 射线检测结果
     * @param player 敲钟玩家（可为 nullptr）
     * @param isProjectile 是否由投射物触发
     * @return 如果敲响成功返回 true
     */
    bool onHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hit, Player* player, bool isProjectile);

protected:
    /// 各状态的形状缓存（按 HORIZONTAL_FACING + ATTACHMENT 组合索引）
    std::array<CollisionShape, 16> m_shapesByState{};

    /// 地面附着形状（按 X/Z 轴区分，与 HORIZONTAL_FACING 的轴对应）
    CollisionShape m_floorShapeX;
    CollisionShape m_floorShapeZ;

    /// 天花板附着形状（无方向区分）
    CollisionShape m_ceilingShape;

    /// 双面墙附着形状（按 X/Z 轴区分）
    CollisionShape m_doubleWallShapeX;
    CollisionShape m_doubleWallShapeZ;

    /// 单面墙附着形状（按 4 个水平朝向区分：North/South/East/West）
    CollisionShape m_singleWallShapeNorth;
    CollisionShape m_singleWallShapeSouth;
    CollisionShape m_singleWallShapeEast;
    CollisionShape m_singleWallShapeWest;

    /**
     * @brief 初始化形状缓存
     *
     * 构建 4 类附着形状（Floor/Ceiling/SingleWall/DoubleWall），
     * 并将它们按 HORIZONTAL_FACING + ATTACHMENT 组合填入 m_shapesByState。
     *
     * 参考: net.minecraft.world.level.block.BellBlock#BELL_SHAPE, SHAPE_CEILING,
     *       SHAPE_FLOOR, SHAPE_DOUBLE_WALL, SHAPE_SINGLE_WALL
     */
    void _initializeShapes();

    /**
     * @brief 计算 m_shapesByState 的索引
     *
     * 索引公式：(facing_index << 2) | attachment_index
     * 其中 facing_index 为 HORIZONTAL_FACING 的 0-3 索引，attachment_index 为 BellAttachment 的 0-3 枚举值。
     *
     * @param state 方块状态
     * @return 索引值（0-15）
     */
    [[nodiscard]] static size_t _shapeIndex(const BlockState& state);

    /**
     * @brief 判定是否为"有效敲击方向"
     *
     * 参考: net.minecraft.world.level.block.BellBlock#isProperHit
     *
     * @param state 方块状态
     * @param direction 点击方向（射线检测的面方向）
     * @param hitY 点击位置在方块内的 Y 坐标（0-1）
     * @return 如果为有效敲击返回 true
     */
    [[nodiscard]] static bool _isProperHit(const BlockState& state, Direction direction, f64 hitY);
};

} // namespace blocks
} // namespace mc
