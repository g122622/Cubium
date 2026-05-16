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

#include "../ServerEventBus.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

// 前向声明
namespace mc {
class Player;
class BlockState;
class DamageSource;
class PlayerInventory;
} // namespace mc

namespace mc::entity::effect {
class EffectInstance;
} // namespace mc::entity::effect

namespace mc::server::event {

// ============================================================================
// 方块事件
// ============================================================================

/**
 * @brief 方块破坏事件
 */
struct BlockBreakEvent : ServerEvent {
    PlayerId playerId;       ///< 破坏者ID
    BlockPos pos;            ///< 方块位置
    const BlockState* state; ///< 方块状态
    const ItemStack* tool;   ///< 使用的工具（可能为null）

    BlockBreakEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s, const ItemStack* t)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
        , tool(t)
    {}
};

/**
 * @brief 方块放置事件
 *
 * 当玩家成功放置方块时触发。
 * 参考 MC 1.16.5: CriteriaTriggers.PLACED_BLOCK
 */
struct BlockPlaceEvent : ServerEvent {
    PlayerId playerId;       ///< 放置者ID（可能为0表示非玩家放置）
    BlockPos pos;            ///< 方块位置
    const BlockState* state; ///< 放置的方块状态
    const ItemStack* item;   ///< 使用的物品（可能为null）

    BlockPlaceEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s, const ItemStack* i = nullptr)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
        , item(i)
    {}
};

/**
 * @brief 方块交互事件
 */
struct BlockInteractEvent : ServerEvent {
    PlayerId playerId;       ///< 交互者ID
    BlockPos pos;            ///< 方块位置
    const BlockState* state; ///< 方块状态
    i32 interactionType;     ///< 交互类型（0=右键，1=左键）

    BlockInteractEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s, i32 type)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
        , interactionType(type)
    {}
};

// ============================================================================
// 实体事件
// ============================================================================

/**
 * @brief 实体死亡事件
 */
struct EntityDeathEvent : ServerEvent {
    Entity* entity;            ///< 死亡的实体
    Entity* killer;            ///< 击杀者（可能为null）
    const DamageSource* cause; ///< 死亡原因

    EntityDeathEvent(u64 tick, Entity* e, Entity* k, const DamageSource* c)
        : ServerEvent(tick)
        , entity(e)
        , killer(k)
        , cause(c)
    {}
};

/**
 * @brief 玩家击杀实体事件
 */
struct PlayerKillEntityEvent : ServerEvent {
    PlayerId playerId;         ///< 击杀者ID
    Entity* victim;            ///< 被击杀的实体
    const DamageSource* cause; ///< 死亡原因

    PlayerKillEntityEvent(u64 tick, PlayerId pid, Entity* v, const DamageSource* c)
        : ServerEvent(tick)
        , playerId(pid)
        , victim(v)
        , cause(c)
    {}
};

/**
 * @brief 玩家受伤事件
 */
struct PlayerHurtEvent : ServerEvent {
    PlayerId playerId;         ///< 受伤玩家ID
    Entity* attacker;          ///< 攻击者（可能为null）
    f32 damage;                ///< 伤害值
    const DamageSource* cause; ///< 伤害来源

    PlayerHurtEvent(u64 tick, PlayerId pid, Entity* a, f32 d, const DamageSource* c)
        : ServerEvent(tick)
        , playerId(pid)
        , attacker(a)
        , damage(d)
        , cause(c)
    {}
};

/**
 * @brief 实体受伤事件
 */
struct EntityHurtEvent : ServerEvent {
    Entity* entity;            ///< 受伤实体
    Entity* attacker;          ///< 攻击者（可能为null）
    f32 damage;                ///< 伤害值
    const DamageSource* cause; ///< 伤害来源

    EntityHurtEvent(u64 tick, Entity* e, Entity* a, f32 d, const DamageSource* c)
        : ServerEvent(tick)
        , entity(e)
        , attacker(a)
        , damage(d)
        , cause(c)
    {}
};

/**
 * @brief 玩家与实体交互事件
 */
struct PlayerEntityInteractEvent : ServerEvent {
    PlayerId playerId;   ///< 玩家ID
    Entity* entity;      ///< 交互的实体
    i32 interactionType; ///< 交互类型（0=交互，1=攻击）

