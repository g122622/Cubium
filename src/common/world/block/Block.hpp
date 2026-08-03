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

#include "../../core/BlockRaycastResult.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../item/core/BlockActionResult.hpp"
#include "../../util/Direction.hpp"
#include "../../util/assert/AssertAll.hpp"
#include "../../util/property/StateContainer.hpp"
#include "BlockSoundType.hpp"
#include "BlockState.hpp"
#include "HarvestTool.hpp"
#include "IBlockAnimateContext.hpp"
#include "Material.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "world/biome/BiomeClimate.hpp"
#include "world/map/MaterialColor.hpp"
#include <array>
#include <functional>
#include <memory>
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
     * @brief 方块模型偏移类型
     *
     * 控制方块在渲染时是否应用随机位置偏移，
     * 用于植物、农作物等需要视觉多样性的方块。
     *
     * 参考: net.minecraft.world.level.block.BlockBehaviour.OffsetType
     */
    enum class OffsetType : u8 {
        None = 0, ///< 无偏移（默认）
        XZ = 1,   ///< 仅水平方向偏移
        XYZ = 2   ///< 三个方向都偏移
    };

    /**
     * @brief 音符盒乐器类型
     *
     * 当音符盒放置在此方块上方时演奏的乐器。
     *
     * 参考: net.minecraft.world.level.block.NoteBlockInstrument
     */
    enum class Instrument : u8 {
        Harp = 0,           ///< 竖琴（默认）
        BaseDrum = 1,       ///< 大鼓（石头类方块）
        Snare = 2,          ///< 小军鼓（沙子类方块）
        Hat = 3,            ///< 踩镲（玻璃类方块）
        Bass = 4,           ///< 贝斯（木材类方块）
        Flute = 5,          ///< 长笛（泥土类方块）
        Bell = 6,           ///< 钟（金块）
        Guitar = 7,         ///< 吉他（羊毛类方块）
        Chime = 8,          ///< 风铃（冰类方块）
        Xylophone = 9,      ///< 木琴（骨块）
        IronXylophone = 10, ///< 铁琴（铁块）
        CowBell = 11,       ///< 牛铃（灵魂沙）
        Didgeridoo = 12,    ///< 迪吉里杜管（南瓜类）
        Bit = 13,           ///< 电子音（绿宝石块）
        Banjo = 14,         ///< 班卓琴（干草块）
        Pling = 15          ///< 电钢琴（萤石）
    };

    /**
     * @brief 构造方块属性
     * @param material 材质
     */
    explicit BlockProperties(const Material& material);

    /**
     * @brief 设置硬度
     */
    BlockProperties& hardness(f32 value) noexcept
    {
        m_hardness = value;
        return *this;
    }

    /**
     * @brief 设置抗性
     */
    BlockProperties& resistance(f32 value) noexcept
    {
        m_resistance = value;
        return *this;
    }

    /**
     * @brief 设置光照等级
     */
    BlockProperties& lightLevel(u8 level) noexcept
    {
        m_lightLevel = level > 15 ? 15 : level;
        return *this;
    }

    /**
     * @brief 设置无碰撞
     */
    BlockProperties& noCollision() noexcept
    {
        m_hasCollision = false;
        return *this;
    }

    /**
     * @brief 设置非固体
     */
    BlockProperties& notSolid() noexcept
    {
        m_isSolid = false;
        return *this;
    }

    /**
     * @brief 设置需要工具
     */
    BlockProperties& requiresTool() noexcept
    {
        m_requiresTool = true;
        return *this;
    }

    /**
     * @brief 设置可燃性
     */
    BlockProperties& flammable(bool value = true) noexcept
    {
        m_isFlammable = value;
        return *this;
    }

    /**
     * @brief 设置可替换
     */
    BlockProperties& replaceable() noexcept
    {
        m_isReplaceable = true;
        return *this;
    }

    /**
     * @brief 设置强度（同时设置硬度和抗性）
     */
    BlockProperties& strength(f32 value) noexcept
    {
        m_hardness = value;
        m_resistance = value;
        return *this;
    }

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
    BlockProperties& opacity(i32 value) noexcept
    {
        m_opacity = value < 0 ? 0 : (value > 15 ? 15 : value);
        return *this;
    }

    /**
     * @brief 设置是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties#propagatesSkylightDown
     */
    BlockProperties& propagatesSkylightDown(bool value = true) noexcept
    {
        m_propagatesSkylightDown = value;
        return *this;
    }

    /**
     * @brief 设置挖掘工具类型
     *
     * 指定采集此方块所需的工具类型。
     * 如果设置了工具类型且 requiresTool() 为 true，
     * 则必须使用正确类型的工具才能获得掉落物。
     *
     * @param toolType 工具类型值（HarvestTool::Pickaxe 等）
     */
    BlockProperties& harvestTool(u8 toolType) noexcept
    {
        m_harvestTool = toolType;
        return *this;
    }

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
    BlockProperties& harvestLevel(i32 level) noexcept
    {
        m_harvestLevel = level < 0 ? 0 : level;
        return *this;
    }

    /**
     * @brief 设置掉落表ID
     *
     * 指定方块被破坏时使用的掉落表。
     *
     * @param lootTableId 掉落表ID（如 "minecraft:blocks/diamond_ore"）
     */
    BlockProperties& lootTableId(const std::string& id) noexcept
    {
        m_lootTableId = id;
        m_noLootTable = false;
        return *this;
    }

    /**
     * @brief 禁止自动推导掉落表ID
     *
     * 适用于不应有任何掉落物的方块（如空气），
     * 防止注册时自动推导出 "minecraft:blocks/air" 等无意义掉落表ID。
     */
    BlockProperties& noLootTable() noexcept
    {
        m_noLootTable = true;
        m_lootTableId.clear();
        return *this;
    }

    /**
     * @brief 是否显式禁止掉落表
     */
    [[nodiscard]] bool noLootTable() const noexcept { return m_noLootTable; }

    /**
     * @brief 设置声音类型
     *
     * 设置方块的破坏、踩踏、放置、击打、坠落声音。
     *
     * @param soundType 声音类型的引用（如 BlockSoundTypes::STONE）
     */
    BlockProperties& soundType(const BlockSoundType& soundType) noexcept
    {
        m_soundType = &soundType;
        return *this;
    }

    /**
     * @brief 设置滑度
     *
     * 设置方块的滑动系数，影响实体在方块上的移动阻力。
     * - 0.6f: 默认值（普通方块如石头、泥土、蜂蜜块）
     * - 0.8f: 史莱姆块
     * - 0.98f: 冰、浮冰
     * - 0.989f: 蓝冰
     *
     * 注意：蜂蜜块的减速效果通过 speedFactor(0.4) 和 jumpFactor(0.5) 实现，而非修改滑度。
     *
     * 参考: net.minecraft.block.Block.friction
     *
     * @param value 滑度值 (0.0-1.0)
     */
    BlockProperties& slipperiness(f32 value) noexcept
    {
        m_slipperiness = value;
        return *this;
    }

    /**
     * @brief 设置速度因子
     *
     * 设置方块的速度影响系数。
     * - 1.0f: 默认值（正常速度）
     * - 0.5f: 蜂蜜块（减速）
     *
     * 参考: net.minecraft.block.Block.speedFactor
     *
     * @param value 速度因子值
     */
    BlockProperties& speedFactor(f32 value) noexcept
    {
        m_speedFactor = value;
        return *this;
    }

    /**
     * @brief 设置跳跃因子
     *
     * 设置方块的跳跃影响系数。
     * - 1.0f: 默认值（正常跳跃）
     *
     * 参考: net.minecraft.block.Block.jumpFactor
     *
     * @param value 跳跃因子值
     */
    BlockProperties& jumpFactor(f32 value) noexcept
    {
        m_jumpFactor = value;
        return *this;
    }

    /**
     * @brief 设置是否响应随机刻
     *
     * 如果设置为 true，该方块会被随机刻系统选中执行 randomTick。
     * 用于农作物生长、铜氧化、冰融化等需要随机更新的方块。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties.tickRandomly
     *
     * @return 属性构建器引用
     */
    BlockProperties& tickRandomly() noexcept
    {
        m_ticksRandomly = true;
        return *this;
    }

    /**
     * @brief 设置地图颜色
     *
     * 用于在地图上渲染此方块的颜色。如果不设置，默认使用材质的颜色。
     *
     * @param color 地图颜色ID
     */
    BlockProperties& mapColor(world::map::MaterialColorId color) noexcept
    {
        m_mapColor = color;
        m_hasMapColor = true;
        return *this;
    }

    /**
     * @brief 设置是否可被岩浆点燃
     *
     * 当为 true 时，方块接触岩浆会被点燃（如木头、TNT）。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties.ignitedByLava
     */
    BlockProperties& ignitedByLava(bool value = true) noexcept
    {
        m_ignitedByLava = value;
        return *this;
    }

    /**
     * @brief 设置模型偏移类型
     *
     * 控制方块渲染时的随机位置偏移。
     * - None: 无偏移（默认，大部分方块）
     * - XZ: 仅水平偏移（植物类如花、草）
     * - XYZ: 三轴偏移（如菌类）
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties.offsetType
     */
    BlockProperties& offsetType(OffsetType type) noexcept
    {
        m_offsetType = type;
        return *this;
    }

    /**
     * @brief 设置音符盒乐器
     *
     * 当音符盒放置在此方块上方时演奏的乐器。
     *
     * 参考: net.minecraft.block.AbstractBlock.Properties.instrument
     */
    BlockProperties& instrument(Instrument instr) noexcept
    {
        m_instrument = instr;
        return *this;
    }

    // Getters
    [[nodiscard]] const Material& material() const noexcept { return *m_material; }
    [[nodiscard]] f32 hardness() const noexcept { return m_hardness; }
    [[nodiscard]] f32 resistance() const noexcept { return m_resistance; }
    [[nodiscard]] u8 lightLevel() const noexcept { return m_lightLevel; }
    [[nodiscard]] bool hasCollision() const noexcept { return m_hasCollision; }
    [[nodiscard]] bool isSolid() const noexcept { return m_isSolid; }
    [[nodiscard]] bool isFlammable() const noexcept { return m_isFlammable; }
    [[nodiscard]] bool requiresTool() const noexcept { return m_requiresTool; }
    [[nodiscard]] bool isReplaceable() const noexcept { return m_isReplaceable; }
    [[nodiscard]] i32 opacity() const noexcept { return m_opacity; }
    [[nodiscard]] bool doesPropagateSkylightDown() const noexcept { return m_propagatesSkylightDown; }
    [[nodiscard]] u8 harvestTool() const noexcept { return m_harvestTool; }
    [[nodiscard]] i32 harvestLevel() const noexcept { return m_harvestLevel; }
    [[nodiscard]] const std::string& lootTableId() const noexcept { return m_lootTableId; }
    [[nodiscard]] const BlockSoundType* soundType() const noexcept { return m_soundType; }
    [[nodiscard]] f32 slipperiness() const noexcept { return m_slipperiness; }
    [[nodiscard]] f32 speedFactor() const noexcept { return m_speedFactor; }
    [[nodiscard]] f32 jumpFactor() const noexcept { return m_jumpFactor; }
    [[nodiscard]] bool ticksRandomly() const noexcept { return m_ticksRandomly; }
    [[nodiscard]] world::map::MaterialColorId mapColor() const noexcept { return m_mapColor; }
    [[nodiscard]] bool hasMapColor() const noexcept { return m_hasMapColor; }
    [[nodiscard]] bool isIgnitedByLava() const noexcept { return m_ignitedByLava; }
    [[nodiscard]] OffsetType getOffsetType() const noexcept { return m_offsetType; }
    [[nodiscard]] Instrument getInstrument() const noexcept { return m_instrument; }

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
    i32 m_opacity = 15; // 默认完全不透明
    bool m_propagatesSkylightDown = false;
    u8 m_harvestTool = HarvestTool::None;
    i32 m_harvestLevel = 0;
    std::string m_lootTableId;
    bool m_noLootTable = false;                                                // 显式禁止自动推导掉落表ID（如空气方块）
    const BlockSoundType* m_soundType = &BlockSoundTypes::STONE;               // 默认使用石头声音
    f32 m_slipperiness = 0.6f;                                                 // MC默认滑度
    f32 m_speedFactor = 1.0f;                                                  // MC默认速度因子
    f32 m_jumpFactor = 1.0f;                                                   // MC默认跳跃因子
    bool m_ticksRandomly = false;                                              // 是否响应随机刻
    world::map::MaterialColorId m_mapColor = world::map::MaterialColorId::AIR; // 地图颜色
    bool m_hasMapColor = false;                                                // 是否显式设置了地图颜色
    bool m_ignitedByLava = false;                                              // 是否可被岩浆点燃
    OffsetType m_offsetType = OffsetType::None;                                // 模型偏移类型
    Instrument m_instrument = Instrument::Harp;                                // 音符盒乐器
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
 *             .create([](const Block& block, auto values, const std::vector<StateHolder<Block,
 * BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) { return
 * std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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
    [[nodiscard]] const ResourceLocation& blockLocation() const noexcept { return m_blockLocation; }

    /**
     * @brief 获取方块ID
     */
    [[nodiscard]] u32 blockId() const noexcept { return m_blockId; }

    /**
     * @brief 获取材质
     */
    [[nodiscard]] const Material& material() const noexcept { return *m_material; }

    /**
     * @brief 获取状态容器
     */
    [[nodiscard]] const StateContainer<Block, BlockState>& stateContainer() const noexcept { return *m_stateContainer; }

    /**
     * @brief 获取默认状态
     */
    [[nodiscard]] const BlockState& defaultState() const noexcept { return *m_defaultState; }

    /**
     * @brief 获取硬度
     */
    [[nodiscard]] f32 hardness() const noexcept { return m_hardness; }

    /**
     * @brief 获取抗性
     */
    [[nodiscard]] f32 resistance() const noexcept { return m_resistance; }

    /**
     * @brief 获取光照等级
     */
    [[nodiscard]] u8 lightLevel() const noexcept { return m_lightLevel; }

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
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getLightValue
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
    [[nodiscard]] i32 opacity() const noexcept { return m_opacity; }

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
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getShadeBrightness
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
     * 参考: net.minecraft.block.Block#getDefaultMaterialColor
     */
    [[nodiscard]] virtual world::map::MaterialColorId getMapColor(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return m_mapColor;
    }

    /**
     * @brief 检查是否传播天空光向下
     */
    [[nodiscard]] bool doesPropagateSkylightDown() const noexcept { return m_propagatesSkylightDown; }

    /**
     * @brief 获取挖掘工具类型
     *
     * 返回采集此方块所需的工具类型。
     * 如果返回 HarvestTool::None，则不需要特定工具。
     *
     * @return 工具类型值
     */
    [[nodiscard]] u8 harvestTool() const noexcept { return m_harvestTool; }

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
    [[nodiscard]] i32 harvestLevel() const noexcept { return m_harvestLevel; }

    /**
     * @brief 是否需要正确工具才能采集
     *
     * 如果返回 true，则必须使用正确类型且等级足够的工具
     * 才能获得方块掉落物。
     *
     * @return 是否需要正确工具
     */
    [[nodiscard]] bool requiresTool() const noexcept { return m_requiresTool; }

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
    [[nodiscard]] bool isReplaceable() const noexcept { return m_isReplaceable; }

    // ========== 声音类型 ==========

    /**
     * @brief 获取方块的声音类型
     *
     * 返回方块的声音类型，包含破坏、踩踏、放置、击打、坠落声音。
     *
     * @return 声音类型的常量引用
     */
    [[nodiscard]] const BlockSoundType& getSoundType() const noexcept { return *m_soundType; }

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
    [[nodiscard]] virtual std::string getLootTableId() const noexcept { return m_lootTableId; }

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
    void setLootTableId(const std::string& id) noexcept { m_lootTableId = id; }

    /**
     * @brief 获取中键选取方块时获得的物品堆
     *
     * 当玩家中键点击方块时，返回应给予玩家的物品堆。
     * 默认实现返回空（由外部系统通过 BlockItemRegistry 查找对应方块物品）。
     * 某些方块（如洞穴藤蔓）的中键选取返回的物品与方块物品不同，
     * 需要覆写此方法返回正确的物品。
     *
     * 参考: net.minecraft.block.Block.getCloneItemStack
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
     * 参考: net.minecraft.block.Block#getBlockSupportShape
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
     * 参考: net.minecraft.block.BlockState#getFaceOcclusionShape
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
     * 参考: net.minecraft.block.BlockBehaviour#getEntityInsideCollisionShape
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
     * 参考: net.minecraft.block.BlockState#useShapeForLightOcclusion
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
    [[nodiscard]] virtual i32 getOpacity(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const;

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
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#tick
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
     * @brief 状态变更时是否保留方块实体
     *
     * 当方块因氧化/涂蜡/除蜡/刮削等原因导致方块 ID 变化但语义上"仍是同一个方块"时，
     * 应返回 true 以保留旧方块实体（物品内容物、自定义名称等不丢失）。
     *
     * 默认实现返回 false：方块 ID 变化时旧方块实体会被销毁，新方块实体会被创建。
     * 铜箱子等容器类铜方块应重写此方法返回 true，避免物品在氧化/涂蜡/除蜡/刮削时丢失。
     *
     * 参考: net.minecraft.world.level.block.state.BlockBehaviour#shouldChangedStateKeepBlockEntity
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
     * 参考: net.minecraft.block.Block#playerWillDestroy
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
     * 参考: net.minecraft.block.BlockBehaviour.spawnAfterBreak
     */
    virtual void spawnAfterBreak(
        IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, bool dropExp) const;

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
     * 参考: net.minecraft.block.Block.getExplosionResistance
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
     * 参考: net.minecraft.block.Block.canDropFromExplosion
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
     * 参考: net.minecraft.block.Block.dropResources(BlockState, LevelAccessor, BlockPos, BlockEntity)
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 被破坏的方块状态
     */
    static void dropResources(IWorld& world, const BlockPos& pos, const BlockState& state);

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
     * 参考: net.minecraft.block.Block.onBlockExploded
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
     * 参考: net.minecraft.block.Block.onEntityCollision
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
     * 参考: net.minecraft.block.Block.onLanded
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
     * 参考: net.minecraft.block.Block.onEntityWalk
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
     * 参考: net.minecraft.block.Block.getFriction
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
     * 参考: net.minecraft.block.Block.getSpeedFactor
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
     * 参考: net.minecraft.block.Block.getJumpFactor
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
     * 参考: net.minecraft.block.Block.isLadder
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
     * 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#ticksRandomly
     *
     * @return 是否响应随机刻
     */
    [[nodiscard]] virtual bool ticksRandomly() const noexcept { return m_ticksRandomly; }

    /**
     * @brief 是否可被岩浆点燃
     *
     * 当返回 true 时，此方块接触岩浆会被点燃。
     * 木头类、TNT等可燃方块应返回 true。
     *
     * 参考: net.minecraft.block.Block.isIgnitedByLava
     */
    [[nodiscard]] bool isIgnitedByLava() const noexcept { return m_ignitedByLava; }

    /**
     * @brief 获取模型偏移类型
     *
     * 控制方块渲染时的随机位置偏移。
     * 植物、花等小方块使用XZ偏移来避免视觉重复。
     *
     * 参考: net.minecraft.block.Block.getOffsetType
     */
    [[nodiscard]] BlockProperties::OffsetType getOffsetType() const noexcept { return m_offsetType; }

    /**
     * @brief 获取音符盒乐器
     *
     * 当音符盒放置在此方块上方时演奏的乐器。
     *
     * 参考: net.minecraft.block.Block.getInstrument
     */
    [[nodiscard]] BlockProperties::Instrument getInstrument() const noexcept { return m_instrument; }

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
     * @param stack 放置该方块所用的物品堆（始终非空，数量尚未扣除）
     */
    virtual void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack);

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
     * 参考: net.minecraft.block.Block#canSustainPlant
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
     * @brief 玩家右键点击
     *
     * 当玩家右键点击方块时调用。
     * 默认实现返回 Pass。
     *
     * 返回 `BlockActionResult` 而非裸 `ActionResultType`，以支持携带
     * `heldItemTransformedTo` 信息（参考 MC 1.21.11
     * `InteractionResult.Success.heldItemTransformedTo(ItemStack)`）。
     *
     * 为了向后兼容，`BlockActionResult` 可从 `ActionResultType` 隐式构造，
     * 因此旧 override 直接 `return ActionResultType::Success;` 仍然有效。
     * 需要传递物品转换信息的方块（如 `ShelfBlock`）应使用
     * `BlockActionResult::success(stack)` 或
     * `BlockActionResult::success().heldItemTransformedTo(stack)`。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果（可能携带转换后的手持物品）
     */
    [[nodiscard]] virtual BlockActionResult onBlockActivated(const BlockState& state,
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
    [[nodiscard]] virtual bool hasBlockEntity() const noexcept { return false; }

    /**
     * @brief 检查是否为游戏管理员方块
     *
     * 游戏管理员方块需要创造模式 + OP等级>=2 权限才能放置、破坏和交互。
     * 包括命令方块、结构方块、拼图方块等。
     *
     * Block 子类需要通过同时继承 GameMasterBlock 标记接口并重写此方法返回 true
     * 来标记自身为管理员方块。
     *
     * 参考: net.minecraft.world.level.block.GameMasterBlock
     *
     * @return 如果此方块为游戏管理员方块返回true
     */
    [[nodiscard]] virtual bool isGameMaster() const noexcept { return false; }

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
     * @param side 信号输出方向
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
     * @param side 信号输出方向
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
     * 参考: net.minecraft.block.Block#isStickyBlock
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
     * 参考: net.minecraft.block.Block#canStickTo
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
     * 参考: net.minecraft.block.FireBlock.getFlammability()
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
     * 参考: net.minecraft.block.FireBlock.getFireSpreadSpeed()
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

    // ========================================================================
    // 静态辅助方法
    // ========================================================================

    /**
     * @brief 检查方块面是否应该被渲染
     *
     * 判断相邻两个方块之间是否需要渲染遮挡面。
     * 用于渲染面剔除优化。
     *
     * 参考: net.minecraft.block.Block#shouldSideBeRendered
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
     * 参考: net.minecraft.block.Block#hasSolidSideOnTop
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
     * 参考: net.minecraft.block.Block#hasEnoughSolidSide
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
     * 参考: net.minecraft.world.level.block.Block#canSupportCenter
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
     * 参考: net.minecraft.world.level.block.Block#canSupportRigidBlock
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
     * 参考: net.minecraft.block.Block#doesSideFillSquare (旧名)
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
     * 参考: net.minecraft.block.Block#isFaceFull
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
     * 参考: net.minecraft.block.Block#isExceptionForConnection
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
     * 参考: net.minecraft.block.Block#pushEntitiesUp
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
     * 累积结果并返回最终状态。用于区块后处理生成、结构放置、活塞移动等场景。
     *
     * @param state 当前方块状态
     * @param world 世界接口
     * @param pos 当前方块位置
     * @return 根据邻居状态更新后的方块状态
     */
    [[nodiscard]] static BlockState updateFromNeighbourShapes(
        const BlockState& state, IWorld& world, const BlockPos& pos);

    // ========================================================================
    // 攻击和交互
    // ========================================================================

    /**
     * @brief 玩家左键攻击方块
     *
     * 当玩家左键点击（攻击）方块时调用。
     * 默认实现为空，需要特殊行为的方块（如 TNT、创造模式等）应重写。
     *
     * 参考: net.minecraft.block.Block#onBlockClicked
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
     * 参考: net.minecraft.block.Block#onProjectileHit
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
     * 参考: net.minecraft.block.Block#fallOn
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
     * 参考: net.minecraft.block.Block#handlePrecipitation
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
     * 参考: net.minecraft.block.Block#harvestBlock
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
     * 参考: net.minecraft.block.Block#getPlayerRelativeBlockHardness
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
     * 参考: net.minecraft.block.Block#getRenderType
     *
     * @param state 方块状态
     * @return 渲染类型
     */
    enum class RenderType : u8 {
        MODEL,               // 正常模型渲染
        INVISIBLE,           // 不可见（空气、屏障等）
        LIQUID,              // 液体渲染（水、岩浆）
        ENTITYBLOCK_ANIMATED // 方块实体动画（箱子、熔炉等）
    };

    [[nodiscard]] virtual RenderType getRenderType(const BlockState& state) const noexcept
    {
        MC_UNUSED(state);
        return RenderType::MODEL;
    }

    /**
     * @brief 检查是否允许移动（路径查找）
     *
     * 用于实体路径查找系统判断是否可以穿过方块。
     * 默认返回材质的 blocksMovement 取反。
     *
     * 参考: net.minecraft.block.Block#allowsMovement
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
     * @brief 转换为字符串
     */
    [[nodiscard]] virtual std::string toString() const noexcept { return m_blockLocation.toString(); }

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
    bool m_isFlammable = false;
    bool m_propagatesSkylightDown = false;
    bool m_requiresTool = false;
    bool m_isReplaceable = false; // 是否可被替换
    bool m_ticksRandomly = false; // 是否响应随机刻
    u8 m_harvestTool = HarvestTool::None;
    i32 m_harvestLevel = 0;

    // 地图颜色（用于在地图上渲染方块颜色）
    world::map::MaterialColorId m_mapColor = world::map::MaterialColorId::AIR;
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
    bool m_ignitedByLava = false;                                                 // 是否可被岩浆点燃
    BlockProperties::OffsetType m_offsetType = BlockProperties::OffsetType::None; // 模型偏移类型
    BlockProperties::Instrument m_instrument = BlockProperties::Instrument::Harp; // 音符盒乐器

    // 由createBlockState设置
    std::unique_ptr<StateContainer<Block, BlockState>> m_stateContainer;
    const BlockState* m_defaultState = nullptr;
};

} // namespace mc
