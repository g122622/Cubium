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

#include "common/advancement/AdvancementManager.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/BlockTriggers.hpp"
#include "common/advancement/trigger/impl/ChanneledLightningTrigger.hpp"
#include "common/advancement/trigger/impl/EffectTriggers.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/advancement/trigger/impl/ItemTriggers.hpp"
#include "common/advancement/trigger/impl/LocationTrigger.hpp"
#include "common/advancement/trigger/impl/PlayerKilledEntityTrigger.hpp"
#include "common/advancement/trigger/impl/TickTrigger.hpp"
#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include "common/world/village/VillageManager.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <functional>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server::advancement {

/**
 * @brief 成就事件处理器
 *
 * 订阅服务端事件并触发相应的成就触发器。
 *
 * ## 架构说明
 *
 * AdvancementEventHandler 需要从 PlayerId 获取 ServerPlayer 以触发成就检测。
 * 这通过以下路径实现：
 *
 * 1. IServer::playerEntityManager() → ServerPlayerEntityManager
 * 2. IServer::getPlayerWorld(playerId) → ServerWorld*
 * 3. ServerPlayerEntityManager::getPlayerEntity(playerId, world) → Player*
 * 4. Player::asServerPlayer() → ServerPlayer*
 *
 * 注意：不使用 PlayerManager::getPlayer() 获取 ServerPlayerData，
 * 因为 ServerPlayerData 只存储网络会话数据，不持有 ServerPlayer 引用。
 */
class AdvancementEventHandler {
public:
    /**
     * @brief 设置服务器接口
     * @param server 服务器接口指针
     *
     * 必须在 initialize() 之前调用。
     * IServer 提供访问 ServerPlayerEntityManager 和 ServerWorld 的能力。
     */
    void setServer(IServer* server) { m_server = server; }

    /**
     * @brief 设置玩家管理器
     * @param playerManager 玩家管理器指针
     *
     * 用于通过 UUID 查找玩家数据。CuredZombieVillagerEvent 携带 UUID 而非 PlayerId，
     * 需要通过 PlayerManager::findByUuid() 转换为 PlayerId。
     */
    void setPlayerManager(mc::server::core::PlayerManager* playerManager) { m_playerManager = playerManager; }