    PlayerEntityInteractEvent(u64 tick, PlayerId pid, Entity* e, i32 type)
        : ServerEvent(tick)
        , playerId(pid)
        , entity(e)
        , interactionType(type)
    {}
};

// ============================================================================
// 玩家事件
// ============================================================================

/**
 * @brief 玩家登录事件
 */
struct PlayerLoginEvent : ServerEvent {
    PlayerId playerId;    ///< 玩家ID
    std::string username; ///< 用户名

    PlayerLoginEvent(u64 tick, PlayerId pid, const std::string& name)
        : ServerEvent(tick)
        , playerId(pid)
        , username(name)
    {}
};

/**
 * @brief 玩家登出事件
 */
struct PlayerLogoutEvent : ServerEvent {
    PlayerId playerId;  ///< 玩家ID
    std::string reason; ///< 登出原因

    PlayerLogoutEvent(u64 tick, PlayerId pid, const std::string& r)
        : ServerEvent(tick)
        , playerId(pid)
        , reason(r)
    {}
};

/**
 * @brief 玩家重生事件
 */
struct PlayerRespawnEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    GlobalPos pos;     ///< 重生位置

    PlayerRespawnEvent(u64 tick, PlayerId pid, const GlobalPos& p)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
    {}
};

/**
 * @brief 玩家睡眠事件
 */
struct PlayerSleepEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    BlockPos bedPos;   ///< 床的位置

    PlayerSleepEvent(u64 tick, PlayerId pid, const BlockPos& pos)
        : ServerEvent(tick)
        , playerId(pid)
        , bedPos(pos)
    {}
};

/**
 * @brief 玩家起床事件
 */
struct PlayerWakeUpEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    BlockPos bedPos;   ///< 床的位置

    PlayerWakeUpEvent(u64 tick, PlayerId pid, const BlockPos& pos)
        : ServerEvent(tick)
        , playerId(pid)
        , bedPos(pos)
    {}
};

/**
 * @brief 玩家位置事件（用于位置触发器）
 */
struct PlayerLocationEvent : ServerEvent {
    PlayerId playerId;     ///< 玩家ID
    Vector3d position;     ///< 位置
    const Biome* biome;    ///< 生物群系（可能为null）
    DimensionId dimension; ///< 维度

    PlayerLocationEvent(u64 tick, PlayerId pid, const Vector3d& pos, const Biome* b, DimensionId dim)
        : ServerEvent(tick)
        , playerId(pid)
        , position(pos)
        , biome(b)
        , dimension(dim)
    {}
};

/**
 * @brief 维度变化事件
 */
struct DimensionChangeEvent : ServerEvent {
    PlayerId playerId;         ///< 玩家ID
    DimensionId fromDimension; ///< 原维度
    DimensionId toDimension;   ///< 目标维度
    Vector3d position;         ///< 目标位置

    DimensionChangeEvent(u64 tick, PlayerId pid, DimensionId from, DimensionId to, const Vector3d& pos)
        : ServerEvent(tick)
        , playerId(pid)
        , fromDimension(from)
        , toDimension(to)
        , position(pos)
    {}
};

// ============================================================================
// 物品事件
// ============================================================================

/**
 * @brief 物品栏变化事件
 */
struct InventoryChangedEvent : ServerEvent {
    PlayerId playerId;                ///< 玩家ID
    const PlayerInventory* inventory; ///< 物品栏
    i32 slot;                         ///< 变化的槽位
    const ItemStack* oldItem;         ///< 原物品（可能为null）
    const ItemStack* newItem;         ///< 新物品（可能为null）

    InventoryChangedEvent(
        u64 tick, PlayerId pid, const PlayerInventory* inv, i32 s, const ItemStack* old, const ItemStack* newItem)
        : ServerEvent(tick)
        , playerId(pid)
        , inventory(inv)
        , slot(s)
        , oldItem(old)
        , newItem(newItem)
    {}
};

/**
 * @brief 物品拾取事件
 */
struct ItemPickupEvent : ServerEvent {
    PlayerId playerId;  ///< 玩家ID
    ItemStack item;     ///< 拾取的物品
    Entity* itemEntity; ///< 物品实体（可能为null）

