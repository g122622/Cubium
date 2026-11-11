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
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/BlockOffsetType.hpp"
#include "common/world/block/base/types/HarvestTool.hpp"
#include "common/world/block/base/types/NoteBlockInstrument.hpp"
#include "common/world/block/properties/StatePredicate.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <optional>
#include <string>

namespace mc {
class Block;
class BlockRegistry;

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
     * @brief 强制视为实体方块（原版 `BlockBehaviour.Properties#forceSolidOn`）
     *
     * 覆盖 `isSolid` 的形状推断，直接返回 true。用于「碰撞形状不满整格、但语义上
     * 算实体方块」的情况。
     */
    BlockProperties& forceSolidOn() noexcept
    {
        m_forceSolidOn = true;
        return *this;
    }

    /**
     * @brief 强制不视为实体方块（原版 `BlockBehaviour.Properties#forceSolidOff`）
     *
     * 覆盖 `isSolid` 的形状推断，直接返回 false。优先级低于 `forceSolidOn`。
     */
    BlockProperties& forceSolidOff() noexcept
    {
        m_forceSolidOff = true;
        return *this;
    }

    /**
     * @brief 设置无碰撞（对齐 vanilla `BlockBehaviour.Properties#noCollision`）
     *
     * 原版这个开关同时关掉 `canOcclude`：没有碰撞形状的方块不可能遮挡光线。
     * 早期实现只清了 `m_hasCollision`，于是空气、花草这些无碰撞方块的 `canOcclude`
     * 仍为 true，光照引擎会把它们当作遮挡体。
     */
    BlockProperties& noCollision() noexcept
    {
        m_hasCollision = false;
        m_canOcclude = false;
        return *this;
    }