    /**
     * @brief 初始化事件处理器
     *
     * 订阅所有相关的事件。
     */
    void initialize()
    {
        // 订阅物品栏变化事件
        m_inventoryChangedSubscription =
            event::ServerEventBus::instance().makeSubscription<event::InventoryChangedEvent>(
                [this](const event::InventoryChangedEvent& e) { _onInventoryChanged(e); });

        // 订阅玩家击杀实体事件
        m_playerKillSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerKillEntityEvent>(
            [this](const event::PlayerKillEntityEvent& e) { _onPlayerKillEntity(e); });

        // 订阅玩家登录事件（初始化成就监听器）
        m_playerLoginSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerLoginEvent>(
            [this](const event::PlayerLoginEvent& e) { _onPlayerLogin(e); });

        // 订阅方块放置事件
        m_blockPlaceSubscription = event::ServerEventBus::instance().makeSubscription<event::BlockPlaceEvent>(
            [this](const event::BlockPlaceEvent& e) { _onBlockPlaced(e); });

        // 订阅僵尸村民治愈事件
        m_curedZombieVillagerSubscription =
            event::ServerEventBus::instance().makeSubscription<event::CuredZombieVillagerEvent>(
                [this](const event::CuredZombieVillagerEvent& e) { _onCuredZombieVillager(e); });

        // 订阅玩家睡眠事件
        m_playerSleepSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerSleepEvent>(
            [this](const event::PlayerSleepEvent& e) { _onPlayerSleep(e); });

        // 订阅效果变化事件
        m_effectChangedSubscription = event::ServerEventBus::instance().makeSubscription<event::EffectChangedEvent>(
            [this](const event::EffectChangedEvent& e) { _onEffectChanged(e); });

        // 订阅玩家位置事件
        m_playerLocationSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerLocationEvent>(
            [this](const event::PlayerLocationEvent& e) { _onPlayerLocation(e); });

        // 订阅维度变化事件
        m_dimensionChangeSubscription = event::ServerEventBus::instance().makeSubscription<event::DimensionChangeEvent>(
            [this](const event::DimensionChangeEvent& e) { _onDimensionChange(e); });

        // 订阅引雷附魔触发事件
        m_channeledLightningSubscription =
            event::ServerEventBus::instance().makeSubscription<event::ChanneledLightningEvent>(
                [this](const event::ChanneledLightningEvent& e) { _onChanneledLightning(e); });

        // 订阅消耗物品事件
        m_consumeItemSubscription = event::ServerEventBus::instance().makeSubscription<event::ConsumeItemEvent>(
            [this](const event::ConsumeItemEvent& e) { _onConsumeItem(e); });

        // 订阅物品耐久变化事件
        m_itemDurabilitySubscription = event::ServerEventBus::instance().makeSubscription<event::ItemDurabilityEvent>(
            [this](const event::ItemDurabilityEvent& e) { _onItemDurability(e); });

        // 订阅附魔事件
        m_enchantItemSubscription = event::ServerEventBus::instance().makeSubscription<event::EnchantItemEvent>(
            [this](const event::EnchantItemEvent& e) { _onEnchantItem(e); });

        // 订阅填充桶事件
        m_filledBucketSubscription = event::ServerEventBus::instance().makeSubscription<event::FilledBucketEvent>(
            [this](const event::FilledBucketEvent& e) { _onFilledBucket(e); });

        // 订阅进入方块事件
        m_enterBlockSubscription = event::ServerEventBus::instance().makeSubscription<event::EnterBlockEvent>(
            [this](const event::EnterBlockEvent& e) { _onEnterBlock(e); });

        // 订阅滑落方块事件
        m_slideDownBlockSubscription = event::ServerEventBus::instance().makeSubscription<event::SlideDownBlockEvent>(
            [this](const event::SlideDownBlockEvent& e) { _onSlideDownBlock(e); });

        // 订阅蜂巢破坏事件
        m_beeNestDestroyedSubscription =
            event::ServerEventBus::instance().makeSubscription<event::BeeNestDestroyedEvent>(
                [this](const event::BeeNestDestroyedEvent& e) { _onBeeNestDestroyed(e); });

        // 订阅动物繁殖事件
        m_bredAnimalsSubscription = event::ServerEventBus::instance().makeSubscription<event::BredAnimalsEvent>(
            [this](const event::BredAnimalsEvent& e) { _onBredAnimals(e); });

        // 订阅村民交易事件
        m_villagerTradeSubscription = event::ServerEventBus::instance().makeSubscription<event::VillagerTradeEvent>(
            [this](const event::VillagerTradeEvent& e) { _onVillagerTrade(e); });

        // 订阅动物驯服事件
        m_tameAnimalSubscription = event::ServerEventBus::instance().makeSubscription<event::TameAnimalEvent>(
            [this](const event::TameAnimalEvent& e) { _onTameAnimal(e); });

        // 订阅实体召唤事件
        m_summonedEntitySubscription = event::ServerEventBus::instance().makeSubscription<event::SummonedEntityEvent>(
            [this](const event::SummonedEntityEvent& e) { _onSummonedEntity(e); });

        // 订阅服务端Tick事件（触发 TickTrigger）
        m_serverTickSubscription = event::ServerEventBus::instance().makeSubscription<event::ServerTickEvent>(
            [this](const event::ServerTickEvent& e) { _onServerTick(e); });

        m_initialized = true;
    }