    ItemPickupEvent(u64 tick, PlayerId pid, const ItemStack& i, Entity* e)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
        , itemEntity(e)
    {}
};

/**
 * @brief 物品丢弃事件
 */
struct ItemDropEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack item;    ///< 丢弃的物品

    ItemDropEvent(u64 tick, PlayerId pid, const ItemStack& i)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
    {}
};

/**
 * @brief 物品使用事件
 */
struct ItemUseEvent : ServerEvent {
    PlayerId playerId;                    ///< 玩家ID
    ItemStack item;                       ///< 使用的物品
    std::optional<BlockPos> targetBlock;  ///< 目标方块（可能为null）
    std::optional<EntityId> targetEntity; ///< 目标实体（可能为null）

    ItemUseEvent(u64 tick,
        PlayerId pid,
        const ItemStack& i,
        const std::optional<BlockPos>& block,
        const std::optional<EntityId>& entity)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
        , targetBlock(block)
        , targetEntity(entity)
    {}
};

/**
 * @brief 物品消耗事件（进食等）
 */
struct ConsumeItemEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack item;    ///< 消耗的物品

    ConsumeItemEvent(u64 tick, PlayerId pid, const ItemStack& i)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
    {}
};

/**
 * @brief 物品耐久变化事件
 */
struct ItemDurabilityEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack item;    ///< 物品
    i32 oldDurability; ///< 旧耐久
    i32 newDurability; ///< 新耐久

    ItemDurabilityEvent(u64 tick, PlayerId pid, const ItemStack& i, i32 oldD, i32 newD)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
        , oldDurability(oldD)
        , newDurability(newD)
    {}
};

/**
 * @brief 物品销毁事件
 *
 * 当物品因使用而损坏或消耗完毕时触发。
 * 参考 MC 1.16.5: Forge PlayerDestroyItemEvent
 * 触发场景：
 * - 武器攻击损坏
 * - 工具使用损坏
 * - 物品交互消耗（如给动物喂食后物品用尽）
 * - 盾牌格挡损坏
 */
struct PlayerDestroyItemEvent : ServerEvent {
    PlayerId playerId;    ///< 玩家ID
    ItemStack item;       ///< 销毁前的物品副本
    i32 slot;             ///< 物品所在槽位（主手=0，副手=40，其他为物品栏槽位，-1表示未知）
    Hand hand;            ///< 使用的手（MainHand 或 OffHand）

    PlayerDestroyItemEvent(u64 tick, PlayerId pid, const ItemStack& i, i32 s, Hand h)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
        , slot(s)
        , hand(h)
    {}
};

/**
 * @brief 附魔事件
 */
struct EnchantItemEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack item;    ///< 附魔的物品
    i32 levels;        ///< 消耗的等级
    i32 cost;          ///< 附魔费用

    EnchantItemEvent(u64 tick, PlayerId pid, const ItemStack& i, i32 lvl, i32 c)
        : ServerEvent(tick)
        , playerId(pid)
        , item(i)
        , levels(lvl)
        , cost(c)
    {}
};

// ============================================================================
// 效果事件
// ============================================================================

/**
 * @brief 效果变化事件
 */
struct EffectChangedEvent : ServerEvent {
    PlayerId playerId;                              ///< 玩家ID
    const entity::effect::EffectInstance* effect; ///< 效果实例
    bool added;                                     ///< true=添加效果，false=移除效果

    EffectChangedEvent(u64 tick, PlayerId pid, const entity::effect::EffectInstance* e, bool a)
        : ServerEvent(tick)
        , playerId(pid)
        , effect(e)
        , added(a)
    {}
};

/**
 * @brief 酿造完成事件
 */
struct BrewedPotionEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack potion;  ///< 酿造的药水

    BrewedPotionEvent(u64 tick, PlayerId pid, const ItemStack& p)
        : ServerEvent(tick)
        , playerId(pid)
        , potion(p)
    {}
};

// ============================================================================
// 生物事件
// ============================================================================

/**
 * @brief 动物繁殖事件
 */
