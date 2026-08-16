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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {

// Forward declarations
class Block;
class BlockState;
class BlockRegistry;
class IWorld;
class IBlockReader;
class BlockPos;
class Player;
class BlockEntity;
class Entity;
class IPlantable;
class ItemStack;

namespace math {
class IRandom;
}

namespace loot {
class LootTableManager;
class LootTable;
} // namespace loot

namespace fluid {
class FluidState;
}

namespace item {
namespace tool {
// Tool type constants for harvest tool comparison
// These values must match ToolType enum in item/tool/ToolType.hpp
constexpr u8 TOOL_TYPE_NONE = 0;
constexpr u8 TOOL_TYPE_PICKAXE = 1;
constexpr u8 TOOL_TYPE_AXE = 2;
constexpr u8 TOOL_TYPE_SHOVEL = 3;
constexpr u8 TOOL_TYPE_HOE = 4;
constexpr u8 TOOL_TYPE_SWORD = 5;
constexpr u8 TOOL_TYPE_SHEARS = 6;
} // namespace tool
} // namespace item

// Forward declaration for Direction (needed for method parameters)
enum class Direction : u8;

// Forward declaration for Material (needed for getMaterial() return type)
class Material;

// Forward declaration for BlockSoundType (needed for getSoundType() return type)
class BlockSoundType;

// Forward declaration for IProperty (used in StateHolder base class)
class IProperty;

// Forward declaration for SupportType (used in isFaceSturdy parameter)
class SupportType;

/**
 * @brief 方块状态
 *
 * 不可变的方块状态对象，包含方块的所有属性值。
 * 继承自StateHolder以支持O(1)的状态转换。
 */
class BlockState : public StateHolder<Block, BlockState> {
public:
    /**
     * @brief 构造方块状态
     */
    BlockState(const Block& block,
        std::vector<size_t> valueIndices,
        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
        const std::vector<BlockState*>* allStates,
        u32 stateId);

    /**
     * @brief 是否为空气
     */
    [[nodiscard]] bool isAir() const;

    /**
     * @brief 是否应该生成地形粒子
     *
     * 对应 MC 1.21.11 net.minecraft.world.level.block.state.BlockBehaviour.BlockStateBase#shouldSpawnTerrainParticles。
     * 默认实现：!isAir()。
     * 用于刷子（BrushItem）等需要根据视线方块生成方块碎屑粒子的场景。
     *
     * @return 如果应生成地形粒子返回 true
     */
    [[nodiscard]] bool shouldSpawnTerrainParticles() const { return !isAir(); }

    /**
     * @brief 获取方块的渲染类型是否为不可见
     *
     * 委托到方块的 getRenderType 虚方法，判断是否为 INVISIBLE。
     * 用于刷子（BrushItem）判断是否生成碎屑粒子（INVISIBLE 方块不生成）。
     *
     * @return 如果方块渲染类型为 INVISIBLE 返回 true
     */
    [[nodiscard]] bool isInvisibleRenderType() const;

    /**
     * @brief 是否为固体
     */
    [[nodiscard]] bool isSolid() const { return m_isSolid; }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque() const { return m_isOpaque; }

    /**
     * @brief 是否透明
     */
    [[nodiscard]] bool isTransparent() const { return !m_isOpaque; }

    /**
     * @brief 是否阻挡移动
     */
    [[nodiscard]] bool blocksMovement() const { return m_blocksMovement; }

    /**
     * @brief 是否为液体
     */
    [[nodiscard]] bool isLiquid() const { return m_isLiquid; }

    /**
     * @brief 是否可燃
     */
    [[nodiscard]] bool isFlammable() const { return m_isFlammable; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }

    /**
     * @brief 检查此状态是否属于指定方块
     * @param block 方块指针
     * @return 如果此状态的方块与给定方块相同则返回true
     */
    [[nodiscard]] bool is(const Block* block) const { return block != nullptr && &owner() == block; }

    /**
     * @brief 获取此状态所属的方块
     * @return 方块的const引用
     */
    [[nodiscard]] const Block& getBlock() const { return owner(); }

    /**
     * @brief 获取此状态所属方块的可变引用
     *
     * Block 对象在 BlockRegistry 中注册，生命周期与程序相同，
     * 且 BlockRegistry 以非const方式持有 Block。此方法提供了从
     * BlockState 获取非const Block 引用的安全途径，避免了调用方
     * 需要手动 const_cast 的不便。
     *
     * 适用场景：调用 Block 的非const虚方法（如 tick、onBlockAdded、
     * onBlockRemoved、neighborChanged、scheduleBlockTick 等），
     * 这些方法在语义上不修改 Block 对象自身的状态，但由于 C++
     * 虚方法机制需要非const引用。
     *
     * @return 方块的可变引用
     */
    [[nodiscard]] Block& getBlockMutable() const { return const_cast<Block&>(owner()); }

    /**
     * @brief 获取光照透明度 (0-15)
     */
    [[nodiscard]] i32 getOpacity() const { return m_opacity; }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool propagatesSkylightDown() const { return m_propagatesSkylightDown; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const { return m_resistance; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const { return m_blockId; }

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape() const;

    /**
     * @brief 获取渲染形状
     */
    [[nodiscard]] const CollisionShape& getShape() const;

    /**
     * @brief 获取遮挡形状
     */
    [[nodiscard]] const CollisionShape& getOcclusionShape() const;

    /**
     * @brief 获取方块支撑形状
     *
     * 支撑形状用于判断方块面是否足够坚固以支撑其他方块放置。
     * 默认返回碰撞形状。某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，
     * 但支撑形状是完整方块。
     *
     * 参考: net.minecraft.block.BlockStateBase#getBlockSupportShape
     *
     * @return 支撑形状引用
     */
    [[nodiscard]] const CollisionShape& getBlockSupportShape() const;

    /**
     * @brief 获取指定面的遮挡形状
     *
     * 用于光照遮挡检测。返回方块在指定方向上的投影形状。
     *
     * @param direction 方向
     * @return 面遮挡形状
     */
    [[nodiscard]] CollisionShape getFaceOcclusionShape(Direction direction) const;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * @return 是否使用形状进行光照遮挡
     */
    [[nodiscard]] bool useShapeForLightOcclusion() const { return m_useShapeForLightOcclusion; }

    /**
     * @brief 是否为"条件完全遮挡"方块（StarLight 专用）
     *
     * 对齐 Moonrise StarLightEngine 的 isConditionallyFullOpaque：
     *   isConditionallyFullOpaque = canOcclude && useShapeForLightOcclusion
     * 其中 vanilla canOcclude = useShapeForLightOcclusion && opacity >= 15，
     * 故等价于：useShapeForLightOcclusion && opacity >= MAX_LIGHT_LEVEL。
     *
     * 只有"用形状遮挡"且"完全不透明(opacity>=15)"的方块（如条件完整的实心方块）才会走
     * StarLight 的条件透明（sided transparent）复杂传播路径。非完整方块（火把、栅栏、
     * 台阶等）虽 useShapeForLightOcclusion=true，但 opacity<15，isConditionallyFullOpaque
     * 为 false，应走普通简单传播路径——它们自身的形状遮挡已通过 getFaceOcclusionShape 在
     * 传播时单独判定，不需要整块走复杂路径。
     *
     * 若误用 useShapeForLightOcclusion 代替本方法，火把等方块会错误进入复杂路径，导致
     * 其发射的方块光无法正常传播到相邻空气格（如火把(14)邻格应得13却得0）。
     *
     * 参考: ca.spottedleaf.moonrise.patches.starlight.blockstate.StarlightAbstractBlockState
     */
    [[nodiscard]] bool isConditionallyFullOpaque() const { return m_useShapeForLightOcclusion && m_opacity >= 15; }

    /**
     * @brief 是否有不透明碰撞形状
     *
     * 用于环境光遮蔽(AO)计算。如果方块有不透明的完整碰撞箱，
     * 则返回true，导致周围顶点变暗。
     */
    [[nodiscard]] bool hasOpaqueCollisionShape() const;

    /**
     * @brief 获取环境光遮蔽亮度值
     *
     * 委托到方块的 getShadeBrightness 方法。
     * 返回值用于AO计算：
     * - 0.2f: 方块有不透明碰撞形状（实心方块），产生阴影
     * - 1.0f: 方块无碰撞或透明（玻璃、树叶等），不产生阴影
     *
     * 子类可通过重写 getShadeBrightness 改变此行为。
     */
    [[nodiscard]] f32 getAmbientOcclusionLightValue() const;

    /**
     * @brief 获取方块的遮光亮度
     *
     * 委托到方块的 getShadeBrightness 虚方法。
     * 环境光遮蔽（AO）计算使用此值确定方块对周围顶点亮度的贡献。
     *
     * @return 遮光亮度 (0.0F-1.0F)
     */
    [[nodiscard]] f32 getShadeBrightness() const;

    /**
     * @brief 检查指定面是否为实体面
     *
     * 用于流体流动判断、渲染面剔除等。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param side 要检查的面
     * @return 如果该面是实体面返回true
     */
    [[nodiscard]] bool isSolidSide(IWorld& world, const BlockPos& pos, Direction side) const;

    /**
     * @brief 检查方块的碰撞形状在指定方向的面是否完全填充
     *
     * 用于判断方块面是否足够坚固以支撑其他方块放置（如雪层放置判断）。
     * 提取碰撞形状在指定方向的面投影，然后判断投影是否覆盖整个单位方块面。
     *
     * 参考: net.minecraft.block.BlockStateBase#isFaceSturdy(Direction, SupportType.FULL)
     *
     * @param direction 要检查的面方向
     * @return 如果面的投影覆盖整个 1x1 区域返回 true
     */
    [[nodiscard]] bool isFaceFull(Direction direction) const;

    /**
     * @brief 检查方块的支撑形状在指定方向是否提供指定类型的支撑
     *
     * 对应 MC 1.21.11 net.minecraft.world.level.block.state.BlockBehaviour.BlockStateBase#isFaceSturdy。
     * 判定基于方块的 BlockSupportShape（支撑形状），而非碰撞形状。
     *
     * 三种支撑类型：
     * - SupportType::Full：方块面投影必须覆盖整个 1×1 面
     * - SupportType::Center：方块面投影必须包含中心 2×2 像素到 10×10 像素的"中心柱"区域
     * - SupportType::Rigid：方块面投影必须覆盖 1×1 面除中心 12×12 像素柱以外的外环区域
     *
     * 参考: net.minecraft.block.BlockStateBase#isFaceSturdy(BlockGetter, BlockPos, Direction, SupportType)
     *
     * @param world 世界接口
     * @param pos 方块位置
     * @param direction 要检查的面方向
     * @param supportType 支撑类型
     * @return 如果提供指定类型的支撑返回 true
     */
    [[nodiscard]] bool isFaceSturdy(
        IWorld& world, const BlockPos& pos, Direction direction, const SupportType& supportType) const;

    /**
     * @brief 检查是否为不透明完整方块
     *
     * 用于渲染和光照计算。
     *
     * @param world 世界
     * @param pos 方块位置
     * @return 如果是不透明完整方块返回true
     */
    [[nodiscard]] bool isOpaqueCube(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 获取方块资源位置
     */
    [[nodiscard]] const ResourceLocation& blockLocation() const;

    /**
     * @brief 获取流体状态
     *
     * 委托到方块的 getFluidState 方法
     */
    [[nodiscard]] const fluid::FluidState* getFluidState() const;

    /**
     * @brief 获取材质
     *
     * 委托到方块的 material 方法
     */
    [[nodiscard]] const Material& getMaterial() const;

    /**
     * @brief 获取声音类型
     *
     * 委托到方块的 getSoundType 方法
     */
    [[nodiscard]] const BlockSoundType& getSoundType() const;

    /**
     * @brief 获取挖掘工具类型
     *
     * 返回采集此方块所需的工具类型。
     * 返回值与 item::tool 命名空间中的常量比较：
     * - TOOL_TYPE_NONE (0): 无需工具
     * - TOOL_TYPE_PICKAXE (1): 镐
     * - TOOL_TYPE_AXE (2): 斧
     * - TOOL_TYPE_SHOVEL (3): 锹
     * - TOOL_TYPE_HOE (4): 锄
     * - TOOL_TYPE_SWORD (5): 剑
     * - TOOL_TYPE_SHEARS (6): 剪刀
     *
     * @return 工具类型值
     */
    [[nodiscard]] u8 getHarvestTool() const;

    /**
     * @brief 获取挖掘等级
     *
     * 返回采集此方块所需的最低工具等级。
     * - 0: 木/金工具可采集
     * - 1: 石制工具可采集
     * - 2: 铁制工具可采集
     * - 3: 钻石工具可采集
     * - 4: 下界合金工具可采集
     *
     * @return 挖掘等级（0-4）
     */
    [[nodiscard]] i32 getHarvestLevel() const;

    /**
     * @brief 获取地图颜色
     *
     * 委托到方块的 getMapColor 方法。
     * 用于在地图上渲染此方块的颜色。
     *
     * @param world 世界（可选，用于生物群系感知）
     * @param pos 位置（可选）
     * @return 地图颜色ID
     */
    [[nodiscard]] world::map::MaterialColorId getMapColor(IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

    /**
     * @brief 检查工具是否有效
     *
     * 检查指定工具类型和等级是否能采集此方块。
     *
     * @param toolType 工具类型值
     * @param harvestLevel 工具挖掘等级
     * @return 如果工具有效返回true
     */
    [[nodiscard]] bool isToolEffective(u8 toolType, i32 harvestLevel) const;

    /**
     * @brief 是否需要正确工具才能采集
     *
     * 如果返回 true，则必须使用正确类型且等级足够的工具
     * 才能获得方块掉落物。
     *
     * @return 是否需要正确工具
     */
    [[nodiscard]] bool requiresTool() const;

    /**
     * @brief 检查是否为粘性方块
     *
     * 委托到方块的 isStickyBlock 方法。
     *
     * @return 如果是粘性方块返回 true
     */
    [[nodiscard]] bool isStickyBlock() const;

    /**
     * @brief 检查此方块状态是否可被替换
     *
     * 对应 MC 的 BlockState.canBeReplaced()，返回方块注册时设置的 replaceable 属性。
     * 空气、水、岩浆、花草、火等方块为 true；石头、泥土等实心方块为 false。
     * 此方法等价于 isAir() || getMaterial().isReplaceable()，但性能更优（缓存值）。
     *
     * 用于：世界生成谓词(ReplaceablePredicate)、掉落方块判断、AI寻路等场景。
     * 不适用于：玩家放置时的替换判断（应使用 Block::isReplaceable(state, context)）。
     */
    [[nodiscard]] bool canBeReplaced() const { return m_canBeReplaced; }

    /**
     * @brief 检查此方块状态是否可被流体替换
     *
     * 对应 MC 的 BlockState.canBeReplaced(Fluid)，默认实现为 canBeReplaced() || !isSolid()。
     * 非固体方块即使不是 replaceable，也可以被流体流入（如门、告示牌等）。
     *
     * 用于：流体流动判断、桶放置流体等场景。
     * 方块子类可重写 Block::canBeReplacedByFluid() 来拒绝流体替换（如末地传送门）。
     */
    [[nodiscard]] bool canBeReplacedByFluid() const;

    /**
     * @brief 检查两个方块是否可以粘连
     *
     * 委托到方块的 canStickTo 方法。
     *
     * @param other 目标方块状态
     * @return 如果可以粘连返回 true
     */
    [[nodiscard]] bool canStickTo(const BlockState& other) const;

    // ========================================================================
    // 火焰相关
    // ========================================================================

    /**
     * @brief 获取方块的可燃性值
     *
     * 委托到方块的 getFlammability 方法。
     *
     * @param world 世界（可选）
     * @param pos 方块位置（可选）
     * @param face 点燃面（可选）
     * @return 可燃性值 (0-300)
     */
    [[nodiscard]] i32 getFlammability(
        IWorld* world = nullptr, const BlockPos* pos = nullptr, Direction face = static_cast<Direction>(255)) const;

    /**
     * @brief 获取方块的火焰蔓延速度
     *
     * 委托到方块的 getFireSpreadSpeed 方法。
     *
     * @param world 世界（可选）
     * @param pos 方块位置（可选）
     * @param face 蔓延面（可选）
     * @return 火焰蔓延速度
     */
    [[nodiscard]] i32 getFireSpreadSpeed(
        IWorld* world = nullptr, const BlockPos* pos = nullptr, Direction face = static_cast<Direction>(255)) const;

    /**
     * @brief 检查方块是否为火焰源
     *
     * 委托到方块的 isFireSource 方法。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param side 火焰所在面
     * @return 如果是火源返回 true
     */
    [[nodiscard]] bool isFireSource(IWorld& world, const BlockPos& pos, Direction side) const;

    /**
     * @brief 方块被点燃时的回调
     *
     * 委托到方块的 catchFire 方法。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param face 点燃面
     * @param igniter 点燃者（可能为空）
     */
    void catchFire(IWorld& world,
        const BlockPos& pos,
        Direction face = static_cast<Direction>(255),
        Entity* igniter = nullptr) const;

    /**
     * @brief 转换为模型键（用于查找模型变体）
     * @return 格式: "axis=y,facing=north" 或 "" (无属性时)
     */
    [[nodiscard]] std::string toModelKey() const;

protected:
    /**
     * @brief 获取拥有者名称
     */
    [[nodiscard]] std::string ownerName() const override;

private:
    friend class Block;
    friend class BlockRegistry;

    /**
     * @brief 缓存方块属性
     */
    void _cacheProperties();

    // 缓存的属性
    bool m_isSolid = true;
    bool m_isOpaque = true;
    bool m_blocksMovement = false;
    bool m_isLiquid = false;
    bool m_isFlammable = false;
    bool m_canBeReplaced = false; // 是否可被替换（缓存自 Block.m_isReplaceable）
    bool m_propagatesSkylightDown = false;
    bool m_useShapeForLightOcclusion = false; // 是否使用形状进行光照遮挡
    u8 m_lightLevel = 0;
    u8 m_harvestTool = 0; // HarvestTool::None
    i32 m_opacity = 15;   // 默认完全不透明
    i32 m_harvestLevel = 0;
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u32 m_blockId = 0;
    world::map::MaterialColorId m_mapColor = world::map::MaterialColorId::AIR;
};

} // namespace mc
