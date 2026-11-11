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

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/PathComputationType.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/physics/collision/EntityCollisionContext.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/BlockRenderType.hpp"
#include "common/world/block/base/types/HarvestTool.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/properties/BlockProperties.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "world/biome/BiomeClimate.hpp"
#include "world/map/MaterialColor.hpp"
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc {

// Forward declarations (BlockState is already declared in BlockState.hpp)
class BlockRegistry;
class World;
class BlockItemUseContext;
class Player;
class BlockEntity;
class Entity;
class IPlantable;  // 前向声明植物接口
class ItemStack;   // 前向声明物品堆
class SupportType; // 前向声明支撑类型

namespace entity {
class EntityType; // 前向声明实体类型（isValidSpawn 的区分参数）
} // namespace entity
enum class ContainerType : u8; // 前向声明容器类型（定义于 entity/inventory/ContainerTypes.hpp）

namespace world::explosion {
class Explosion;
} // namespace world::explosion

namespace math {
class IRandom;
}

namespace loot {
class LootTableManager;
class LootTable;
} // namespace loot

namespace fluid {
class FluidState;
} // namespace fluid

/**
 * @brief 方块基类
 *
 * 所有方块类型的基类。方块通过BlockRegistry注册，
 * 每个方块有一个或多个BlockState表示不同状态。
 *
 *
 * 用法示例:
 * @code
 * class StoneBlock : public Block {
 * public:
 *     StoneBlock() : Block(BlockProperties(Material::ROCK).hardness(1.5f)) {
 *         auto container = StateContainer<Block, BlockState>::Builder(*this)
 *             .create([](const Block& block, auto values, const std::vector<StateHolder<Block,
 * BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) { return
 * std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
 *             });
 *         createBlockState(std::move(container));
 *     }
 * };
 * @endcode
 */
class Block {
public:
    virtual ~Block() = default;

    // ========================================================================
    // 静态方法
    // ========================================================================

    /**
     * @brief 根据方块ID获取方块
     */
    [[nodiscard]] static Block* getBlock(u32 blockId);

    /**
     * @brief 根据资源位置获取方块
     */
    [[nodiscard]] static Block* getBlock(const ResourceLocation& id);

    /**
     * @brief 根据状态ID获取方块状态
     */
    [[nodiscard]] static BlockState* getBlockState(u32 stateId);

    /**
     * @brief 遍历所有方块
     */
    static void forEachBlock(std::function<void(Block&)> callback);

    /**
     * @brief 遍历所有方块状态
     */
    static void forEachBlockState(std::function<void(const BlockState&)> callback);

    // ========================================================================
    // 方块属性
    // ========================================================================

    /**
     * @brief 获取方块资源位置
     */
    [[nodiscard]] const ResourceLocation& blockLocation() const noexcept
    {
        return m_blockLocation;
    }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const noexcept
    {
        return m_blockId;
    }

    /**
     * @brief 获取材质
     */
    [[nodiscard]] const Material& material() const noexcept
    {
        return *m_material;
    }

    /**
     * @brief 获取状态容器
     */
    [[nodiscard]] const StateContainer<Block, BlockState>& stateContainer() const noexcept
    {
        return *m_stateContainer;
    }

    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] const BlockState& defaultState() const noexcept
    {
        return *m_defaultState;
    }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const noexcept
    {
        return m_hardness;
    }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const noexcept
    {
        return m_resistance;
    }

    /** @brief 覆写的窒息谓词；nullptr 表示沿用默认判定。 */
    [[nodiscard]] const StatePredicate* suffocatingPredicate() const noexcept
    {
        return m_isSuffocating;
    }

    /**
     * @brief 覆写的视线谓词；nullptr 表示沿用默认判定。
     *
     * 原版把 `isViewBlocking` 默认为 `isSuffocating` 的同一个实例，因此只覆写窒息谓词时
     * 视线判定也随之改变；这里 `Block::isViewBlockingAt` 在视线谓词为空时转调
     * `Block::isSuffocatingAt`，保持同一语义。
     */
    [[nodiscard]] const StatePredicate* viewBlockingPredicate() const noexcept
    {
        return m_isViewBlocking;
    }

    /**
     * @brief 速度倍率（原版 `Block#getSpeedFactor`）
     *
     * 玩家站在这块方块上时，水平移动速度乘以此值。蜂蜜块、灵魂沙为 0.4。
     * 与摩擦系数是两套机制：摩擦决定「是否会打滑」，速度倍率决定「走得多快」。
     */
    [[nodiscard]] f32 speedFactor() const noexcept
    {
        return m_speedFactor;
    }

    /**
     * @brief 跳跃倍率（原版 `Block#getJumpFactor`）
     *
     * 玩家从这块方块起跳时的初速度乘以此值。蜂蜜块为 0.5；灵魂沙是 1.0
     * （灵魂沙只减速、不限制跳跃）。
     */
    [[nodiscard]] f32 jumpFactor() const noexcept
    {
        return m_jumpFactor;
    }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const noexcept
    {
        return m_lightLevel;
    }

    /**
     * @brief 获取方块状态的动态光照等级
     *
     * 默认返回静态光照等级，子类可重写以实现状态相关的动态光照。
     * 例如：熔炉在燃烧时发光、重生锚根据充能等级发光。
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文感知）
     * @param pos 位置（可选）
     * @return 光照等级 (0-15)
     *
     */
    [[nodiscard]] virtual u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return state.lightLevel();
    }

    /**
     * @brief 获取方块光照透明度 (0-15)
     */
    [[nodiscard]] i32 opacity() const noexcept
    {
        return m_opacity;
    }

    /**
     * @brief 获取方块的遮光亮度
     *
     * 返回方块在环境光遮蔽计算中的亮度贡献。默认实现基于碰撞形状：
     * - 碰撞形状为完整方块时返回 0.2F（产生 AO 阴影）
     * - 否则返回 1.0F（不产生额外阴影）
     *
     * 子类可重写以实现特殊的遮光行为：
     * - MudBlock/SoulSandBlock: 碰撞形状不完整但仍需阴影，返回 0.2F
     * - SnowLayerBlock: 满层(8)返回 0.2F，其他返回 1.0F
     * - BarrierBlock/StructureVoidBlock: 不可见方块，返回 1.0F
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文感知）
     * @param pos 位置（可选）
     * @return 遮光亮度 (0.0F-1.0F)
     *
     */
    [[nodiscard]] virtual f32 getShadeBrightness(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

    /**
     * @brief 获取地图颜色
     *
     * 返回此方块在地图上渲染时使用的颜色。
     * 默认使用 BlockProperties 中设置的颜色，若未设置则回退到材质颜色。
     * 子类可重写以实现状态相关或生物群系相关的颜色（如草方块）。
     *
     * @param state 方块状态
     * @param world 世界（可选，用于生物群系感知）
     * @param pos 位置（可选）
     * @return 地图颜色ID
     *
     *
     * 注册时显式设置过 `BlockProperties::mapColor()` 的方块用注册值；否则回退到
     * 原版快照表（`lookupVanillaMapColor`）。快照由差分能力 block_map_color 逐方块实测
     * 导出——地图颜色在原版是硬编码在注册调用里的，数据包里没有这项信息。
     *
     * 注意：地图颜色**不随方块状态变化**。1.21.11 的 `BlockBehaviour.getMapColor` 直接返回
     * 注册时传入的静态 `mapColor`，`RotatedPillarBlock`（按轴）、`LeavesBlock` 等都没有重写它。
     * 地图上"含水方块显示水色"来自渲染侧把状态换成流体方块的做法（见
     * `FilledMapItem::_getCorrectStateForFluidBlock`），草/树叶的生物群系染色则属于客户端
     * `BlockColors` 的职责，两者都不是 `getMapColor` 的语义。
     */
    [[nodiscard]] virtual world::map::MaterialColorId getMapColor(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        if (m_hasMapColor) {
            return m_mapColor;
        }
        return m_mapColorResolver != nullptr ? m_mapColorResolver(state) : m_mapColor;
    }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool doesPropagateSkylightDown() const noexcept
    {
        return m_propagatesSkylightDown;
    }

    /**
     * @brief 获取挖掘工具类型
     *
     * 返回采集此方块所需的工具类型。
     * 如果返回 HarvestTool::None，则不需要特定工具。
     *
     * @return 工具类型值
     */
    [[nodiscard]] u8 harvestTool() const noexcept
    {
        return m_harvestTool;
    }

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
    [[nodiscard]] i32 harvestLevel() const noexcept
    {
        return m_harvestLevel;
    }

    /**
     * @brief 是否需要正确工具才能采集
     *
     * 如果返回 true，则必须使用正确类型且等级足够的工具
     * 才能获得方块掉落物。
     *
     * @return 是否需要正确工具
     */
    [[nodiscard]] bool requiresTool() const noexcept
    {
        return m_requiresTool;
    }

    /**
     * @brief 获取方块是否可被替换（无上下文）
     *
     * 返回方块注册时设置的 replaceable 属性。
     * 对应 MC 的 BlockState.canBeReplaced() 无参版本。
     * 空气、水、岩浆、花草、火等方块返回 true；石头、泥土等实心方块返回 false。
     *
     * 用于世界生成谓词(ReplaceablePredicate)、掉落方块判断等场景。
     * 不适用于玩家放置时的替换判断（应使用 isReplaceable(state, context) 虚方法）。
     *
     * @return 是否可被替换
     */
    [[nodiscard]] bool isReplaceable() const noexcept
    {
        return m_isReplaceable;
    }

    // ========== 声音类型 ==========

    /**
     * @brief 获取方块的声音类型
     *
     * 返回方块的声音类型，包含破坏、踩踏、放置、击打、坠落声音。
     *
     * @return 声音类型的常量引用
     */
    [[nodiscard]] const BlockSoundType& getSoundType() const noexcept
    {
        return *m_soundType;
    }

    /**
     * @brief 获取该方块状态下的声音类型（虚函数入口）
     *
     * 对齐原版 `BlockBehaviour#getSoundType(BlockState)`：绝大多数方块与状态无关，直接用
     * 属性里配置的那一份；个别方块按状态切换（饰纹陶罐 `CRACKED` 时破坏声变为碎裂声）。
     *
     * 需要按状态区分声音的子类应重写本函数，而**不要**重写无参的
     * {@code getSoundType()}——无参版本拿不到状态，调用方已经全部改为传状态。
     *
     * @param state 方块状态
     * @return 声音类型的常量引用
     */
    [[nodiscard]] virtual const BlockSoundType& getSoundType(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return *m_soundType;
    }

    // ========== 掉落表 ==========

    /**
     * @brief 获取方块的掉落表ID
     *
     * 返回用于生成方块掉落的掉落表ID。
     * 默认返回空字符串，表示使用默认掉落逻辑。
     *
     * 子类可重写此方法返回自定义掉落表ID。
     * 例如：
     * - 石头: "minecraft:blocks/stone"
     * - 钻石矿石: "minecraft:blocks/diamond_ore"
     *
     * @return 掉落表ID，如 "minecraft:blocks/stone"，空字符串表示无掉落表
     */
    [[nodiscard]] virtual std::string getLootTableId() const noexcept
    {
        return m_lootTableId;
    }

    /**
     * @brief 获取方块的掉落表
     *
     * 从掉落表管理器获取此方块的掉落表。
     *
     * @param manager 掉落表管理器
     * @return 掉落表指针，无掉落表或找不到时返回nullptr
     */
    [[nodiscard]] const loot::LootTable* getLootTable(const loot::LootTableManager& manager) const;

    /**
     * @brief 设置掉落表ID
     *
     * @param id 掉落表ID
     */
    void setLootTableId(const std::string& id) noexcept
    {
        m_lootTableId = id;
    }

    /**
     * @brief 获取中键选取方块时获得的物品堆
     *
     * 当玩家中键点击方块时，返回应给予玩家的物品堆。
     * 默认实现返回空（由外部系统通过 BlockItemRegistry 查找对应方块物品）。
     * 某些方块（如洞穴藤蔓）的中键选取返回的物品与方块物品不同，
     * 需要覆写此方法返回正确的物品。
     *
     *
     * @param state 方块状态
     * @param world 世界指针（可能为空）
     * @param pos 方块位置指针（可能为空）
     * @return 物品堆，空表示使用默认方块物品
     */
    [[nodiscard]] virtual ItemStack getCloneItemStack(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

    // ========================================================================
    // 虚方法
    // ========================================================================

    /**
     * @brief 获取渲染形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getShape(const BlockState& state) const;

    /**
     * @brief 获取碰撞形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getCollisionShape(const BlockState& state) const;

    /**
     * @brief 获取带实体上下文的碰撞形状
     *
     * 对齐 vanilla BlockBehaviour#getCollisionShape(BlockState, BlockGetter, BlockPos,
     * CollisionContext)。当碰撞形状需按实体区分时（如细雪 PowderSnowBlock：可行走实体
     * 得完整碰撞箱、下落实体得半穿透形状、其余得空形状），方块重写此方法依据 ctx 与方块
     * 世界坐标判定。
     *
     * 默认实现委托 getCollisionShape(state)，即不区分实体——绝大多数方块行为不变。
     * 仅物理引擎在持有实体上下文时调用此方法；无实体上下文（如方块放置预检、AI 寻路、
     * 渲染等）的调用点继续调用 getCollisionShape。
     *
     * @param state 方块状态
     * @param ctx 实体碰撞上下文（entity 可能为 nullptr）
     * @param blockY 方块世界 Y 坐标（用于 isAbove 几何判定实体是否在方块顶面）
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getCollisionShapeForEntity(
        const BlockState& state, const EntityCollisionContext& ctx, i32 blockY) const;

    /**
     * @brief 该位置是否允许生成指定类型的生物
     *
     * 对应 vanilla `BlockBehaviour.BlockStateBase#isValidSpawn(BlockGetter, BlockPos, EntityType)`，
     * 其默认谓词为「该方块的顶面是完整支撑面，且自身发光等级 < 14」。传送门刷僵尸猪灵、
     * 结构/刷怪蛋等「在此方块上方生成实体」的场景都用它做最后一道位置校验。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param entityType 待生成的实体类型（默认谓词不使用，供覆写区分生物）
     * @return 允许生成返回 true
     */
    [[nodiscard]] virtual bool isValidSpawn(
        IWorld& world, const BlockPos& pos, const BlockState& state, const entity::EntityType& entityType) const;

    /**
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getOcclusionShape(const BlockState& state) const;

    /**
     * @brief 获取方块支撑形状
     *
     * 支撑形状用于判断方块面是否足够坚固以支撑其他方块放置。
     * 默认返回碰撞形状。某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，
     * 但支撑形状是完整方块，因此需要在子类中重写此方法。
     *
     *
     * @param state 方块状态
     * @return 支撑形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getBlockSupportShape(const BlockState& state) const;

    /**
     * @brief 获取指定面的遮挡形状
     *
     * 用于光照遮挡检测。返回方块在指定方向上的投影形状。
     * 默认实现返回完整遮挡形状的切片。
     *
     *
     * @param state 方块状态
     * @param direction 方向
     * @return 面遮挡形状
     */
    [[nodiscard]] virtual CollisionShape getFaceOcclusionShape(const BlockState& state, Direction direction) const;

    /**
     * @brief 获取实体内部碰撞形状
     *
     * 返回用于检测实体是否在方块内部的碰撞形状。大多数方块返回完整方块形状
     * （默认行为），这样只要实体 AABB 与方块网格重叠就会触发 onEntityCollision。
     * 某些方块（如炼药锅、细雪）重写此方法返回更小的形状，使得只有当实体实际
     * 进入方块的内容区域（如炼药锅内的岩浆/水）时才触发 onEntityCollision。
     *
     *
     * @param state 方块状态
     * @return 实体内部碰撞形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getEntityInsideCollisionShape(const BlockState& state) const;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 如果返回 true，光照引擎会使用 getOcclusionShape() 和
     * getFaceOcclusionShape() 进行精确的光照传播计算。
     * 如果返回 false，光照引擎会使用简单的透明度值。
     *
     * 用于台阶、楼梯、栅栏等非完整方块。
     *
     *
     * @param state 方块状态
     * @return 是否使用形状进行光照遮挡
     */
    [[nodiscard]] virtual bool useShapeForLightOcclusion(const BlockState& state) const;

    /**
     * @brief 判断当前方块与邻居方块之间的面是否应该跳过渲染
     *
     * 当两个相邻方块之间的面不需要渲染时返回 true（面被剔除），
     * 返回 false 时该面需要正常渲染。
     *
     * 默认实现返回 false（始终渲染面），子类可以重写此方法实现
     * 特殊的渲染剔除逻辑（如铁栏杆、铜栏杆之间的连接面剔除）。
     *
     * @param selfState 当前方块的方块状态
     * @param neighborState 邻居方块的方块状态
     * @param direction 从当前方块指向邻居方块的方向
     * @return 如果应该跳过渲染返回 true，否则返回 false
     */
    [[nodiscard]] virtual bool skipRendering(
        const BlockState& selfState, const BlockState& neighborState, Direction direction) const;

    /**
     * @brief 是否为空气
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isAir(const BlockState& state) const;

    /**
     * @brief 是否为固体
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isSolid(const BlockState& state) const;

    /**
     * @brief 该状态是否阻挡实体运动（vanilla `BlockState#blocksMotion`）
     *
     * 判据是 `isSolid()`，**只排除蜘蛛网与竹笋**两个方块。它们看着有碰撞，原版却
     * 允许实体穿行；高度图的 MOTION_BLOCKING 系列与窒息/视线默认谓词都以它为准。
     *
     * 注意与 `isSolid()` 区分：树叶、玻璃的 `isSolid()` 为 true（能挡住脚步），
     * 但它们不窒息、不挡视线；漏斗 `isSolid()` 也是 true，但碰撞形状不满格。
     */
    [[nodiscard]] virtual bool blocksMotion(const BlockState& state) const;

    /**
     * @brief 该状态的碰撞形状是否覆盖整个方块（vanilla `BlockState#isCollisionShapeFullBlock`）
     *
     * 判定对象是**碰撞形状**而非外形：耕地、泥土路径的碰撞比外形矮一格像素，两者都判 false；
     * 告示牌、按钮没有碰撞，同样判 false。带位置查询（叠加 `offsetType` 偏移）。
     */
    [[nodiscard]] virtual bool isCollisionShapeFullBlock(const BlockState& state, const BlockPos& pos) const;

    /**
     * @brief 该状态是否让位于其中的实体窒息（vanilla `BlockState#isSuffocating`）
     *
     * 注册时可用 `BlockProperties::suffocating()` 覆写；未覆写时默认判据为
     * `blocksMotion() && isCollisionShapeFullBlock()`。
     * 窒息判定在实体每 tick 的 `isInWall` 链路上调用，原版语义是「这一格能不能把人闷死」，
     * 与是否透光、是否透明**无关**：泥土、灵魂沙、冰都是有碰撞满格的窒息方块，玻璃则不是。
     */
    [[nodiscard]] virtual bool isSuffocatingAt(const BlockState& state, const BlockPos& pos) const
    {
        const StatePredicate* predicate = suffocatingPredicate();
        if (predicate != nullptr) {
            return predicate->test(state, pos);
        }
        return blocksMotion(state) && isCollisionShapeFullBlock(state, pos);
    }

    /**
     * @brief 该状态是否挡住视线（vanilla `BlockState#isViewBlocking`）
     *
     * 未覆写视线谓词时直接转调 `isSuffocatingAt`（原版默认就是同一个谓词实例），
     * 因此只覆写了窒息判定的方块（甘蔗丛、红树根）会连带影响视线判定。
     */
    [[nodiscard]] virtual bool isViewBlockingAt(const BlockState& state, const BlockPos& pos) const
    {
        const StatePredicate* predicate = viewBlockingPredicate();
        if (predicate != nullptr) {
            return predicate->test(state, pos);
        }
        return isSuffocatingAt(state, pos);
    }

    /**
     * @brief 是否不透明
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isOpaque(const BlockState& state) const;

    /**
     * @brief 该状态的方块能否被指定计算类型穿过
     *
     * 对齐原版 `Block#isPathfindable(BlockState, PathComputationType)`：寻路判定问的是
     * 「这一格能不能让身体过去」，它与「有没有碰撞」并不等价——雪层碰撞形状非空，但
     * 只有层数 >= 5 才不可穿过；铁轨没有碰撞，却也不是可替换方块。默认实现对应
     * `PathComputationType.LAND` 下「无碰撞即可穿过」。
     *
     * @param state 方块状态
     * @param type 计算类型
     */
    [[nodiscard]] virtual bool isPathfindable(const BlockState& state, PathComputationType type) const;

    /**
     * @brief 该方块是否参与遮挡计算（vanilla `BlockState#canOcclude`）
     *
     * 由注册属性 `BlockProperties::noOcclusion()` 决定，与材质无关。canOcclude=false 的方块
     * 不产生任何遮挡：无遮挡形状、非实体渲染（isSolidRender=false）、无 AO 阴影。
     *
     * 该开关与材质无关：铁栏杆（金属材质）、潜影盒（木质材质）等 vanilla noOcclusion 方块
     * 不再被当作完全遮光（见 `Block::getOpacity`）。
     *
     */
    [[nodiscard]] bool canOcclude() const noexcept
    {
        return m_canOcclude;
    }

    /**
     * @brief 刷新全部状态的光照缓存（opacity / propagatesSkylightDown）
     *
     * 状态在 StateContainer 构造期缓存光照属性，早于子类构造完成与 setDefaultState()；
     * 方块注册流程在全部方块构造完毕后调用（幂等，可重复调用）。
     */
    void refreshLightCache();

    /**
     * @brief 获取光照透明度 (0-15)
     *
     * 返回方块阻挡光线的程度：
     * - 0: 完全透明，光线无衰减通过
     * - 1-14: 部分透明，光线衰减指定等级
     * - 15: 完全不透明，阻挡所有光线
     *
     * 对于透明方块（如玻璃），返回0但仍然阻挡天空光传播。
     * 对于树叶、冰、水等，返回1-2使光线衰减。
     *
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文相关透明度）
     * @param pos 位置（可选）
     * @return 光照透明度 (0-15)
     */
    [[nodiscard]] virtual i32 getOpacity(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

    /**
     * @brief 检查是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 如果天空光可以传播返回true
     */
    [[nodiscard]] virtual bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

    /**
     * @brief 检查指定面是否为实体面
     *
     * 用于流体流动判断、渲染面剔除等。
     * 默认实现基于材质和碰撞形状。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 要检查的面
     * @return 如果该面是实体面返回true
     */
    [[nodiscard]] virtual bool isSolidSide(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const;

    /**
     * @brief 获取流体状态
     *
     * 默认返回空流体。液体方块（LiquidBlock）会重写此方法返回对应的流体。
     *
     * @param state 方块状态
     * @return 流体状态指针
     */
    [[nodiscard]] virtual const fluid::FluidState* getFluidState(const BlockState& state) const;

    // ========================================================================
    // Tick方法
    // ========================================================================

    /**
     * @brief 执行方块计划刻
     *
     * 当方块的计划刻到期时调用。默认实现为空。
     * 需要tick行为的方块（如活塞、红石元件、农作物等）应重写此方法。
     *
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    virtual void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random);

    /**
     * @brief 执行随机刻
     *
     * 在随机刻中被调用。默认实现为空。
     * 需要随机tick行为的方块（如农作物生长、铜氧化等）应重写此方法。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    virtual void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random);

    /**
     * @brief 客户端方块动画 tick
     *
     * 仅在客户端每 tick 调用，用于生成视觉效果粒子、播放环境音效等。
     * 默认实现为空操作。需要客户端动画效果的方块应重写此方法。
     *
     * 由 ClientWorld::animateTick 在每 tick 对玩家周围随机采样位置调用，
     * 采样范围约 32 格，每 tick 采样约 1334 个位置。
     *
     * 注意：此方法仅在客户端执行，不会在服务端调用。
     *
     * @param context 动画上下文（提供粒子和音效接口）
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    virtual void animateTick(
        IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const;

    /**
     * @brief 邻居方块更新
     *
     * 当相邻方块改变时调用。默认实现为空。
     * 需要响应邻居变化的方块（如红石、流体等）应重写此方法。
     *
     * @param world 世界引用
     * @param pos 当前方块位置
     * @param neighborBlock 邻居方块
     * @param neighborPos 邻居位置
     * @param isMoving 是否正在移动（活塞等）
     */
    virtual void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving);

    /**
     * @brief 间接邻居形状更新（对应 vanilla `BlockBehaviour#updateIndirectNeighbourShapes`）
     *
     * 当本方块在某位置被写入/移除时，原版除了对 6 个**直接**邻居做形状更新
     * （`updateNeighbourShapes`）之外，还会调用本方法让方块自己处理"隔一格的邻居"。
     * 目前只有红石线需要：红石线连到一格高的方块上时（`RedstoneSide::Up`），真正的
     * 连接对象在斜上方，直接邻居更新覆盖不到它。
     *
     * 默认实现为空（与 vanilla 基类一致）。**这里只能发起形状更新**（经由
     * `IWorld::notifyShapeChanged`），不要触发邻居通知或方块 tick。
     *
     * @param state 本方块（调用方所在位置）的状态
     * @param world 世界
     * @param pos 调用方所在位置
     * @param flags 形状更新标志（原版传入的 flags 已剥掉形状递归自身）
     * @param recursionLeft 剩余的形状更新递归预算
     * @param movedByPiston 是否由活塞推动引发（对应 vanilla 单独注入 `updateIndirectNeighborShape` 的场景）
     */
    virtual void updateIndirectNeighbourShapes(
        const BlockState& state, IWorld& world, const BlockPos& pos, i32 flags, i32 recursionLeft, bool movedByPiston);

    /**
     * @brief 对 6 个邻居发起形状更新（对应 vanilla `BlockState#updateNeighbourShapes`）
     *
     * 按 `UPDATE_SHAPE_ORDER` 依次对每个邻居调用 {@link IWorld::notifyShapeChanged}。
     *
     * @param state 本次写入后的方块状态
     * @param world 世界
     * @param pos 本次写入的位置
     * @param flags 形状更新标志
     * @param recursionLeft 剩余的形状更新递归预算
     * @param movedByPiston 是否由活塞推动引发
     */
    static void updateNeighbourShapes(
        const BlockState& state, IWorld& world, const BlockPos& pos, i32 flags, i32 recursionLeft, bool movedByPiston);

    /**
     * @brief 对单个邻居发起形状更新（对应 vanilla `NeighborUpdater#executeShapeUpdate`）
     *
     * 读取 `neighborPos` 的当前状态，用来源方向/来源状态跑一次 `updatePostPlacement`，
     * 结果不同则经 `Block::updateOrDestroy` 写回。对应原版 `NeighborUpdater.executeShapeUpdate`：
     * `flags` 含 UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE 且目标为红石线时直接跳过。
     *
     * @param direction 从邻居指向本方块的方向（即邻居的"更新方向"）
     * @param sourceState 本方块（更新来源）的状态
     * @param neighborPos 邻居位置（被更新者）
     * @param sourcePos 本方块位置（更新来源）
     * @param flags 形状更新标志
     * @param recursionLeft 剩余的形状更新递归预算
     * @param movedByPiston 是否由活塞推动引发
     */
    static void executeShapeUpdate(Direction direction,
        const BlockState& sourceState,
        IWorld& world,
        const BlockPos& neighborPos,
        const BlockPos& sourcePos,
        i32 flags,
        i32 recursionLeft,
        bool movedByPiston);

    /**
     * @brief 方块被放置时的处理
     *
     * 当方块被放置到世界中时调用。默认实现为空。
     * 需要特殊初始化的方块应重写此方法。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston);

    /**
     * @brief 方块被移除时的处理
     *
     * 当方块从世界中移除时调用。默认实现为空。
     * 需要特殊清理的方块应重写此方法。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston);

    /**
     * @brief 状态变更时是否保留方块实体
     *
     * 当方块因氧化/涂蜡/除蜡/刮削等原因导致方块 ID 变化但语义上"仍是同一个方块"时，
     * 应返回 true 以保留旧方块实体（物品内容物、自定义名称等不丢失）。
     *
     * 默认实现返回 false：方块 ID 变化时旧方块实体会被销毁，新方块实体会被创建。
     * 铜箱子等容器类铜方块应重写此方法返回 true，避免物品在氧化/涂蜡/除蜡/刮削时丢失。
     *
     * (MC 1.21.11)
     *
     * @param state 当前方块状态
     * @return 如果状态变更时应保留方块实体返回 true
     */
    [[nodiscard]] virtual bool shouldChangedStateKeepBlockEntity(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 玩家即将破坏方块时调用
     *
     * 在方块被破坏之前调用，允许方块在破坏前执行逻辑（如级联销毁关联方块）。
     * 与 onBlockRemoved 的区别：此方法接收玩家信息，可区分创造/生存模式。
     * onBlockRemoved 在方块状态变更后调用，不包含玩家上下文。
     *
     * 默认实现检查 GUARDED_BY_PIGLINS 标签，如果方块属于此标签则激怒附近猪灵。
     * 需要额外行为的方块应重写此方法（记得调用基类实现以保留猪灵愤怒行为）。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态（破坏前的状态）
     * @param player 破坏方块的玩家
     *
     */
    virtual void playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player);

    /**
     * @brief 方块被破坏后的额外生成处理
     *
     * 当方块被玩家或爆炸破坏后调用，用于执行额外的生成逻辑（如蠹虫生成）。
     * 默认实现为空。此方法在掉落物生成之后、方块实际移除之前调用。
     *
     * 与 onBlockRemoved 的区别：此方法接收破坏工具信息，可用于判断附魔效果。
     * onBlockRemoved 在方块状态变更后调用，不包含工具上下文。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态（破坏前的状态）
     * @param tool 破坏工具（可能为空，如爆炸破坏）
     * @param dropExp 是否掉落经验（爆炸时可能为 false）
     *
     */
    virtual void spawnAfterBreak(
        IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, bool dropExp) const;

    /**
     * @brief 破坏方块应掉落的经验值（vanilla `DropExperienceBlock.tryDropExperience` 的采样部分）
     *
     * 原版把经验区间放在方块实例上（`DropExperienceBlock.xpRange`），挖掉时按区间采样。
     * Cubium 的方块状态参数来自数据包，经验区间在注册期手工登记，因此这里只暴露
     * 「按区间采样一次」的接口，由各矿物/幽匿方块覆写。
     *
     * 采样会消耗一次随机数（固定值区间不消耗），因此「有没有经验」会影响同一 tick
     * 内后续随机数的走向，调用方必须按原版时机调用。
     *
     * @param random 采样用的随机源
     * @return 应掉落的经验值；0 表示不掉落
     */
    [[nodiscard]] virtual i32 dropExperience(math::IRandom& random) const
    {
        (void)random;
        return 0;
    }

    // ========================================================================
    // 爆炸相关
    // ========================================================================

    /**
     * @brief 获取爆炸抗性
     *
     * 返回方块抵抗爆炸的能力。值越大越难被破坏。
     * 默认实现返回方块的 resistance 值。
     *
     * @param state 方块状态
     * @return 爆炸抗性值
     *
     */
    [[nodiscard]] virtual f32 getExplosionResistance(const BlockState& state) const noexcept
    {
        return state.resistance();
    }

    /**
     * @brief 判断方块是否可以在爆炸中掉落物品
     *
     * 返回 true 表示爆炸破坏时可以掉落物品。
     * 某些方块（如叶子、玻璃）在爆炸时不会掉落物品。
     *
     * @param state 方块状态
     * @return 是否可以掉落物品
     *
     */
    [[nodiscard]] virtual bool canDropFromExplosion(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 在世界中生成方块的掉落物品
     *
     * 当方块被非玩家方式破坏（如海绵吸水、爆炸等）时，调用此方法生成掉落物品。
     * 使用方块的掉落表生成物品，不携带工具和玩家上下文（因此无时运/精准采集加成）。
     * 仅在服务端（lootTableManager 非空时）生效。
     *
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 被破坏的方块状态
     */
    static void dropResources(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 在世界中生成方块的掉落物品（携带方块实体上下文）
     *
     * 与三参版本相同，但把方块实体作为掉落参数 `BLOCK_ENTITY` 传入，供
     * `copy_components` / `copy_custom_data` 等掉落函数复制容器名称与组件。
     * 必须在方块实体被移除之前调用。
     *
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 被破坏的方块状态
     * @param blockEntity 该位置的方块实体（可为 nullptr）
     */
    static void dropResources(IWorld& world, const BlockPos& pos, const BlockState& state, BlockEntity* blockEntity);

    /**
     * @brief 方块被爆炸破坏时的处理
     *
     * 当方块被爆炸破坏时调用。默认实现为空。
     * 特殊方块（如 TNT）可以重写此方法实现特殊行为。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @param explosion 引发此方块破坏的爆炸，可能为 nullptr
     *
     */
    virtual void onBlockExploded(
        IWorld& world, const BlockPos& pos, const BlockState& state, const world::explosion::Explosion* explosion) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        MC_UNUSED(explosion);
    }

    /**
     * @brief 实体与方块碰撞时调用
     *
     * 当实体进入方块的碰撞区域时调用。用于特殊方块行为，如漏斗收集物品、
     * 仙人掌造成伤害、甜浆果丛减速等。
     * 默认实现为空。
     *
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    virtual void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
    }

    /**
     * @brief 实体着地时调用
     *
     * 当实体垂直移动后落到此方块上时调用。
     * 此方法必须更新实体的Y轴速度，因为实体不会自动执行此操作。
     * 默认实现将实体的Y速度归零。
     *
     *
     * @param state 方块状态
     * @param world 世界引用（可为IBlockReader）
     * @param pos 方块位置
     * @param entity 着地的实体
     */
    virtual void onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const;

    /**
     * @brief 实体在方块上行走时调用
     *
     * 当实体在地面行走时每帧调用。用于特殊方块行为，如岩浆块造成伤害、
     * 岩浆方块产生气泡、脚手架攀爬等。
     * 默认实现为空。
     *
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 行走的实体
     */
    virtual void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const;

    /**
     * @brief 获取方块滑度
     *
     * 返回方块的滑动系数，影响实体在方块上的移动阻力。
     * - 0.6f: 默认值（普通方块如石头、泥土、蜂蜜块）
     * - 0.8f: 史莱姆块
     * - 0.98f: 冰、浮冰
     * - 0.989f: 蓝冰
     *
     * 注意：蜂蜜块的减速效果通过 speedFactor 和 jumpFactor 实现，而非修改滑度。
     *
     *
     * @param state 方块状态
     * @param world 世界引用（可选）
     * @param pos 方块位置（可选）
     * @param entity 实体（可选，用于上下文相关滑度）
     * @return 滑度值 (0.0-1.0)
     */
    [[nodiscard]] virtual f32 getSlipperiness(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
        MC_UNUSED(state);
        return m_slipperiness;
    }

    /**
     * @brief 获取方块速度因子
     *
     * 返回方块的速度影响系数，影响实体在方块上的移动速度。
     * - 1.0f: 默认值（正常速度）
     * - 0.5f: 蜂蜜块（减速）
     * - 1.5f: 灵魂沙（减速，特殊处理）
     *
     *
     * @param state 方块状态
     * @param world 世界引用（可选）
     * @param pos 方块位置（可选）
     * @return 速度因子值
     */
    [[nodiscard]] virtual f32 getSpeedFactor(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return m_speedFactor;
    }

    /**
     * @brief 获取方块跳跃因子
     *
     * 返回方块的跳跃影响系数。
     * - 1.0f: 默认值（正常跳跃）
     * - 蜂蜜块会减少跳跃高度
     *
     *
     * @param state 方块状态
     * @param world 世界引用（可选）
     * @param pos 方块位置（可选）
     * @return 跳跃因子值
     */
    [[nodiscard]] virtual f32 getJumpFactor(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return m_jumpFactor;
    }

    /**
     * @brief 检查方块是否可攀爬
     *
     * 返回此方块是否可以作为梯子供实体攀爬。
     * 梯子、藤蔓、脚手架等方块应返回 true。
     * 实体在攀爬方块上时：
     * - 可以向上/向下移动
     * - 最大水平速度为 0.15
     * - 重力被抵消
     *
     *
     * @param state 方块状态
     * @param world 世界引用（可选）
     * @param pos 方块位置（可选）
     * @param entity 实体（可选，用于上下文相关判断）
     * @return 如果实体可以攀爬此方块返回 true
     */
    [[nodiscard]] virtual bool isLadder(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 是否响应随机刻
     *
     * 返回true时，该方块会被随机刻系统选中执行randomTick。
     * 默认返回BlockProperties中设置的值。
     *
     *
     * @return 是否响应随机刻
     */
    [[nodiscard]] virtual bool ticksRandomly() const noexcept
    {
        return m_ticksRandomly;
    }

    /**
     * @brief 指定状态是否响应随机刻（对齐 vanilla `BlockBehaviour#isRandomlyTicking(BlockState)`）
     *
     * 随机刻调度的正确入口。默认沿用方块级的 `ticksRandomly()`，需要按状态决定的方块
     * （红石矿只在 LIT 时随机刻）重写本方法。
     */
    [[nodiscard]] virtual bool isRandomlyTicking(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return ticksRandomly();
    }

    /**
     * @brief 是否可被岩浆点燃
     *
     * 当返回 true 时，此方块接触岩浆会被点燃。
     * 木头类、TNT等可燃方块应返回 true。
     *
     */
    [[nodiscard]] bool isIgnitedByLava() const noexcept
    {
        return m_ignitedByLava;
    }

    /**
     * @brief 获取模型偏移类型
     *
     * 控制方块渲染时的随机位置偏移。
     * 植物、花等小方块使用XZ偏移来避免视觉重复。
     *
     */
    [[nodiscard]] BlockOffsetType getOffsetType() const noexcept
    {
        return m_offsetType;
    }

    /**
     * @brief 方块形状在给定位置的随机偏移。
     *
     * 花、作物、树苗等方块带 `offsetType`，同一方块在不同坐标处会轻微移动位置，
     * 避免整片同种方块看起来完全一致。偏移量由**坐标哈希**决定，必须逐坐标计算，
     * 不能当作常量。
     *
     * 取值范围：水平 `±getMaxHorizontalOffset()`（默认 0.25），
     * 竖直 `-getMaxVerticalOffset()..0`（默认 0.2）。
     *
     * @param pos 方块位置
     * @return 偏移向量，`OffsetType::None` 时为零向量
     */
    [[nodiscard]] Vector3 getOffset(const BlockPos& pos) const noexcept;

    /**
     * @brief 获取指定位置处的形状（世界坐标，可能含 `offsetType` 偏移）。
     *
     * **默认实现不应用偏移**：原版只有少数方块（`FlowerBlock`、`BambooStalkBlock`、
     * `PointedDripstoneBlock`）在 `getShape` 里调用了 `move(getOffset(pos))`，
     * 草、树苗、蘑菇等虽有 `offsetType` 但 `getShape` 仍返回固定形状。
     * 需要偏移的方块覆写本方法。
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @return 形状（按值返回，因为每个位置的偏移不同）
     */
    [[nodiscard]] virtual CollisionShape getShapeAt(const BlockState& state, const BlockPos& pos) const
    {
        MC_UNUSED(pos);
        return getShape(state);
    }

    /**
     * @brief 获取指定位置处的**碰撞**形状（世界坐标）。
     *
     * 原版 `getCollisionShape` 是 `hasCollision ? getShape(...) : empty()`：无碰撞的方块
     * （按钮、火把、花草）即使外形非空，碰撞形状也为空。有碰撞时还要按方块自身的覆写
     * 决定——悬挂告示牌、南瓜头、砂轮等 `getCollisionShape` 覆写返回**固定常量**，
     * 与外形无关。因此这里必须以 `getCollisionShape` 为基准，不能退回 `getShapeAt`。
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @return 碰撞形状
     */
    [[nodiscard]] virtual CollisionShape getCollisionShapeAt(const BlockState& state, const BlockPos& pos) const
    {
        MC_UNUSED(pos);
        return getCollisionShape(state);
    }

    /**
     * @brief 获取指定位置处的支撑形状（世界坐标）。
     *
     * 默认转调**无位置的** `getBlockSupportShape` 再加偏移，而不是转调 `getCollisionShapeAt`：
     * 支撑形状是独立概念，树叶的碰撞形状是完整方块但支撑形状恒空，卡片/梯子等也各自
     * 覆写了空支撑形状。转调碰撞形状会把那些覆写全部绕过。
     *
     * 偏移取自 `getOffsetAt`（默认等于无位置 `getOffset`）。原版 `getBlockSupportShape`
     * 继承自 `getCollisionShape`，本身不含偏移，偏移只可能来自这两个覆写层。
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @return 支撑形状
     */
    [[nodiscard]] virtual CollisionShape getBlockSupportShapeAt(const BlockState& state, const BlockPos& pos) const
    {
        const CollisionShape base = getBlockSupportShape(state);
        const Vector3 offset = getOffsetAt(state, pos);
        if (offset.x == 0.0f && offset.y == 0.0f && offset.z == 0.0f) {
            return base;
        }
        return base.offset(offset.x, offset.y, offset.z);
    }

    /**
     * @brief 取指定状态在指定位置的 `offsetType` 偏移。
     *
     * 默认转调 `getOffset(pos)`（忽略状态）。需要「偏移随状态变化」的方块在这里覆写；
     * 目前所有 `offsetType` 方块的偏移都与状态无关。
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @return 偏移量
     */
    [[nodiscard]] virtual Vector3 getOffsetAt(const BlockState& state, const BlockPos& pos) const noexcept
    {
        MC_UNUSED(state);
        return getOffset(pos);
    }

    /**
     * @brief 水平偏移量上限。
     *
     * 偏移公式把它作为 clamp 的边界；个别方块会覆写（例如更高的作物）。
     */
    [[nodiscard]] virtual f32 getMaxHorizontalOffset() const noexcept
    {
        return 0.25f;
    }

    /**
     * @brief 竖直偏移量上限（只向下偏移，故实际范围是 `-该值..0`）。
     */
    [[nodiscard]] virtual f32 getMaxVerticalOffset() const noexcept
    {
        return 0.2f;
    }

    /**
     * @brief 获取音符盒乐器
     *
     * 当音符盒放置在此方块上方时演奏的乐器。显式设置的属性优先（如头颅、熔炉）；
     * 未显式设置时由音符盒按方块类型默认规则推导（NoteBlock::blockInstrumentByType）。
     *
     */
    [[nodiscard]] NoteBlockInstrument getInstrument() const noexcept
    {
        return m_instrument.value_or(NoteBlockInstrument::Harp);
    }

    /**
     * @brief 在方块构造期（非虚调用）设置音符盒乐器
     *
     * 用于子类构造函数中覆盖音符盒乐器（如头颅按种类设定），不依赖虚派发。
     */
    void setNonVirtualInstrument(NoteBlockInstrument instrument) noexcept
    {
        m_instrument = instrument;
    }

    /// 是否显式设置了音符盒乐器（否则由音符盒按方块类型默认规则推导）
    [[nodiscard]] bool hasExplicitInstrument() const noexcept
    {
        return m_instrument.has_value();
    }

    // ========================================================================
    // 方块状态
    // ========================================================================

    /**
     * @brief 填充方块状态容器
     *
     * 子类重写此方法添加自定义状态属性。
     * 默认实现为空（无属性）。
     *
     * @param container 状态容器构建器
     */
    virtual void fillStateContainer(StateContainer<Block, BlockState>& container);

    /**
     * @brief 获取默认方块状态
     *
     * 返回方块的默认状态。
     * 子类应重写此方法返回带默认属性值的状态。
     *
     * @return 默认方块状态
     */
    [[nodiscard]] virtual const BlockState& getDefaultState() const;

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据放置上下文确定方块的状态。
     * 默认实现返回默认状态。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] virtual BlockState getStateForPlacement(BlockItemUseContext& context);

    /**
     * @brief 方块放置后的处理
     *
     * 在方块被玩家放置后调用，携带放置用的物品堆，便于方块从物品读取数据
     * （例如铁砧重命名后的自定义名称、物品 BlockEntityTag 等）。
     *
     * 默认实现为空。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param placer 放置者（玩家、生物或发射器等放置方块的实体；非实体来源放置时为 nullptr）
     * @param stack 放置该方块所用的物品堆（始终非空，数量尚未扣除）
     */
    virtual void onBlockPlacedBy(
        IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack);

    /**
     * @brief 方块更新后处理
     *
     * 当邻居方块更新时调用，返回更新后的状态。
     * 默认实现返回原状态。
     *
     * @param state 当前方块状态
     * @param facing 更新的方向
     * @param facingState 邻居状态
     * @param world 世界
     * @param currentPos 当前方块位置
     * @param facingPos 邻居位置
     * @return 更新后的状态
     */
    [[nodiscard]] virtual BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos);

    /**
     * @brief 检查是否可以放置
     *
     * @param state 方块状态
     * @param world 世界读取器
     * @param pos 方块位置
     * @return 如果可以放置返回true
     */
    [[nodiscard]] virtual bool isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 放置前的严格判定（对齐 vanilla Block#getStateForPlacement 返回 null 的放弃语义）
     *
     * 默认实现返回 true：绝大多数方块的放置可行性由 isValidPosition（canSurvive）完整表达。
     * 少数方块（如梯子）的放置依赖上下文（点击面/相邻方块）且无法用单个状态 + canSurvive 表达，
     * 可重写此方法，在 BlockItem 计算放置状态之前给出精确判定。
     *
     * @param context 放置上下文
     * @return 可以放置返回 true
     */
    [[nodiscard]] virtual bool canPlaceAt(const BlockItemUseContext& context) const;

    /**
     * @brief 检查方块是否可被替换
     *
     * 当玩家使用物品点击方块时调用，判断是否可以替换该方块。
     * 默认实现返回 BlockProperties::isReplaceable() 的值。
     *
     * 子类可重写此方法实现特殊替换逻辑，如台阶可被同类型台阶替换形成双层台阶，
     * 花瓣床可被同类型花瓣床堆叠（AMOUNT+1）。
     *
     * @param state 当前方块状态
     * @param context 物品使用上下文（只读）
     * @return 如果方块可被替换返回true
     */
    [[nodiscard]] virtual bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const;

    /**
     * @brief 检查方块是否可以支撑植物
     *
     * 用于检查土壤方块是否支持种植特定类型的植物。
     * 默认实现返回 false（不支持任何植物）。
     * 子类（如草方块、泥土、耕地等）应重写此方法。
     *
     *
     * @param state 当前方块状态
     * @param world 世界读取器
     * @param pos 方块位置
     * @param facing 植物朝向（通常是 Direction::UP）
     * @param plant 植物接口（可获取植物类型和状态）
     * @return 如果可以支撑植物返回true
     */
    [[nodiscard]] virtual bool canSustainPlant(const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos,
        Direction facing,
        const IPlantable& plant) const;

    // ========================================================================
    // 交互
    // ========================================================================

    /**
     * @brief 玩家手持物品右键点击方块（对应 vanilla `BlockBehaviour.useItemOn`）
     *
     * 仅当玩家手持物品参与交互时分派；由服务端 `useItemOn` 派发链在主手交互且未潜行时调用。
     * 默认实现返回 `TryEmptyHandInteraction`，即"本方块不关心手持物品，请以空手语义重试"，
     * 这会驱动服务端转调 `useWithoutItem`（对齐 vanilla `BlockBehaviour.useItemOn` 默认返回
     * `InteractionResult.TRY_WITH_EMPTY_HAND`）。
     *
     * 返回值若 `consumesAction()` 为真，则服务端短路后续的 `useWithoutItem` 与 `Item::useOn`。
     * 需要传递物品转换信息的方块应使用 `BlockActionResult::success(stack)` 系列工厂方法。
     *
     * 消耗纪律：本方法对 `stack` 的修改直接作用于玩家权威手持槽（服务端传入的即权威引用），
     * 服务端在返回后统一同步物品栏。创造模式下的"无消耗"须自行用
     * `if (!player.hasInfiniteMaterials()) stack.shrink(1);` 表达。
     *
     * @param stack 玩家手中该手的物品堆（引用语义：可直接 shrink/替换以消耗或转换物品）
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果（可能携带转换后的手持物品）
     */
    [[nodiscard]] virtual BlockActionResult useItemOn(ItemStack& stack,
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit);

    /**
     * @brief 玩家空手右键点击方块（对应 vanilla `BlockBehaviour.useWithoutItem`）
     *
     * 在"手持物品时 `useItemOn` 返回 `TryEmptyHandInteraction`"或"主手为空/正在潜行"时调用。
     * 默认实现返回 `Pass`，对齐 vanilla `BlockBehaviour.useWithoutItem` 默认返回
     * `InteractionResult.PASS`。
     *
     * 返回值语义与 `useItemOn` 一致（`BlockActionResult`，携带可选的 `heldItemTransformedTo`，
     * 供返回值替换空手槽位的场景使用，如花盆取物）。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hit 射线检测结果
     * @return 交互结果（可能携带转换后的手持物品）
     */
    [[nodiscard]] virtual BlockActionResult useWithoutItem(
        const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit);

    /**
     * @brief 获取方块提供的菜单类型（对应 vanilla `BlockState.getMenuProvider` 的返回值）
     *
     * 有界面的方块（箱子、熔炉、铁砧、附魔台……）重写此方法返回其 `ContainerType`；
     * 无界面的方块返回 `std::nullopt`。服务端在"旁观者右键"与"空手右键"两条路径上用它
     * 统一打开界面，使容器类型只由方块自身声明一处，避免每个方块各写一遍
     * `world.openContainer(ContainerType::Xxx, pos, player)`。
     *
     * 注意：返回 `ContainerType` 表示"具备打开条件"，但方块自身特有的开箱前置条件
     * （如上锁、被遮挡）仍须由 `useWithoutItem` 检查后再调用 `world.openContainer`。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 菜单类型，无界面时返回 `std::nullopt`
     */
    [[nodiscard]] virtual std::optional<ContainerType> getMenuProvider(
        const BlockState& state, IWorld& world, const BlockPos& pos) const;

    // ========================================================================
    // 方块实体
    // ========================================================================

    /**
     * @brief 检查是否有方块实体
     *
     * @return 如果此方块有对应的方块实体返回true
     */
    [[nodiscard]] virtual bool hasBlockEntity() const noexcept
    {
        return false;
    }

    /**
     * @brief 检查是否为游戏管理员方块
     *
     * 游戏管理员方块需要创造模式 + OP等级>=2 权限才能放置、破坏和交互。
     * 包括命令方块、结构方块、拼图方块等。
     *
     * Block 子类需要通过同时继承 GameMasterBlock 标记接口并重写此方法返回 true
     * 来标记自身为管理员方块。
     *
     *
     * @return 如果此方块为游戏管理员方块返回true
     */
    [[nodiscard]] virtual bool isGameMaster() const noexcept
    {
        return false;
    }

    /**
     * @brief 创建方块实体
     *
     * @param pos 方块位置
     * @return 方块实体，如果无实体返回nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos);

    /**
     * @brief 处理方块事件
     *
     * 当服务端调用 IWorld::blockEvent() 时，事件在服务端执行后广播到客户端。
     * 客户端收到 BlockEventPacket 后，通过 IWorld::blockEvent() 调用此方法。
     *
     * 对于拥有方块实体的方块，默认实现将事件委托给方块实体的 triggerEvent()。
     * 没有方块实体的方块（如音符盒、活塞）应重写此方法以直接处理事件。
     *
     * 参考 MC Java: BlockBehaviour.triggerEvent()
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param id 事件ID
     * @param type 事件类型/数据
     * @return 如果事件被成功处理返回 true
     */
    [[nodiscard]] virtual bool triggerEvent(
        const BlockState& state, IWorld& world, const BlockPos& pos, i32 id, i32 type) const;

    // ========================================================================
    // 红石
    // ========================================================================

    /**
     * @brief 检查是否可以提供红石信号
     *
     * @param state 方块状态
     * @return 如果可以提供信号返回true
     */
    [[nodiscard]] virtual bool canProvidePower(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 检查方块是否可以连接红石线
     *
     * 某些方块（如红石火把、红石块）不能被红石线连接，
     * 但仍然可以输出红石信号。默认实现返回 canProvidePower。
     *
     * @param state 方块状态
     * @param side 连接方向（从红石线的角度看）
     * @return 如果可以连接红石线返回true
     */
    [[nodiscard]] virtual bool canConnectRedstone(const BlockState& state, Direction side) const noexcept
    {
        MC_UNUSED(side);
        return canProvidePower(state);
    }

    /**
     * @brief 检查是否有红石比较器输入覆盖
     *
     * @param state 方块状态
     * @return 如果有比较器输入覆盖返回true
     */
    [[nodiscard]] virtual bool hasComparatorInputOverride(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 获取红石比较器信号
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] virtual i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 获取弱红石信号
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 从信号接收者指向本方块的查询方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] virtual i32 getWeakPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return 0;
    }

    /**
     * @brief 获取强红石信号
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 从信号接收者指向本方块的查询方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] virtual i32 getStrongPower(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return 0;
    }

    // ========================================================================
    // 推动反应
    // ========================================================================

    /**
     * @brief 获取推动反应
     *
     * @param state 方块状态
     * @return 推动反应类型
     */
    [[nodiscard]] virtual Material::PushReaction getPushReaction(const BlockState& state) const;

    // ========================================================================
    // 黏液块/蜂蜜块粘连
    // ========================================================================

    /**
     * @brief 检查方块是否为粘性方块
     *
     * 粘性方块（黏液块、蜂蜜块）可以粘住相邻方块一起被活塞推动。
     *
     * @param state 方块状态
     * @return 如果是粘性方块返回 true
     */
    [[nodiscard]] virtual bool isStickyBlock(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 检查两个方块是否可以粘连
     *
     * 判断此方块是否可以粘住另一个方块。
     * 黏液块可以粘住黏液块和蜂蜜块，蜂蜜块只能粘住蜂蜜块。
     *
     * @param state 当前方块状态
     * @param other 目标方块状态
     * @return 如果可以粘连返回 true
     */
    [[nodiscard]] virtual bool canStickTo(const BlockState& state, const BlockState& other) const noexcept
    {
        MC_UNUSED(state);
        MC_UNUSED(other);
        return false;
    }

    // ========================================================================
    // 火焰相关
    // ========================================================================

    /**
     * @brief 获取方块的可燃性值
     *
     * 返回值范围 0-300，表示方块被点燃和烧毁的概率。
     * - 0: 不可燃
     * - 越高越容易被点燃
     * - >= 100 时可能直接烧毁而非点燃
     *
     * 默认实现返回 0（不可燃）。
     * 可燃方块应重写此方法或通过 FireInfoRegistry 注册。
     *
     * Forge: IForgeBlock.getFlammability()
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文相关可燃性）
     * @param pos 方块位置（可选）
     * @param face 点燃面（可选）
     * @return 可燃性值 (0-300)
     */
    [[nodiscard]] virtual i32 getFlammability(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        Direction face = static_cast<Direction>(255)) const noexcept;

    /**
     * @brief 获取方块的火焰蔓延速度
     *
     * 返回值用于计算火焰蔓延到此方块的速度。
     * 值越高，火焰蔓延越快。
     *
     * 默认实现返回 0（不加速蔓延）。
     * 可燃方块应重写此方法或通过 FireInfoRegistry 注册。
     *
     * Forge: IForgeBlock.getFireSpreadSpeed()
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 方块位置（可选）
     * @param face 蔓延面（可选）
     * @return 火焰蔓延速度
     */
    [[nodiscard]] virtual i32 getFireSpreadSpeed(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        Direction face = static_cast<Direction>(255)) const noexcept;

    /**
     * @brief 检查方块是否为火焰源
     *
     * 返回 true 时火焰不会熄灭。
     * 如下界岩、岩浆等方块应重写此方法返回 true。
     *
     * 参考: Forge IForgeBlock.isFireSource()
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 火焰所在面
     * @return 如果是火源返回 true
     */
    [[nodiscard]] virtual bool isFireSource(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return false;
    }

    /**
     * @brief 方块被点燃时的回调
     *
     * 当方块被火焰点燃时调用，用于执行特殊逻辑（如 TNT 爆炸）。
     * 默认实现为空。
     *
     * 参考: Forge IForgeBlock.catchFire()
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param face 点燃面
     * @param igniter 点燃者（可能为空）
     */
    virtual void catchFire(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction face = static_cast<Direction>(255),
        Entity* igniter = nullptr) const
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(face);
        MC_UNUSED(igniter);
    }

    // ========================================================================
    // 信标光束颜色
    // ========================================================================

    /**
     * @brief 获取信标光束颜色倍数
     *
     * 当方块位于信标上方时，此方法返回该方块对信标光束颜色的影响。
     * 染色玻璃等透明方块会重写此方法返回对应的颜色。
     * 默认实现返回 nullptr，表示不改变光束颜色。
     *
     * 参考: Forge: IForgeBlock#getBeaconColorMultiplier
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 方块位置（可选）
     * @param beaconPos 信标位置（可选）
     * @return RGB 颜色数组指针 {r, g, b}，范围 [0.0, 1.0]；返回 nullptr 表示不修改颜色
     */
    [[nodiscard]] virtual const std::array<f32, 3>* getBeaconColorMultiplier(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const BlockPos* beaconPos = nullptr) const
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(beaconPos);
        return nullptr;
    }

    // ========================================================================
    // 旋转和镜像
    // ========================================================================

    /**
     * @brief 旋转方块状态
     *
     * 用于结构放置时的旋转处理。
     * 默认实现返回原状态（不旋转）。
     *
     * @param state 原状态
     * @param rotation 旋转类型
     * @return 旋转后的状态
     */
    [[nodiscard]] virtual const BlockState& rotate(const BlockState& state, Rotation rotation) const;

    /**
     * @brief 镜像方块状态
     *
     * 用于结构放置时的镜像处理。
     * 默认实现返回原状态（不镜像）。
     *
     * @param state 原状态
     * @param mirror 镜像类型
     * @return 镜像后的状态
     */
    [[nodiscard]] virtual const BlockState& mirror(const BlockState& state, Mirror mirror) const;

    /**
     * @brief 计算活塞推动后的实际落地状态
     *
     * MC 1.21.11 在 PistonMovingBlockEntity.finalTick 中调用 Block.updateFromNeighbourShapes
     * 计算被推动方块落地后的状态；部分方块的 updateShape 除邻居形状更新外，还承担按推动方向
     * 旋转的语义（如釉面陶瓦：推动轴与朝向轴相同时顺时针 90°，垂直时保持朝向）。
     *
     * Cubium 的 updatePostPlacement 没有设置上下文与方向参数，无法获知推动方向，故单独抽象出
     * 本方法，仅由活塞落地路径调用。默认实现返回原状态。
     *
     * @param state 被移动方块的状态
     * @param pushDirection 推动方向（伸出为活塞朝向，收回为其反方向）
     * @return 落地状态
     */
    [[nodiscard]] virtual const BlockState& getStateForMovement(const BlockState& state, Direction pushDirection) const;

    // ========================================================================
    // 静态辅助方法
    // ========================================================================

    /**
     * @brief 检查方块面是否应该被渲染
     *
     * 判断相邻两个方块之间是否需要渲染遮挡面。
     * 用于渲染面剔除优化。
     *
     *
     * @param state 当前方块状态
     * @param world 世界
     * @param pos 当前方块位置
     * @param face 要检查的面方向
     * @return 如果该面应该被渲染返回 true
     */
    [[nodiscard]] static bool shouldSideBeRendered(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction face);

    /**
     * @brief 检查指定位置顶部是否有固体面
     *
     * 用于判断方块是否可以放置在某个位置（如门、栅栏等需要顶部支撑的方块）。
     *
     *
     * @param world 世界
     * @param pos 方块位置
     * @return 如果顶部有固体面返回 true
     */
    [[nodiscard]] static bool hasSolidSideOnTop(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查指定位置是否有足够固体面
     *
     * 检查方块在指定方向是否有固体面。
     *
     *
     * @param world 世界
     * @param pos 方块位置
     * @param direction 检查方向
     * @return 如果有足够固体面返回 true
     */
    [[nodiscard]] static bool hasEnoughSolidSide(IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 检查方块是否在指定方向的面上提供 Center 支撑
     *
     * 对应 MC 1.21.11 net.minecraft.world.level.block.Block#canSupportCenter。
     * 用于钟、灯笼、火把、孢子花、蜡烛等悬挂类方块的支撑判定。
     *
     * 判定逻辑：
     * 1. 获取 pos 处方块的 BlockState
     * 2. 如果该方块属于 #minecraft:unstable_bottom_center 标签（栅栏门），返回 false
     * 3. 否则返回 BlockState.isFaceSturdy(direction, SupportType.CENTER)
     *
     * 与 hasEnoughSolidSide 不同，本方法基于"支撑形状"而非"固体面"判定，
     * 允许非完整方块（如铁砧、楼梯）在中心柱区域提供支撑。
     *
     *
     * @param world 世界只读接口
     * @param pos 方块位置
     * @param direction 检查的面方向（方块贴在该方向的面上）
     * @return 如果提供 Center 支撑返回 true
     */
    [[nodiscard]] static bool canSupportCenter(IWorld& world, const BlockPos& pos, Direction direction);

    /**
     * @brief 检查方块是否在顶面提供 Rigid 支撑
     *
     * 对应 MC 1.21.11 net.minecraft.world.level.block.Block#canSupportRigidBlock。
     * 用于铁轨、压力板等需要外环刚性支撑的方块。
     *
     * 判定逻辑：BlockState.isFaceSturdy(Direction.UP, SupportType.RIGID)
     *
     *
     * @param world 世界只读接口
     * @param pos 方块位置
     * @return 如果顶面提供 Rigid 支撑返回 true
     */
    [[nodiscard]] static bool canSupportRigidBlock(IWorld& world, const BlockPos& pos);

    /**
     * @brief 判断方块面是否填充方形区域
     *
     * 用于判断遮挡形状是否完全覆盖面，影响邻居方块的渲染。
     *
     * 等价于 net.minecraft.block.Block#isFaceFull (1.21.11)
     *
     * @param shape 面的遮挡形状
     * @param direction 面方向
     * @return 如果形状填充整个面返回 true
     */
    [[nodiscard]] static bool doesSideFillSquare(const CollisionShape& shape, Direction direction);

    /**
     * @brief 判断形状的指定面是否完全填充
     *
     * 提取形状在指定方向的面投影，然后判断投影是否覆盖整个单位方块面。
     * 这是 isFaceSturdy 的核心判定逻辑。
     *
     *
     * @param shape 3D 形状（通常是碰撞形状或支撑形状）
     * @param direction 要检查的面方向
     * @return 如果面的投影覆盖整个 1x1 区域返回 true
     */
    [[nodiscard]] static bool isFaceFull(const CollisionShape& shape, Direction direction);

    /**
     * @brief 判断方块是否为连接例外方块
     *
     * 某些方块虽然是固体的，但栅栏、墙、玻璃板等不应与它们建立连接。
     * 这些方块包括：树叶、屏障、雕刻南瓜、南瓜灯、西瓜、南瓜、潜影盒。
     *
     *
     * @param state 方块状态
     * @return 如果是连接例外方块返回 true
     */
    [[nodiscard]] static bool isExceptionForConnection(const BlockState& state);

    /**
     * @brief 当方块碰撞形状增大时，将嵌入方块内的实体向上推出
     *
     * 计算 oldState 和 newState 之间的碰撞形状差异（新增部分），
     * 找到与新形状重叠的实体，并将它们向上推出。
     *
     * 典型用途：
     * - 雪层增加时推出站在上面的实体
     * - 耕地变为泥土时推出上面的实体（FarmlandBlock::turnToDirt）
     *
     *
     * @param oldState 变化前的方块状态
     * @param newState 变化后的方块状态
     * @param world 世界接口
     * @param pos 方块位置
     * @return 变化后的方块状态（即 newState）
     */
    static const BlockState& pushEntitiesUp(
        const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos);

    // ========================================================================
    // 方块形状更新
    // ========================================================================

    /**
     * @brief 方块形状更新的方向迭代顺序
     *
     * 按照 WEST→EAST→NORTH→SOUTH→DOWN→UP 的轴对顺序遍历邻居方向。
     * 同轴的两个方向连续处理，确保方块形状更新在轴向上一致收敛。
     */
    static constexpr std::array<Direction, 6> UPDATE_SHAPE_ORDER = {
        Direction::West, Direction::East, Direction::North, Direction::South, Direction::Down, Direction::Up};

    /**
     * @brief 根据邻居方块状态更新当前方块的形状
     *
     * 遍历 UPDATE_SHAPE_ORDER 中的 6 个方向，对每个方向调用 updatePostPlacement，
     * 累积结果并返回最终状态。累积过程中状态一旦变为空气即提前结束：
     * 原版在 airState 上继续迭代时 updateShape 恒返回自身，Cubium 直接短路可得到相同结果，
     * 且避免 `state = air.updatePostPlacement(...)` 把状态重新写回非空气值。
     *
     * 用于区块后处理生成、结构放置、活塞移动等场景。
     *
     * @param state 当前方块状态
     * @param world 世界接口
     * @param pos 当前方块位置
     * @return 根据邻居状态更新后的方块状态
     */
    [[nodiscard]] static BlockState updateFromNeighbourShapes(
        const BlockState& state, IWorld& world, const BlockPos& pos);

    /**
     * @brief 更新或销毁方块：新状态为空气时摧毁方块，否则写入新状态。
     *
     * 形状更新级联的统一出口。新状态为空气时调用 IWorld::destroyBlock：产生破坏粒子、
     * dropBlock 为 true 时按掉落表生成掉落、发送 BLOCK_DESTROY 游戏事件；flags 含
     * UPDATE_SUPPRESS_DROPS 时传 dropBlock=false 抑制掉落。新状态非空气时写入新状态，
     * 并剥掉该标志位以免语义泄漏到下游写入。
     *
     * 等价于传入 BlockUpdateFlags::DEFAULT_DESTROY_RECURSION。
     *
     * @param oldState 变化前的方块状态
     * @param newState 变化后的方块状态
     * @param world 世界接口
     * @param pos 方块位置
     * @param flags 更新标志
     */
    static void updateOrDestroy(
        const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos, i32 flags);

    /**
     * @brief 更新或销毁方块（指定形状更新递归预算）
     *
     * 对应 MC Java: Block.updateOrDestroy(oldState, newState, level, pos, flags, recursionLeft)。
     * recursionLeft 是形状更新级联的深度预算，变空气时原样传给 IWorld::destroyBlock。
     *
     * @param oldState 变化前的方块状态
     * @param newState 变化后的方块状态
     * @param world 世界接口
     * @param pos 方块位置
     * @param flags 更新标志
     * @param recursionLeft 剩余的形状更新递归预算
     */
    static void updateOrDestroy(const BlockState& oldState,
        const BlockState& newState,
        IWorld& world,
        const BlockPos& pos,
        i32 flags,
        i32 recursionLeft);

    // ========================================================================
    // 攻击和交互
    // ========================================================================

    /**
     * @brief 玩家左键攻击方块
     *
     * 当玩家左键点击（攻击）方块时调用。
     * 默认实现为空，需要特殊行为的方块（如 TNT、创造模式等）应重写。
     *
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     */
    virtual void attack(const BlockState& state, IWorld& world, const BlockPos& pos, Player& player)
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(player);
    }

    /**
     * @brief 投掷物击中方块
     *
     * 当投掷物（箭、三叉戟等）击中方块时调用。
     * 默认实现为空，需要特殊行为的方块应重写。
     *
     *
     * @param world 世界
     * @param state 方块状态
     * @param hitResult 击中结果
     * @param projectile 投掷物实体
     */
    virtual void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
    {
        MC_UNUSED(world);
        MC_UNUSED(state);
        MC_UNUSED(hitResult);
        MC_UNUSED(projectile);
    }

    /**
     * @brief 实体摔落在方块上
     *
     * 当实体从高处摔落到方块上时调用。
     * 默认实现调用 entity.causeFallDamage() 施加普通摔落伤害。
     * 子类可重写此方法以自定义摔落行为：
     * - 石笋方块：施加增大的石笋伤害，不调用父类方法（替代普通摔落伤害）
     * - 海龟蛋方块：不调用父类方法（取消摔落伤害）
     * - 耕地方块：先执行踩踏逻辑，再调用父类方法（保留普通摔落伤害）
     *
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @param entity 摔落的实体
     * @param fallDistance 摔落距离
     */
    virtual void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance);

    /**
     * @brief 降水处理
     *
     * 在降水 tick 中对每个降水位置调用，让方块响应降水（雨/雪）。
     * 默认实现为空。方块可以重写此方法来响应降水，例如：
     * - 炼药锅在雨天填充水、在雪天填充细雪
     * - 避雷针在雷暴时被激活
     *
     * 调用条件：世界正在下雨（isRaining()）且生物群系的降水类型不为 None。
     *
     *
     * @param world 世界
     * @param pos 方块位置
     * @param precipitation 降水类型（Rain / Snow）
     */
    virtual void handlePrecipitation(
        IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation)
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(precipitation);
    }

    /**
     * @brief 玩家收割方块
     *
     * 当玩家成功破坏方块时调用，处理掉落物和统计数据。
     * 默认实现调用掉落处理。
     *
     *
     * @param world 世界
     * @param player 玩家
     * @param pos 方块位置
     * @param state 方块状态
     * @param blockEntity 方块实体（可能为空）
     * @param stack 使用工具（可能为空）
     */
    virtual void harvestBlock(IWorld& world,
        Player& player,
        const BlockPos& pos,
        const BlockState& state,
        BlockEntity* blockEntity,
        const ItemStack* stack);

    /**
     * @brief 获取玩家相对硬度
     *
     * 计算玩家挖掘方块的速度倍率。
     * 考虑工具效率、附魔效果、水下挖掘等因素。
     *
     *
     * @param player 玩家
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     * @return 相对硬度值（越大越容易挖掘）
     */
    [[nodiscard]] virtual f32 getPlayerRelativeBlockHardness(
        Player& player, IBlockReader& world, const BlockPos& pos, const BlockState& state) const;

    /**
     * @brief 获取渲染类型
     *
     * 返回方块的渲染类型：
     * - MODEL: 正常模型渲染
     * - INVISIBLE: 不可见（空气）
     * - LIQUID: 液体渲染
     * - ENTITYBLOCK_ANIMATED: 方块实体动画渲染
     *
     *
     * @param state 方块状态
     * @return 渲染类型
     */

    [[nodiscard]] virtual BlockRenderType getRenderType(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return BlockRenderType::MODEL;
    }

    /**
     * @brief 检查是否允许移动（路径查找）
     *
     * 用于实体路径查找系统判断是否可以穿过方块。
     * 默认返回材质的 blocksMovement 取反。
     *
     *
     * @param state 方块状态
     * @param world 世界读取器
     * @param pos 方块位置
     * @return 如果允许移动返回 true
     */
    [[nodiscard]] virtual bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return !state.blocksMovement();
    }

    /**
     * @brief 实体能否在该方块占用的位置内重生
     *
     * 默认：方块不是固体、也不是液体。压力板、告示牌、旗帜等方块重写为 true。
     * 仅用于强制重生点的落点合法性判定（床/重生锚走各自的站位搜索）。
     *
     *
     * @param state 方块状态
     * @return 允许在方块内部重生返回 true
     */
    [[nodiscard]] virtual bool isPossibleToRespawnInThis(const BlockState& state) const
    {
        return !state.isSolid() && !state.isLiquid();
    }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual std::string toString() const noexcept
    {
        return m_blockLocation.toString();
    }