struct BredAnimalsEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    Entity* child;     ///< 子代
    Entity* parent1;   ///< 父母1
    Entity* parent2;   ///< 父母2

    BredAnimalsEvent(u64 tick, PlayerId pid, Entity* c, Entity* p1, Entity* p2)
        : ServerEvent(tick)
        , playerId(pid)
        , child(c)
        , parent1(p1)
        , parent2(p2)
    {}
};

/**
 * @brief 动物驯服事件
 */
struct TameAnimalEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    Entity* animal;    ///< 驯服的动物

    TameAnimalEvent(u64 tick, PlayerId pid, Entity* a)
        : ServerEvent(tick)
        , playerId(pid)
        , animal(a)
    {}
};

/**
 * @brief 召唤实体事件
 */
struct SummonedEntityEvent : ServerEvent {
    PlayerId playerId; ///< 召唤者ID（可能为null）
    Entity* entity;    ///< 召唤的实体

    SummonedEntityEvent(u64 tick, PlayerId pid, Entity* e)
        : ServerEvent(tick)
        , playerId(pid)
        , entity(e)
    {}
};

// ============================================================================
// 村民事件
// ============================================================================

/**
 * @brief 村民交易事件
 */
struct VillagerTradeEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    Entity* villager;  ///< 村民
    ItemStack bought;  ///< 购买的物品
    ItemStack sold;    ///< 出售的物品

    VillagerTradeEvent(u64 tick, PlayerId pid, Entity* v, const ItemStack& b, const ItemStack& s)
        : ServerEvent(tick)
        , playerId(pid)
        , villager(v)
        , bought(b)
        , sold(s)
    {}
};

/**
 * @brief 治愈僵尸村民事件
 *
 * 当玩家治愈僵尸村民时触发。
 * 参考 MC 1.16.5: CriteriaTriggers.CURED_ZOMBIE_VILLAGER
 */
struct CuredZombieVillagerEvent : ServerEvent {
    std::string starterUuid; ///< 治愈发起者玩家UUID（可能为空）
    Entity* zombie;          ///< 治愈前的僵尸村民实体
    Entity* villager;        ///< 治愈后的村民实体

    CuredZombieVillagerEvent(u64 tick, const std::string& uuid, Entity* z, Entity* v)
        : ServerEvent(tick)
        , starterUuid(uuid)
        , zombie(z)
        , villager(v)
    {}
};

// ============================================================================
// 结构建事件
// ============================================================================

/**
 * @brief 信标构建事件
 */
struct ConstructBeaconEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    BlockPos pos;      ///< 信标位置
    i32 level;         ///< 信标等级

    ConstructBeaconEvent(u64 tick, PlayerId pid, const BlockPos& p, i32 lvl)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , level(lvl)
    {}
};

// ============================================================================
// 其他事件
// ============================================================================

/**
 * @brief 信标效果事件
 */
struct BeaconEffectEvent : ServerEvent {
    PlayerId playerId;  ///< 玩家ID
    BlockPos beaconPos; ///< 信标位置
    i32 effectId;       ///< 效果ID
    i32 duration;       ///< 持续时间（tick）

    BeaconEffectEvent(u64 tick, PlayerId pid, const BlockPos& pos, i32 effect, i32 dur)
        : ServerEvent(tick)
        , playerId(pid)
        , beaconPos(pos)
        , effectId(effect)
        , duration(dur)
    {}
};

/**
 * @brief 不死图腾使用事件
 */
struct UsedTotemEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack totem;   ///< 图腾物品

    UsedTotemEvent(u64 tick, PlayerId pid, const ItemStack& t)
        : ServerEvent(tick)
        , playerId(pid)
        , totem(t)
    {}
};

/**
 * @brief 漂浮事件
 */
struct LevitationEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    f32 duration;      ///< 漂浮时间（秒）
    f32 distance;      ///< 漂浮距离

    LevitationEvent(u64 tick, PlayerId pid, f32 dur, f32 dist)
        : ServerEvent(tick)
        , playerId(pid)
        , duration(dur)
        , distance(dist)
    {}
};

/**
 * @brief 下界旅行距离事件
 */
struct NetherTravelEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    f32 distance;      ///< 旅行距离

    NetherTravelEvent(u64 tick, PlayerId pid, f32 dist)
        : ServerEvent(tick)
        , playerId(pid)
        , distance(dist)
    {}
};