    /**
     * @brief 关闭事件处理器
     *
     * 取消所有事件订阅。
     */
    void shutdown()
    {
        m_inventoryChangedSubscription.unsubscribe();
        m_playerKillSubscription.unsubscribe();
        m_playerLoginSubscription.unsubscribe();
        m_blockPlaceSubscription.unsubscribe();
        m_curedZombieVillagerSubscription.unsubscribe();
        m_playerSleepSubscription.unsubscribe();
        m_effectChangedSubscription.unsubscribe();
        m_playerLocationSubscription.unsubscribe();
        m_dimensionChangeSubscription.unsubscribe();
        m_channeledLightningSubscription.unsubscribe();
        m_consumeItemSubscription.unsubscribe();
        m_itemDurabilitySubscription.unsubscribe();
        m_enchantItemSubscription.unsubscribe();
        m_filledBucketSubscription.unsubscribe();
        m_enterBlockSubscription.unsubscribe();
        m_slideDownBlockSubscription.unsubscribe();
        m_beeNestDestroyedSubscription.unsubscribe();
        m_bredAnimalsSubscription.unsubscribe();
        m_villagerTradeSubscription.unsubscribe();
        m_tameAnimalSubscription.unsubscribe();
        m_summonedEntitySubscription.unsubscribe();
        m_serverTickSubscription.unsubscribe();
        m_initialized = false;
    }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    /**
     * @brief 处理物品栏变化事件
     *
     * 触发 InventoryChangedTrigger。
     */
    void _onInventoryChanged(const event::InventoryChangedEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::InventoryChangedTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 需要从 playerId 获取 ServerPlayer
        // 这需要 ServerWorld 或 PlayerManager 的访问
        // 暂时使用事件中的 inventory 指针
        if (e.inventory == nullptr) {
            return;
        }

        // 获取 Player 对象
        mc::Player* player = e.inventory->getPlayer();
        if (player == nullptr) {
            return;
        }

        // 转换为 ServerPlayer
        mc::ServerPlayer* serverPlayer = player->asServerPlayer();
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测 - 使用 triggerWithPredicate 方法
        // 注意：triggerWithPredicate 内部会自动检查是否有监听器
        trigger->triggerWithPredicate(
            *advancements, [&e](const mc::advancement::InventoryChangedTriggerInstance& instance) {
                return instance.testWithInventory(mc::PlayerInventory::TOTAL_SIZE,
                    [&e](i32 slot) -> mc::ItemStack { return e.inventory->getItem(slot); });
            });
    }