protected:
    friend class BlockRegistry;
    friend class BlockState;

    /**
     * @brief 构造方块
     */
    explicit Block(BlockProperties properties);

    /**
     * @brief 创建方块状态容器
     */
    void createBlockState(std::unique_ptr<StateContainer<Block, BlockState>> container);

    /**
     * @brief 设置默认状态
     */
    void setDefaultState(const BlockState& state);

    // 由BlockRegistry设置
    ResourceLocation m_blockLocation;
    u32 m_blockId = 0;

    // 由构造函数设置
    const Material* m_material;
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u8 m_lightLevel = 0;
    i32 m_opacity = 15; // 默认完全不透明
    bool m_hasCollision = true;
    bool m_isSolid = true;
    bool m_forceSolidOn = false;  // 强制视为实体方块（覆盖 isSolid 的形状推断）
    bool m_forceSolidOff = false; // 强制不视为实体方块（覆盖 isSolid 的形状推断）
    /// 窒息谓词；空表示沿用默认判定（blocksMotion && 碰撞形状满格）
    const StatePredicate* m_isSuffocating = nullptr;
    /// 视线谓词；空表示沿用窒息判定
    const StatePredicate* m_isViewBlocking = nullptr;

protected:
    bool m_isFlammable = false;
    bool m_canOcclude = true; // 是否参与遮挡计算（vanilla canOcclude，noOcclusion() 关闭）
    bool m_propagatesSkylightDown = false;
    bool m_propagatesSkylightDownExplicit = false; // 是否显式设置过 propagatesSkylightDown（区分显式值与默认哨兵）
    bool m_requiresTool = false;
    bool m_isReplaceable = false; // 是否可被替换
    bool m_ticksRandomly = false; // 是否响应随机刻
    u8 m_harvestTool = HarvestTool::None;
    i32 m_harvestLevel = 0;

    // 地图颜色（用于在地图上渲染方块颜色）
    world::map::MaterialColorId m_mapColor = world::map::MaterialColorId::AIR;
    using MapColorResolver = world::map::MaterialColorId (*)(const BlockState&);
    MapColorResolver m_mapColorResolver = nullptr;
    bool m_hasMapColor = false; // 是否显式设置了地图颜色（否则使用材质颜色）

    // 掉落表ID（注册时若为空且未禁止，将自动推导为 "<namespace>:blocks/<path>"）
    std::string m_lootTableId;
    bool m_noLootTable = false; // 显式禁止掉落表

    // 声音类型（默认为石头声音）
    const BlockSoundType* m_soundType = &BlockSoundTypes::STONE;

    // 物理属性
    f32 m_slipperiness = 0.6f; // 默认滑度
    f32 m_speedFactor = 1.0f;  // 默认速度因子
    f32 m_jumpFactor = 1.0f;   // 默认跳跃因子

    // 1.17+ 扩展属性
    bool m_ignitedByLava = false;                         // 是否可被岩浆点燃
    BlockOffsetType m_offsetType = BlockOffsetType::None; // 模型偏移类型
    /// 音符盒乐器：未显式设置时按材质推导（见 getInstrument）
    std::optional<NoteBlockInstrument> m_instrument;

    // 由createBlockState设置
    std::unique_ptr<StateContainer<Block, BlockState>> m_stateContainer;
    const BlockState* m_defaultState = nullptr;
};

} // namespace mc
