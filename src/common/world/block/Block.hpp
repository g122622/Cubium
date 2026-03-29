#pragma once

#include "../../core/Types.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../physics/collision/CollisionShape.hpp"
#include "../../util/property/StateHolder.hpp"
#include "../../util/property/StateContainer.hpp"
#include "../../util/Direction.hpp"
#include "../../util/assert/AssertAll.hpp"
#include "../../core/BlockRaycastResult.hpp"
#include "Material.hpp"
#include "HarvestTool.hpp"
#include "BlockSoundType.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace mc {
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
}
}

// Forward declarations
class Block;
class BlockState;
class BlockRegistry;
class IWorld;
class IBlockReader;
class BlockPos;
class World;
class BlockItemUseContext;
class Player;
class BlockEntity;
class Entity;

namespace math {
class IRandom;
}

namespace loot {
class LootTableManager;
class LootTable;
}

namespace fluid {
class FluidState;
} // namespace fluid

/**
 * @brief VoxelShape工具类
 *
 * 提供常用碰撞形状的静态实例。
 */
class VoxelShapes {
public:
    /**
     * @brief 获取空形状
     */
    static const CollisionShape& empty();

    /**
     * @brief 获取完整方块形状
     */
    static const CollisionShape& fullCube();

    /**
     * @brief 创建方块形状
     * @param x1, y1, z1 起始坐标 (像素，0-16)
     * @param x2, y2, z2 结束坐标 (像素，0-16)
     */
    static CollisionShape cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2);
};

/**
 * @brief 方块状态
 *
 * 不可变的方块状态对象，包含方块的所有属性值。
 * 继承自StateHolder以支持O(1)的状态转换。
 *
 * 参考: net.minecraft.block.BlockState
 */
class BlockState : public StateHolder<Block, BlockState> {
public:
    /**
     * @brief 构造方块状态
     */
    BlockState(const Block& block,
               std::unordered_map<const IProperty*, size_t> values,
               u32 stateId);

    /**
     * @brief 是否为空气
     */
    [[nodiscard]] bool isAir() const;

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
    [[nodiscard]] bool is(const Block* block) const {
        return block != nullptr && &owner() == block;
    }

    /**
     * @brief 获取此状态所属的方块
     * @return 方块引用
     */
    [[nodiscard]] const Block& getBlock() const {
        return owner();
    }

    /**
     * @brief 获取光照透明度 (0-15)
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getOpacity
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
     * @brief 是否有不透明碰撞形状
     *
     * 用于环境光遮蔽(AO)计算。如果方块有不透明的完整碰撞箱，
     * 则返回true，导致周围顶点变暗。
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#hasOpaqueCollisionShape
     */
    [[nodiscard]] bool hasOpaqueCollisionShape() const;

    /**
     * @brief 获取环境光遮蔽亮度值
     *
     * 返回值用于AO计算：
     * - 0.2f: 方块有不透明碰撞形状（实心方块），产生阴影
     * - 1.0f: 方块无碰撞或透明（玻璃、树叶等），不产生阴影
     *
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getAmbientOcclusionLightValue
     */
    [[nodiscard]] float getAmbientOcclusionLightValue() const;

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
     * @brief 检查是否为不透明完整方块
     *
     * 用于渲染和光照计算。
     *
     * @param world 世界读取器
     * @param pos 方块位置
     * @return 如果是不透明完整方块返回true
     */
    [[nodiscard]] bool isOpaqueCube(IBlockReader& world, const BlockPos& pos) const;

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
     * @brief 转换为模型键（用于查找模型变体）
     * @return 格式: "axis=y,facing=north" 或 "" (无属性时)
     */
    [[nodiscard]] String toModelKey() const;

protected:
    /**
     * @brief 获取拥有者名称
     */
    [[nodiscard]] String ownerName() const override;

private:
    friend class Block;
    friend class BlockRegistry;

    /**
     * @brief 缓存方块属性
     */
    void cacheProperties();

    // 缓存的属性
    bool m_isSolid = true;
    bool m_isOpaque = true;
    bool m_blocksMovement = false;
    bool m_isLiquid = false;
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;
    u8 m_lightLevel = 0;
    u8 m_harvestTool = 0;  // HarvestTool::None
    i32 m_opacity = 15;  // 默认完全不透明
    i32 m_harvestLevel = 0;
    f32 m_hardness = 0.0f;
    f32 m_resistance = 0.0f;
    u32 m_blockId = 0;
};