    /**
     * @brief 处理玩家击杀实体事件
     *
     * 触发 PlayerKilledEntityTrigger。
     */
    void _onPlayerKillEntity(const event::PlayerKillEntityEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::PlayerKilledEntityTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查受害实体和伤害源是否有效
        if (e.victim == nullptr || e.cause == nullptr) {
            return;
        }

        // 使用基类的 trigger 模板方法触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::PlayerKilledEntityTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::PlayerKilledEntityTriggerInstance& instance) {
                return instance.test(*e.victim, *e.cause);
            });
    }

    /**
     * @brief 处理玩家登录事件
     *
     * 初始化玩家的成就监听器。
     */
    void _onPlayerLogin(const event::PlayerLoginEvent& e)
    {
        // 玩家登录时，初始化成就监听器
        // 为玩家尚未追踪的所有成就注册触发器监听器
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 为所有已注册成就注册触发器监听器
        advancements->flushAdvancements(mc::advancement::AdvancementManager::instance());
    }

    /**
     * @brief 处理方块放置事件
     *
     * 触发 PlacedBlockTrigger。
     */
    void _onBlockPlaced(const event::BlockPlaceEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::PlacedBlockTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查玩家ID是否有效（可能为0表示非玩家放置）
        if (e.playerId == 0) {
            return;
        }

        // 检查方块状态是否有效
        if (e.state == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 获取世界引用（用于 LocationPredicate 检测）
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 获取使用的物品（可能为null）
        const mc::ItemStack item = e.item != nullptr ? *e.item : mc::ItemStack();

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::PlacedBlockTriggerInstance>::trigger(
            *advancements, [&e, world, &item](const mc::advancement::PlacedBlockTriggerInstance& instance) {
                return instance.test(*e.state, *world, e.pos, item);
            });
    }

    /**
     * @brief 处理僵尸村民治愈事件
     *
     * 触发 CuredZombieVillagerTrigger。
     * 更新村庄声望（MajorPositive + MinorPositive）。
     */
    void _onCuredZombieVillager(const event::CuredZombieVillagerEvent& e)
    {
        // 检查治愈发起者UUID是否有效
        if (e.starterUuid.empty()) {
            return;
        }

        // 通过 UUID 获取 PlayerId
        if (m_playerManager == nullptr) {
            return;
        }

        const mc::server::ServerPlayerData* playerData = m_playerManager->findByUuid(e.starterUuid);
        if (playerData == nullptr) {
            return;
        }

        PlayerId playerId = playerData->playerId;

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::CuredZombieVillagerTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查僵尸和村民实体是否有效
        if (e.zombie == nullptr || e.villager == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::CuredZombieVillagerTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::CuredZombieVillagerTriggerInstance& instance) {
                return instance.test(*e.zombie, *e.villager);
            });

        // 更新村庄声望
        // 治愈僵尸村民获得:
        // - MajorPositive: +20 (权重5 = +100声望)
        // - MinorPositive: +25 (权重1 = +25声望)
        // 总计: +125声望
        _updateVillageReputationOnCure(e.starterUuid, e.villager);
    }

    /**
     * @brief 治愈僵尸村民时更新村庄声望
     *
     * - MajorPositive: +20 (权重5 = +100声望，永不衰减)
     * - MinorPositive: +25 (权重1 = +25声望，每日衰减1点)
     * 总计: +125声望
     *
     * @param starterUuid 治愈者UUID
     * @param villager 治愈后的村民实体
     */
    void _updateVillageReputationOnCure(const std::string& starterUuid, Entity* villager)
    {
        if (m_server == nullptr || villager == nullptr) {
            return;
        }

        // 获取主世界（村庄和袭击仅存在于主世界）
        auto* overworld = m_server->dimensionManager().getOverworld();
        if (overworld == nullptr || overworld->world() == nullptr) {
            return;
        }
        mc::server::ServerWorld& world = *overworld->world();

        // 获取 VillageManager
        mc::world::village::VillageManager* villageManager = world.villageManager();
        if (villageManager == nullptr) {
            return;
        }

        // 获取村民位置所在的村庄
        mc::world::village::Village* village = villageManager->getVillageAt(mc::BlockPos(
            static_cast<i32>(villager->x()), static_cast<i32>(villager->y()), static_cast<i32>(villager->z())));

        if (village == nullptr) {
            // 村民不在任何村庄内，不更新声望
            return;
        }

        // 将 UUID 字符串转换为 u64 作为玩家标识符
        // 使用 std::hash 来生成一致的 u64 标识符
        u64 playerIdentifier = std::hash<std::string>{}(starterUuid);

        // 添加 MajorPositive 流言 (+20，权重5 = +100声望)
        village->addGossip(playerIdentifier, mc::world::village::VillageGossipType::MajorPositive, 20);

        // 添加 MinorPositive 流言 (+25，权重1 = +25声望)
        village->addGossip(playerIdentifier, mc::world::village::VillageGossipType::MinorPositive, 25);

        spdlog::info("VillageGossip: Player {} cured zombie villager, gained MajorPositive(+20) and MinorPositive(+25) "
                     "at village {}",
            starterUuid,
            village->getId());
    }

    /**
     * @brief 处理玩家睡眠事件
     *
     * 触发 SleptInBedTrigger。
     */
    void _onPlayerSleep(const event::PlayerSleepEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::SleptInBedTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 获取世界
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 获取位置
        mc::Vector3 pos = serverPlayer->position();

        // 触发检测 - SleptInBedTrigger 继承自 LocationTrigger
        trigger->AbstractCriterionTrigger<mc::advancement::LocationTriggerInstance>::trigger(
            *advancements, [world, &pos](const mc::advancement::LocationTriggerInstance& instance) {
                return instance.test(*world, pos.x, pos.y, pos.z);
            });
    }

    /**
     * @brief 处理效果变化事件
     *
     * 触发 EffectsChangedTrigger、HeroOfTheVillageTrigger 和 VoluntaryExileTrigger。
     */
    void _onEffectChanged(const event::EffectChangedEvent& e)
    {
        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发通用的 EffectsChangedTrigger
        {
            auto* effectsTrigger =
                mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::EffectsChangedTrigger>();
            if (effectsTrigger != nullptr) {
                effectsTrigger->AbstractCriterionTrigger<mc::advancement::EffectsChangedTriggerInstance>::trigger(
                    *advancements, [serverPlayer](const mc::advancement::EffectsChangedTriggerInstance& instance) {
                        return instance.test(*serverPlayer);
                    });
            }
        }

        // 只处理添加效果的情况，用于特定的触发器
        if (!e.added || e.effect == nullptr) {
            return;
        }

        // 获取世界
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 获取位置
        mc::Vector3 pos = serverPlayer->position();

        // 根据效果类型触发相应的触发器
        mc::entity::effect::EffectType effectType = e.effect->type();

        // 村庄英雄效果
        if (effectType == mc::entity::effect::EffectType::HeroOfTheVillage) {
            auto* trigger =
                mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::HeroOfTheVillageTrigger>();
            if (trigger != nullptr) {
                trigger->AbstractCriterionTrigger<mc::advancement::LocationTriggerInstance>::trigger(
                    *advancements, [world, &pos](const mc::advancement::LocationTriggerInstance& instance) {
                        return instance.test(*world, pos.x, pos.y, pos.z);
                    });
            }
        }
        // 不祥之兆效果
        else if (effectType == mc::entity::effect::EffectType::BadOmen) {
            auto* trigger =
                mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::VoluntaryExileTrigger>();
            if (trigger != nullptr) {
                trigger->AbstractCriterionTrigger<mc::advancement::LocationTriggerInstance>::trigger(
                    *advancements, [world, &pos](const mc::advancement::LocationTriggerInstance& instance) {
                        return instance.test(*world, pos.x, pos.y, pos.z);
                    });
            }
        }
    }

    /**
     * @brief 处理玩家位置事件
     *
     * 触发 LocationTrigger。
     */
    void _onPlayerLocation(const event::PlayerLocationEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::LocationTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 获取世界
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::LocationTriggerInstance>::trigger(
            *advancements, [world, &e](const mc::advancement::LocationTriggerInstance& instance) {
                return instance.test(*world, e.position.x, e.position.y, e.position.z);
            });
    }

    /**
     * @brief 处理维度变化事件
     *
     * 触发 LocationTrigger（维度变化后检测新位置）。
     */
    void _onDimensionChange(const event::DimensionChangeEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::LocationTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 获取世界（维度变化后玩家已在新维度）
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::LocationTriggerInstance>::trigger(
            *advancements, [world, &e](const mc::advancement::LocationTriggerInstance& instance) {
                return instance.test(*world, e.position.x, e.position.y, e.position.z);
            });
    }

    /**
     * @brief 处理引雷附魔触发事件
     *
     * 触发 ChanneledLightningTrigger。
     */
    void _onChanneledLightning(const event::ChanneledLightningEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::ChanneledLightningTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.casterId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 转换实体列表为 const 指针列表
        std::vector<const mc::Entity*> victimPtrs;
        victimPtrs.reserve(e.victims.size());
        for (const auto* victim : e.victims) {
            victimPtrs.push_back(victim);
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::ChanneledLightningTriggerInstance>::trigger(
            *advancements, [&victimPtrs](const mc::advancement::ChanneledLightningTriggerInstance& instance) {
                return instance.test(victimPtrs);
            });
    }

    /**
     * @brief 处理消耗物品事件
     *
     * 触发 ConsumeItemTrigger。
     */
    void _onConsumeItem(const event::ConsumeItemEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::ConsumeItemTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::ConsumeItemTriggerInstance>::trigger(*advancements,
            [&e](const mc::advancement::ConsumeItemTriggerInstance& instance) { return instance.test(e.item); });
    }

    /**
     * @brief 处理物品耐久变化事件
     *
     * 触发 ItemDurabilityTrigger。
     */
    void _onItemDurability(const event::ItemDurabilityEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::ItemDurabilityTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::ItemDurabilityTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::ItemDurabilityTriggerInstance& instance) {
                return instance.test(e.item, e.oldDurability);
            });
    }

    /**
     * @brief 处理附魔事件
     *
     * 触发 EnchantedItemTrigger。
     */
    void _onEnchantItem(const event::EnchantItemEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::EnchantedItemTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::EnchantedItemTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::EnchantedItemTriggerInstance& instance) {
                return instance.test(e.item, e.levels);
            });
    }

    /**
     * @brief 处理填充桶事件
     *
     * 触发 FilledBucketTrigger。
     */
    void _onFilledBucket(const event::FilledBucketEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::FilledBucketTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测
        trigger->AbstractCriterionTrigger<mc::advancement::FilledBucketTriggerInstance>::trigger(*advancements,
            [&e](const mc::advancement::FilledBucketTriggerInstance& instance) { return instance.test(e.bucket); });
    }

    /**
     * @brief 处理进入方块事件
     *
     * 触发 EnterBlockTrigger。
     */
    void _onEnterBlock(const event::EnterBlockEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::EnterBlockTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查方块状态是否有效
        if (e.state == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 获取世界
        mc::server::ServerWorld* world = serverPlayer->getWorld();
        if (world == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::EnterBlockTriggerInstance>::trigger(
            *advancements, [&e, world](const mc::advancement::EnterBlockTriggerInstance& instance) {
                return instance.test(*e.state, *world, e.pos);
            });
    }

    /**
     * @brief 处理滑落方块事件
     *
     * 触发 SlideDownBlockTrigger。
     */
    void _onSlideDownBlock(const event::SlideDownBlockEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::SlideDownBlockTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查方块状态是否有效
        if (e.state == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::SlideDownBlockTriggerInstance>::trigger(*advancements,
            [&e](const mc::advancement::SlideDownBlockTriggerInstance& instance) { return instance.test(*e.state); });
    }

    /**
     * @brief 处理蜂巢破坏事件
     *
     * 触发 BeeNestDestroyedTrigger。
     */
    void _onBeeNestDestroyed(const event::BeeNestDestroyedEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::BeeNestDestroyedTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查方块状态是否有效
        if (e.state == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::BeeNestDestroyedTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::BeeNestDestroyedTriggerInstance& instance) {
                return instance.test(*e.state, e.tool, e.numBeesInside);
            });
    }

    /**
     * @brief 处理动物繁殖事件
     *
     * 触发 BredAnimalsTrigger。
     * 动物繁殖时触发，检查子代和父母的谓词条件。
     */
    void _onBredAnimals(const event::BredAnimalsEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::BredAnimalsTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查子代和父母实体是否有效
        if (e.child == nullptr || e.parent1 == nullptr || e.parent2 == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::BredAnimalsTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::BredAnimalsTriggerInstance& instance) {
                return instance.test(*e.child, *e.parent1, *e.parent2);
            });
    }

    /**
     * @brief 处理村民交易事件
     *
     * 当玩家与村民/流浪商人完成交易时触发 VillagerTradeTrigger。
     * 同时更新统计 traded_with_villager。
     */
    void _onVillagerTrade(const event::VillagerTradeEvent& e)
    {
        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 更新统计：traded_with_villager
        serverPlayer->getStats().incrementCustom(mc::ResourceLocation("minecraft:traded_with_villager"));

        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::VillagerTradeTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查村民实体是否有效
        if (e.villager == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        // bought = 玩家买到的物品（交易结果），即 VillagerTradeTriggerInstance 匹配的物品条件
        trigger->AbstractCriterionTrigger<mc::advancement::VillagerTradeTriggerInstance>::trigger(
            *advancements, [&e](const mc::advancement::VillagerTradeTriggerInstance& instance) {
                return instance.test(*e.villager, e.bought);
            });
    }

    /**
     * @brief 处理动物驯服事件
     *
     * 触发 TameAnimalTrigger。
     * 当玩家成功驯服动物时触发（狼、猫、鹦鹉、马等）。
     */
    void _onTameAnimal(const event::TameAnimalEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::TameAnimalTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查动物实体是否有效
        if (e.animal == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::TameAnimalTriggerInstance>::trigger(*advancements,
            [&e](const mc::advancement::TameAnimalTriggerInstance& instance) { return instance.test(*e.animal); });
    }

    /**
     * @brief 处理实体召唤事件
     *
     * 触发 SummonedEntityTrigger。
     * 当玩家建造铁傀儡/雪傀儡/凋灵或使用 /summon 命令时触发。
     */
    void _onSummonedEntity(const event::SummonedEntityEvent& e)
    {
        // 召唤者玩家ID可能为0（非玩家召唤），跳过
        if (e.playerId == 0) {
            return;
        }

        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::SummonedEntityTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = _getServerPlayer(e.playerId);
        if (serverPlayer == nullptr) {
            return;
        }

        // 检查是否有监听器
        auto* advancements = serverPlayer->getAdvancements();
        if (advancements == nullptr) {
            return;
        }

        // 检查实体是否有效
        if (e.entity == nullptr) {
            return;
        }

        // 触发检测 - 使用基类模板方法
        trigger->AbstractCriterionTrigger<mc::advancement::SummonedEntityTriggerInstance>::trigger(*advancements,
            [&e](const mc::advancement::SummonedEntityTriggerInstance& instance) { return instance.test(*e.entity); });
    }

    /**
     * @brief 处理服务端Tick事件
     *
     * 每tick触发一次 TickTrigger，用于检测持续条件。
     * 仅当有玩家在线且有 TickTrigger 监听器时才会执行检查。
     */
    void _onServerTick(const event::ServerTickEvent& e)
    {
        // 获取 TickTrigger
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::TickTrigger>();
        if (trigger == nullptr) {
            return;
        }

        // 遍历所有在线玩家触发 TickTrigger
        if (m_server == nullptr) {
            return;
        }

        auto& entityManager = m_server->playerEntityManager();
        auto playerIds = entityManager.getPlayerIds();
        for (PlayerId playerId : playerIds) {
            mc::ServerPlayer* serverPlayer = _getServerPlayer(playerId);
            if (serverPlayer == nullptr) {
                continue;
            }

            auto* advancements = serverPlayer->getAdvancements();
            if (advancements == nullptr) {
                continue;
            }

            // 快速检查：是否有 TickTrigger 的监听器
            if (!trigger->hasListeners(*advancements)) {
                continue;
            }

            // TickTrigger 对所有监听器都触发（无条件）
            trigger->AbstractCriterionTrigger<mc::advancement::TickTriggerInstance>::trigger(
                *advancements, [](const mc::advancement::TickTriggerInstance& /*instance*/) { return true; });
        }
    }

    /**
     * @brief 从 PlayerId 获取 ServerPlayer
     * @param playerId 玩家ID
     * @return ServerPlayer 指针，如果未找到返回 nullptr
     *
     * 通过 ServerPlayerEntityManager 获取玩家实体。
     */
    [[nodiscard]] mc::ServerPlayer* _getServerPlayer(PlayerId playerId)
    {
        if (m_server == nullptr) {
            return nullptr;
        }

        ServerWorld* world = m_server->getPlayerWorld(playerId);
        if (world == nullptr) {
            return nullptr;
        }

        auto& entityManager = m_server->playerEntityManager();

        // 通过 PlayerId 获取 Player 实体
        mc::Player* player = entityManager.getPlayerEntity(playerId, *world);
        if (player == nullptr) {
            return nullptr;
        }

        // 转换为 ServerPlayer
        return player->asServerPlayer();
    }

    // 事件订阅
    event::ServerEventBus::Subscription<event::InventoryChangedEvent> m_inventoryChangedSubscription;
    event::ServerEventBus::Subscription<event::PlayerKillEntityEvent> m_playerKillSubscription;
    event::ServerEventBus::Subscription<event::PlayerLoginEvent> m_playerLoginSubscription;
    event::ServerEventBus::Subscription<event::BlockPlaceEvent> m_blockPlaceSubscription;
    event::ServerEventBus::Subscription<event::CuredZombieVillagerEvent> m_curedZombieVillagerSubscription;
    event::ServerEventBus::Subscription<event::PlayerSleepEvent> m_playerSleepSubscription;
    event::ServerEventBus::Subscription<event::EffectChangedEvent> m_effectChangedSubscription;
    event::ServerEventBus::Subscription<event::PlayerLocationEvent> m_playerLocationSubscription;
    event::ServerEventBus::Subscription<event::DimensionChangeEvent> m_dimensionChangeSubscription;
    event::ServerEventBus::Subscription<event::ChanneledLightningEvent> m_channeledLightningSubscription;
    event::ServerEventBus::Subscription<event::ConsumeItemEvent> m_consumeItemSubscription;
    event::ServerEventBus::Subscription<event::ItemDurabilityEvent> m_itemDurabilitySubscription;
    event::ServerEventBus::Subscription<event::EnchantItemEvent> m_enchantItemSubscription;
    event::ServerEventBus::Subscription<event::FilledBucketEvent> m_filledBucketSubscription;
    event::ServerEventBus::Subscription<event::EnterBlockEvent> m_enterBlockSubscription;
    event::ServerEventBus::Subscription<event::SlideDownBlockEvent> m_slideDownBlockSubscription;
    event::ServerEventBus::Subscription<event::BeeNestDestroyedEvent> m_beeNestDestroyedSubscription;
    event::ServerEventBus::Subscription<event::BredAnimalsEvent> m_bredAnimalsSubscription;
    event::ServerEventBus::Subscription<event::VillagerTradeEvent> m_villagerTradeSubscription;
    event::ServerEventBus::Subscription<event::TameAnimalEvent> m_tameAnimalSubscription;
    event::ServerEventBus::Subscription<event::SummonedEntityEvent> m_summonedEntitySubscription;
    event::ServerEventBus::Subscription<event::ServerTickEvent> m_serverTickSubscription;

    // 服务器接口（用于获取 ServerPlayerEntityManager 和 ServerWorld）
    IServer* m_server = nullptr;

    // 玩家管理器（用于 UUID 查找）
    mc::server::core::PlayerManager* m_playerManager = nullptr;

    bool m_initialized = false;
};

} // namespace mc::server::advancement