/**
 * @brief 钓鱼竿钩住事件
 */
struct FishingRodHookedEvent : ServerEvent {
    PlayerId playerId;    ///< 玩家ID
    Entity* hooked;       ///< 钩住的实体（可能为null）
    ItemStack fishingRod; ///< 钓鱼竿

    FishingRodHookedEvent(u64 tick, PlayerId pid, Entity* h, const ItemStack& rod)
        : ServerEvent(tick)
        , playerId(pid)
        , hooked(h)
        , fishingRod(rod)
    {}
};

/**
 * @brief 弩射击事件
 */
struct ShotCrossbowEvent : ServerEvent {
    PlayerId playerId;  ///< 玩家ID
    ItemStack crossbow; ///< 弩

    ShotCrossbowEvent(u64 tick, PlayerId pid, const ItemStack& c)
        : ServerEvent(tick)
        , playerId(pid)
        , crossbow(c)
    {}
};

/**
 * @brief 标靶命中事件
 */
struct TargetHitEvent : ServerEvent {
    PlayerId playerId;  ///< 玩家ID
    BlockPos pos;       ///< 标靶位置
    i32 signalStrength; ///< 信号强度（1-15）
    Vector3d hitPos;    ///< 命中位置

    TargetHitEvent(u64 tick, PlayerId pid, const BlockPos& p, i32 signal, const Vector3d& hit)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , signalStrength(signal)
        , hitPos(hit)
    {}
};

/**
 * @brief 蜂巢破坏事件
 */
struct BeeNestDestroyedEvent : ServerEvent {
    PlayerId playerId;       ///< 玩家ID
    BlockPos pos;            ///< 蜂巢位置
    const BlockState* state; ///< 蜂巢方块状态
    ItemStack tool;          ///< 使用的工具

    BeeNestDestroyedEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s, const ItemStack& t)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
        , tool(t)
    {}
};

/**
 * @brief 滑块滑落事件
 */
struct SlideDownBlockEvent : ServerEvent {
    PlayerId playerId;       ///< 玩家ID
    BlockPos pos;            ///< 方块位置
    const BlockState* state; ///< 方块状态

    SlideDownBlockEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
    {}
};

/**
 * @brief 配方解锁事件
 */
struct RecipeUnlockedEvent : ServerEvent {
    PlayerId playerId;         ///< 玩家ID
    ResourceLocation recipeId; ///< 配方ID

    RecipeUnlockedEvent(u64 tick, PlayerId pid, const ResourceLocation& id)
        : ServerEvent(tick)
        , playerId(pid)
        , recipeId(id)
    {}
};

/**
 * @brief 填充桶事件
 */
struct FilledBucketEvent : ServerEvent {
    PlayerId playerId; ///< 玩家ID
    ItemStack bucket;  ///< 桶物品

    FilledBucketEvent(u64 tick, PlayerId pid, const ItemStack& b)
        : ServerEvent(tick)
        , playerId(pid)
        , bucket(b)
    {}
};

/**
 * @brief 进入方块事件
 */
struct EnterBlockEvent : ServerEvent {
    PlayerId playerId;       ///< 玩家ID
    BlockPos pos;            ///< 方块位置
    const BlockState* state; ///< 方块状态

    EnterBlockEvent(u64 tick, PlayerId pid, const BlockPos& p, const BlockState* s)
        : ServerEvent(tick)
        , playerId(pid)
        , pos(p)
        , state(s)
    {}
};

/**
 * @brief 引雷附魔触发事件
 *
 * 当玩家使用引雷附魔的三叉戟召唤闪电击中实体时触发。
 * 参考 MC 1.16.5: CriteriaTriggers.CHANNELED_LIGHTNING
 */
struct ChanneledLightningEvent : ServerEvent {
    PlayerId casterId;              ///< 施法者ID（引雷附魔的玩家）
    std::vector<Entity*> victims;   ///< 被闪电击中的实体列表

    ChanneledLightningEvent(u64 tick, PlayerId caster, std::vector<Entity*> v)
        : ServerEvent(tick)
        , casterId(caster)
        , victims(std::move(v))
    {}
};

} // namespace mc::server::event