/**
 * @brief 方块属性构建器
 *
 * 用于构建方块属性的流畅接口。
 *
 * 参考: net.minecraft.block.AbstractBlock.Properties
 *
 * 用法示例:
 * @code
 * auto properties = BlockProperties(Material::ROCK)
 *     .hardness(1.5f)
 *     .resistance(6.0f)
 *     .requiresTool();
 * @endcode
 */
class BlockProperties {
public:
    /**
     * @brief 构造方块属性
     * @param material 材质
     */
    explicit BlockProperties(const Material& material);

    /**
     * @brief 设置硬度
     */
    BlockProperties& hardness(f32 value);

    /**
     * @brief 设置抗性
     */
    BlockProperties& resistance(f32 value);

    /**
     * @brief 设置光照等级
     */
    BlockProperties& lightLevel(u8 level);

    /**
     * @brief 设置无碰撞
     */
    BlockProperties& noCollision();

    /**
     * @brief 设置非固体
     */
    BlockProperties& notSolid();

    /**
     * @brief 设置需要工具
     */
    BlockProperties& requiresTool();

    /**
     * @brief 设置可燃性
     */
    BlockProperties& flammable(bool value = true);

    /**
     * @brief 设置可替换
     */
    BlockProperties& replaceable();

    /**
     * @brief 设置强度（同时设置硬度和抗性）
     */
    BlockProperties& strength(f32 value);

    /**
     * @brief 设置光照透明度
     *
     * @param value 透明度值 (0-15)
     *   - 0: 完全透明，光线无衰减通过
     *   - 1-14: 部分透明，光线衰减指定等级
     *   - 15: 完全不透明，阻挡所有光线
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties#opacity
     */
    BlockProperties& opacity(i32 value);

    /**
     * @brief 设置是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties#propagatesSkylightDown
     */
    BlockProperties& propagatesSkylightDown(bool value = true);

    /**
     * @brief 设置挖掘工具类型
     *
     * 指定采集此方块所需的工具类型。
     * 如果设置了工具类型且 requiresTool() 为 true，
     * 则必须使用正确类型的工具才能获得掉落物。
     *
     * @param toolType 工具类型值（HarvestTool::Pickaxe 等）
     */
    BlockProperties& harvestTool(u8 toolType);

    /**
     * @brief 设置挖掘等级
     *
     * 指定采集此方块所需的最低工具等级。
     * - 0: 木/金工具可采集
     * - 1: 石制工具可采集
     * - 2: 铁制工具可采集
     * - 3: 钻石工具可采集
     * - 4: 下界合金工具可采集
     *
     * @param level 挖掘等级（0-4）
     */
    BlockProperties& harvestLevel(i32 level);

    /**
     * @brief 设置掉落表ID
     *
     * 指定方块被破坏时使用的掉落表。
     *
     * @param lootTableId 掉落表ID（如 "minecraft:blocks/diamond_ore"）
     */
    BlockProperties& lootTableId(const String& id) {
        m_lootTableId = id;
        return *this;
    }

    /**
     * @brief 设置声音类型
     *
     * 设置方块的破坏、踩踏、放置、击打、坠落声音。
     *
     * @param soundType 声音类型的引用（如 BlockSoundTypes::STONE）
     */
    BlockProperties& soundType(const BlockSoundType& soundType) {
        m_soundType = &soundType;
        return *this;
    }

    // Getters
    [[nodiscard]] const Material& material() const { return *m_material; }
    [[nodiscard]] f32 hardness() const { return m_hardness; }
    [[nodiscard]] f32 resistance() const { return m_resistance; }
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }
    [[nodiscard]] bool hasCollision() const { return m_hasCollision; }
    [[nodiscard]] bool isSolid() const { return m_isSolid; }
    [[nodiscard]] bool isFlammable() const { return m_isFlammable; }
    [[nodiscard]] bool requiresTool() const { return m_requiresTool; }
    [[nodiscard]] bool isReplaceable() const { return m_isReplaceable; }
    [[nodiscard]] i32 opacity() const { return m_opacity; }
    [[nodiscard]] bool doesPropagateSkylightDown() const { return m_propagatesSkylightDown; }
    [[nodiscard]] u8 harvestTool() const { return m_harvestTool; }
    [[nodiscard]] i32 harvestLevel() const { return m_harvestLevel; }
    [[nodiscard]] const String& lootTableId() const { return m_lootTableId; }
    [[nodiscard]] const BlockSoundType* soundType() const { return m_soundType; }

