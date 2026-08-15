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

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;
class BlockEntity;
class VoxelShape;

namespace blocks {

/**
 * @brief 营火方块
 *
 * 营火是一种多功能方块：
 * - 光源：发出15级光照（点燃时）
 * - 烹饪：可以烹饪食物（最多4个）
 * - 烟雾：产生向上飘的烟雾粒子
 * - 伤害：站在上面会造成伤害
 * - 实现 IWaterLoggable 接口支持含水功能
 *
 * 状态属性：
 * - LIT: 是否点燃
 * - SIGNAL_FIRE: 是否为信号火（添加烟雾高度）
 * - WATERLOGGED: 是否被水淹没
 * - FACING: 水平朝向（北、东、南、西）
 *
 * 方块实体：CampfireBlockEntity
 *
 * 注意：营火没有 AGE 属性，也不会因为雨天而熄灭。
 * 营火的熄灭方式：
 * 1. 水接触（含水）
 * 2. 铲子右键
 * 3. 喷溅型水瓶
 */
class CampfireBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param lightValue 光照等级（普通营火=15，灵魂营火=10）
     */
    explicit CampfireBlock(BlockProperties properties, u8 lightValue = 15);
    ~CampfireBlock() override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 方块实体 ==========

    /**
     * @brief 检查是否有方块实体
     * @return 返回true，营火有方块实体
     */
    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 光照 ==========

    /**
     * @brief 获取动态光照等级
     *
     * 点燃时发出光照，熄灭时不发光。
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 光照等级 (0 或配置值)
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键交互
     *
     * 可以向营火添加食物进行烹饪。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 实体碰撞 ==========

    /**
     * @brief 实体站在点燃的营火上时造成火焰伤害
     *
     * 对齐 wiki：点燃的营火每游戏刻对位于方块中的生物造成火焰伤害
     * （普通营火 hp1，灵魂营火 hp2），但受击后伤害免疫使生物每半秒
     * （10 tick）实际承受一次。穿冰霜行者靴子免疫。仅在服务端生效。
     * 注意：营火不再引燃实体（1.19.60+ 移除 setOnFire），只走 hurt。
     *
     * Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_营火.txt#伤害
     * Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_灵魂营火.txt#伤害
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

    // ========== 方块移除 ==========

    /**
     * @brief 方块被移除时调用
     *
     * 掉落营火中的所有物品。
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 旋转/镜像 ==========

    /**
     * @brief 旋转方块状态
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块状态
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 工具方法 ==========

    /**
     * @brief 是否点燃
     */
    [[nodiscard]] static bool isLit(const BlockState& state) { return state.get(BlockStateProperties::LIT()); }

    /**
     * @brief 是否为信号火
     */
    [[nodiscard]] static bool isSignalFire(const BlockState& state)
    {
        return state.get(BlockStateProperties::SIGNAL_FIRE());
    }

    /**
     * @brief 获取营火朝向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state)
    {
        return state.get(BlockStateProperties::HORIZONTAL_FACING());
    }

    /**
     * @brief 点燃营火
     */
    static void light(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 熄灭营火
     */
    static void extinguish(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 检查指定位置下方是否有点燃的营火（烟熏效果）
     * @param world 世界引用
     * @param pos 待检测位置
     * @return 下方5格内是否有点燃的营火
     *
     * 从指定位置向下检查1-5格，如果发现点燃的营火则返回true。
     * 用于蜂巢判断蜜蜂是否被营火烟熏安抚。
     */
    [[nodiscard]] static bool isSmokeyPos(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块状态是否为点燃的营火
     * @param state 方块状态
     * @return 是否为点燃的营火
     */
    [[nodiscard]] static bool isLitCampfire(const BlockState& state);

    /**
     * @brief 获取虚拟烟雾柱形状
     *
     * 返回一个 4x16x4 像素（方块本地坐标 0.375x1.0x0.375）的中心柱形状，
     * 用于 isSmokeyPos 检测烟雾是否被方块碰撞形状阻挡。
     * 对应 MC Java 的 CampfireBlock.SHAPE_VIRTUAL_POST = Block.column(4.0, 0.0, 16.0)。
     *
     * @return 虚拟烟雾柱的体素形状
     */
    [[nodiscard]] static const VoxelShape& getVirtualPostShape();

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

protected:
    /// 营火形状
    CollisionShape m_shape;
    /// 光照等级（普通=15，灵魂=10）
    u8 m_lightValue;

    /**
     * @brief 检查下方是否是干草块
     * @param world 世界
     * @param pos 营火位置
     * @return 如果下方是干草块返回true
     */
    [[nodiscard]] bool _isHayBlock(IWorld& world, const BlockPos& pos) const;
};

/**
 * @brief 灵魂营火方块
 *
 * 灵魂营火是营火的变种：
 * - 光源：发出10级光照（比普通营火暗）
 * - 催化：可以重生物魂土上的生物
 * - 蓝色火焰：视觉效果不同于普通营火
 */
class SoulCampfireBlock : public CampfireBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SoulCampfireBlock(BlockProperties properties);
    ~SoulCampfireBlock() override = default;
};

} // namespace blocks
} // namespace mc