    /**
     * @brief 恢复碰撞（覆盖由材质推导出的默认值）
     *
     * 材质（`Material`）会给出一个默认的碰撞开关，例如 `Material::PLANT` 经
     * `makePlantMaterial()` 的 `.solid(false)` 推导出「无碰撞」。
     *
     * 但**材质不等于形状**：有些方块借用植物材质只是为了挖掘手感与可燃性，自身却是
     * 完整的实心方块。典型例子是干海带块（`minecraft:dried_kelp_block`）——原版把它注册成
     * 普通 `Block`，是完整方块，玩家能站上去；Cubium 却因为套用了植物材质而没有碰撞箱，
     * 玩家会直接穿过去。由差分能力 `face_sturdy` 查出。
     *
     * 这个开关同时决定 `getCollisionShape` 是否返回空形状（原版
     * `hasCollision ? getShape(...) : Shapes.empty()`），与 `getShape`（选中框、渲染轮廓）
     * 相互独立——按钮、火把、告示牌等都是「外形非空、碰撞为空」。
     */
    BlockProperties& withCollision() noexcept
    {
        m_hasCollision = true;
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
     */
    BlockProperties& opacity(i32 value) noexcept
    {
        m_opacity = value < 0 ? 0 : (value > 15 ? 15 : value);
        return *this;
    }

    /**
     * @brief 设置该方块不参与遮挡计算
     *
     * 对应 vanilla `BlockBehaviour.Properties#noOcclusion`。关闭后：
     * - 遮挡形状视为空（不做任何面遮挡检测）；
     * - 不再"实体渲染"（isSolidRender 恒为 false），`getOpacity` 不会返回 15；
     * - `getShadeBrightness` 恒为 1.0（无 AO 阴影）。
     *
     * vanilla 的 canOcclude 是 `getLightBlock`/`isSolidRender`/面遮挡检测的总开关，与材质无关：
     * 铁栏杆（金属材质）、潜影盒（木质材质）、树叶、门、活板门等在原版均注册为 noOcclusion。
     */
    /**
     * @brief 覆写「是否造成窒息」（原版 `BlockBehaviour.Properties#isSuffocating`）
     *
     * 默认判定为 `blocksMotion() && isCollisionShapeFullBlock()`，见
     * `Block::isSuffocatingAt`。原版只有少数方块替换它：树叶、玻璃、染色玻璃、铜格栅、
     * 甘蔗丛/红树根（恒 false），耕地/泥土路径（恒 true），活塞（未伸出时 true），
     * 潜影盒（未打开时 true）。
     *
     * @param predicate 谓词实例；必须指向静态存储期对象，注册后不再变化
     */
    BlockProperties& suffocating(const StatePredicate& predicate) noexcept
    {
        m_isSuffocating = &predicate;
        return *this;
    }

    /**
     * @brief 覆写「是否挡住视线」（原版 `BlockBehaviour.Properties#isViewBlocking`）
     *
     * 未覆写时与 `isSuffocating` 共用同一个谓词实例——原版正是 `isViewBlocking = isSuffocating`，
     * 因此只替换窒息判定的方块（甘蔗丛、红树根等）会连带改变视线判定。
     */
    BlockProperties& viewBlocking(const StatePredicate& predicate) noexcept
    {
        m_isViewBlocking = &predicate;
        return *this;
    }

    BlockProperties& noOcclusion() noexcept
    {
        m_canOcclude = false;
        return *this;
    }

    /**
     * @brief 设置是否传播天空光向下
     *
     * 某些方块（如树叶、冰、水）会使天空光衰减1级后传播，
     * 而不是完全阻挡或无衰减传播。
     *
     */
    BlockProperties& propagatesSkylightDown(bool value = true) noexcept
    {
        m_propagatesSkylightDown = value;
        m_propagatesSkylightDownExplicit = true;
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
    [[nodiscard]] bool noLootTable() const noexcept
    {
        return m_noLootTable;
    }

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
     */
    BlockProperties& offsetType(BlockOffsetType type) noexcept
    {
        m_offsetType = type;
        return *this;
    }

    /**
     * @brief 设置音符盒乐器
     *
     * 当音符盒放置在此方块上方时演奏的乐器。
     *
     */
    BlockProperties& instrument(NoteBlockInstrument instr) noexcept
    {
        m_instrument = instr;
        return *this;
    }

    // Getters
    [[nodiscard]] const Material& material() const noexcept
    {
        return *m_material;
    }

    [[nodiscard]] f32 hardness() const noexcept
    {
        return m_hardness;
    }

    [[nodiscard]] f32 resistance() const noexcept
    {
        return m_resistance;
    }

    [[nodiscard]] u8 lightLevel() const noexcept
    {
        return m_lightLevel;
    }

    [[nodiscard]] bool hasCollision() const noexcept
    {
        return m_hasCollision;
    }

    [[nodiscard]] bool isSolid() const noexcept
    {
        return m_isSolid;
    }

    [[nodiscard]] bool isFlammable() const noexcept
    {
        return m_isFlammable;
    }

    [[nodiscard]] bool requiresTool() const noexcept
    {
        return m_requiresTool;
    }

    [[nodiscard]] bool isReplaceable() const noexcept
    {
        return m_isReplaceable;
    }

    [[nodiscard]] i32 opacity() const noexcept
    {
        return m_opacity;
    }

    [[nodiscard]] bool canOcclude() const noexcept
    {
        return m_canOcclude;
    }

    [[nodiscard]] bool doesPropagateSkylightDown() const noexcept
    {
        return m_propagatesSkylightDown;
    }

    [[nodiscard]] u8 harvestTool() const noexcept
    {
        return m_harvestTool;
    }

    [[nodiscard]] i32 harvestLevel() const noexcept
    {
        return m_harvestLevel;
    }

    [[nodiscard]] const std::string& lootTableId() const noexcept
    {
        return m_lootTableId;
    }

    [[nodiscard]] const BlockSoundType* soundType() const noexcept
    {
        return m_soundType;
    }

    [[nodiscard]] f32 slipperiness() const noexcept
    {
        return m_slipperiness;
    }

    [[nodiscard]] f32 speedFactor() const noexcept
    {
        return m_speedFactor;
    }

    [[nodiscard]] f32 jumpFactor() const noexcept
    {
        return m_jumpFactor;
    }

    // ticksRandomly() 只在下面（虚函数区）定义一次。早期这里还有一个同名的非虚重载，
    // 它把虚函数「隐藏」了：`block->ticksRandomly()` 在静态类型是 Block* 时直接绑定到
    // 非虚版本，只返回注册时写死的 m_ticksRandomly 标志。于是所有按状态动态决定随机刻
    // 的子类（铜箱子/铜雕像按氧化等级、花盆按内容物等）——以及通过 ticksRandomly()
    // 重写而非 BlockProperties::tickRandomly() 注册的方块（蜡烛、营火、浮冰等）——
    // 在随机刻调度与区块序列化里全部失效。

    [[nodiscard]] world::map::MaterialColorId mapColor() const noexcept
    {
        return m_mapColor;
    }

    [[nodiscard]] bool hasMapColor() const noexcept
    {
        return m_hasMapColor;
    }

    [[nodiscard]] bool isIgnitedByLava() const noexcept
    {
        return m_ignitedByLava;
    }

    [[nodiscard]] BlockOffsetType getOffsetType() const noexcept
    {
        return m_offsetType;
    }

    [[nodiscard]] NoteBlockInstrument getInstrument() const noexcept
    {
        return m_instrument.value_or(NoteBlockInstrument::Harp);
    }

private:
    friend class Block;
    friend class BlockRegistry;

    const Material* m_material;
    f32 m_hardness;
    f32 m_resistance;
    u8 m_lightLevel;
    bool m_hasCollision;
    bool m_isSolid;
    bool m_forceSolidOn = false;  ///< 强制视为实体方块（覆盖 `isSolid` 的形状推断）
    bool m_forceSolidOff = false; ///< 强制不视为实体方块（覆盖 `isSolid` 的形状推断）
    bool m_isFlammable;
    bool m_requiresTool;
    bool m_isReplaceable;

private:
    i32 m_opacity = 15;       // 默认完全不透明
    bool m_canOcclude = true; // 是否参与遮挡计算（vanilla canOcclude，noOcclusion() 关闭）
    /// 窒息谓词；为空表示使用默认判定（blocksMotion && 碰撞形状满格）
    const StatePredicate* m_isSuffocating = nullptr;
    /// 视线谓词；为空且窒息谓词也为空时同样走默认判定
    const StatePredicate* m_isViewBlocking = nullptr;
    bool m_propagatesSkylightDown = false;
    bool m_propagatesSkylightDownExplicit = false; // 是否显式设置过 propagatesSkylightDown（区分显式值与默认哨兵）
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
    BlockOffsetType m_offsetType = BlockOffsetType::None;                      // 模型偏移类型
    std::optional<NoteBlockInstrument> m_instrument;                           // 音符盒乐器（未设置时按材质推导）
};
} // namespace mc