private:
    friend class Block;
    friend class BlockRegistry;

    const Material* m_material;
    f32 m_hardness;
    f32 m_resistance;
    u8 m_lightLevel;
    bool m_hasCollision;
    bool m_isSolid;
    bool m_isFlammable;
    bool m_requiresTool;
    bool m_isReplaceable;
    i32 m_opacity = 15;  // 默认完全不透明
    bool m_propagatesSkylightDown = false;
    u8 m_harvestTool = HarvestTool::None;
    i32 m_harvestLevel = 0;
    String m_lootTableId;
    const BlockSoundType* m_soundType = &BlockSoundTypes::STONE;  // 默认使用石头声音
};

/**
 * @brief 方块基类
 *
 * 所有方块类型的基类。方块通过BlockRegistry注册，
 * 每个方块有一个或多个BlockState表示不同状态。
 *
 * 参考: net.minecraft.block.Block
 *
 * 用法示例:
 * @code
 * class StoneBlock : public Block {
 * public:
 *     StoneBlock() : Block(BlockProperties(Material::ROCK).hardness(1.5f)) {
 *         auto container = StateContainer<Block, BlockState>::Builder(*this)
 *             .create([](const Block& block, auto values, u32 id) {
 *                 return std::make_unique<BlockState>(block, values, id);
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
    [[nodiscard]] const ResourceLocation& blockLocation() const { return m_blockLocation; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const { return m_blockId; }

    /**
     * @brief 获取材质
     */
    [[nodiscard]] const Material& material() const { return *m_material; }

    /**
     * @brief 获取状态容器
     */
    [[nodiscard]] const StateContainer<Block, BlockState>& stateContainer() const { return *m_stateContainer; }

    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] const BlockState& defaultState() const { return *m_defaultState; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const { return m_resistance; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const { return m_lightLevel; }

    /**
     * @brief 获取光照透明度 (0-15)
     */
    [[nodiscard]] i32 opacity() const { return m_opacity; }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool doesPropagateSkylightDown() const { return m_propagatesSkylightDown; }

    /**
     * @brief 获取挖掘工具类型
     *
     * 返回采集此方块所需的工具类型。
     * 如果返回 HarvestTool::None，则不需要特定工具。
     *
     * @return 工具类型值
     */
    [[nodiscard]] u8 harvestTool() const { return m_harvestTool; }

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
    [[nodiscard]] i32 harvestLevel() const { return m_harvestLevel; }

    /**
     * @brief 是否需要正确工具才能采集
     *
     * 如果返回 true，则必须使用正确类型且等级足够的工具
     * 才能获得方块掉落物。
     *
     * @return 是否需要正确工具
     */
    [[nodiscard]] bool requiresTool() const { return m_requiresTool; }

    // ========== 声音类型 ==========

    /**
     * @brief 获取方块的声音类型
     *
     * 返回方块的声音类型，包含破坏、踩踏、放置、击打、坠落声音。
     *
     * @return 声音类型的常量引用
     */
    [[nodiscard]] const BlockSoundType& getSoundType() const { return *m_soundType; }

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
    [[nodiscard]] virtual String getLootTableId() const { return m_lootTableId; }

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
    void setLootTableId(const String& id) { m_lootTableId = id; }

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
     * @brief 获取遮挡形状
     * @param state 方块状态
     * @return 形状引用
     */
    [[nodiscard]] virtual const CollisionShape& getOcclusionShape(const BlockState& state) const;

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
     * @brief 是否不透明
     * @param state 方块状态
     */
    [[nodiscard]] virtual bool isOpaque(const BlockState& state) const;

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
     * 参考: net.minecraft.block.BlockState#getOpacity
     *
     * @param state 方块状态
     * @param world 世界（可选，用于上下文相关透明度）
     * @param pos 位置（可选）
     * @return 光照透明度 (0-15)
     */
    [[nodiscard]] virtual i32 getOpacity(const BlockState& state,
                                          IWorld* world = nullptr,
                                          const BlockPos* pos = nullptr) const;

    /**
     * @brief 检查是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     * 参考: net.minecraft.block.BlockState#propagatesSkylightDown
     *
     * @param state 方块状态
     * @param world 世界（可选）
     * @param pos 位置（可选）
     * @return 如果天空光可以传播返回true
     */
    [[nodiscard]] virtual bool propagatesSkylightDown(const BlockState& state,
                                                       IWorld* world = nullptr,
                                                       const BlockPos* pos = nullptr) const;

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
    [[nodiscard]] virtual bool isSolidSide(const BlockState& state, IWorld& world,
                                           const BlockPos& pos, Direction side) const;

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
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void tick(IWorld& world, const BlockPos& pos, BlockState& state);

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
    virtual void neighborChanged(IWorld& world, const BlockPos& pos,
                                  Block& neighborBlock, const BlockPos& neighborPos,
                                  bool isMoving);

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
    virtual void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state);

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
    virtual void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 实体与方块碰撞时调用
     *
     * 当实体进入方块的碰撞区域时调用。用于特殊方块行为，如漏斗收集物品。
     * 默认实现为空。
     *
     * @param state 方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param entity 碰撞的实体
     */
    virtual void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
    }

    /**
     * @brief 是否响应随机刻
     *
     * 返回true时，该方块会被随机刻系统选中执行randomTick。
     * 默认返回false。
     *
     * @return 是否响应随机刻
     */
    [[nodiscard]] virtual bool ticksRandomly() const { return false; }

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
     * 在方块被玩家放置后调用。
     * 默认实现为空。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 方块状态
     */
    virtual void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state);

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
    [[nodiscard]] virtual BlockState updatePostPlacement(
        const BlockState& state,
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
    [[nodiscard]] virtual bool isValidPosition(
        const BlockState& state,
        IBlockReader& world,
        const BlockPos& pos) const;

    // ========================================================================
    // 交互
    // ========================================================================

    /**
     * @brief 玩家右键点击
     *
     * 当玩家右键点击方块时调用。
     * 默认实现返回 Pass。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果
     */
    [[nodiscard]] virtual ActionResult onBlockActivated(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit);

    // ========================================================================
    // 方块实体
    // ========================================================================

    /**
     * @brief 检查是否有方块实体
     *
     * @return 如果此方块有对应的方块实体返回true
     */
    [[nodiscard]] virtual bool hasBlockEntity() const { return false; }

    /**
     * @brief 创建方块实体
     *
     * @param pos 方块位置
     * @return 方块实体，如果无实体返回nullptr
     */
    [[nodiscard]] virtual std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos);

    // ========================================================================
    // 红石
    // ========================================================================

    /**
     * @brief 检查是否可以提供红石信号
     *
     * @param state 方块状态
     * @return 如果可以提供信号返回true
     */
    [[nodiscard]] virtual bool canProvidePower(const BlockState& state) const {
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
    [[nodiscard]] virtual bool canConnectRedstone(const BlockState& state, Direction side) const {
        MC_UNUSED(side);
        return canProvidePower(state);
    }

    /**
     * @brief 检查是否有红石比较器输入覆盖
     *
     * @param state 方块状态
     * @return 如果有比较器输入覆盖返回true
     */
    [[nodiscard]] virtual bool hasComparatorInputOverride(const BlockState& state) const {
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
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos) const;

    /**
     * @brief 获取弱红石信号
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param side 信号输出方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] virtual i32 getWeakPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side) const {
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
     * @param side 信号输出方向
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] virtual i32 getStrongPower(
        const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Direction side) const {
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
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual String toString() const {
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
    i32 m_opacity = 15;  // 默认完全不透明
    bool m_hasCollision = true;
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;
    bool m_requiresTool = false;
    u8 m_harvestTool = HarvestTool::None;
    i32 m_harvestLevel = 0;

    // 掉落表ID（默认为空，表示无自定义掉落表）
    String m_lootTableId;

    // 声音类型（默认为石头声音）
    const BlockSoundType* m_soundType = &BlockSoundTypes::STONE;

    // 由createBlockState设置
    std::unique_ptr<StateContainer<Block, BlockState>> m_stateContainer;
    const BlockState* m_defaultState = nullptr;
};

} // namespace mc
