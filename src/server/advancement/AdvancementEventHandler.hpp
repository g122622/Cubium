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

#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/BlockTriggers.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/advancement/trigger/impl/ItemTriggers.hpp"
#include "common/advancement/trigger/impl/LocationTrigger.hpp"
#include "common/advancement/trigger/impl/PlayerKilledEntityTrigger.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/world/IWorld.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "server/advancement/TriggerInstantiation.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

namespace mc::server::advancement {

/**
 * @brief 成就事件处理器
 *
 * 订阅服务端事件并触发相应的成就触发器。
 * 参考 MC 1.16.5 的 CriteriaTriggers 触发机制。
 *
 * ## 架构说明
 *
 * AdvancementEventHandler 需要从 PlayerId 获取 ServerPlayer 以触发成就检测。
 * 这通过以下路径实现：
 *
 * 1. IServer::playerEntityManager() → ServerPlayerEntityManager
 * 2. ServerPlayerEntityManager::getPlayerEntity(playerId, world) → Player*
 * 3. Player::asServerPlayer() → ServerPlayer*
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
                [this](const event::InventoryChangedEvent& e) { onInventoryChanged(e); });

        // 订阅玩家击杀实体事件
        m_playerKillSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerKillEntityEvent>(
            [this](const event::PlayerKillEntityEvent& e) { onPlayerKillEntity(e); });

        // 订阅玩家登录事件（初始化成就监听器）
        m_playerLoginSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerLoginEvent>(
            [this](const event::PlayerLoginEvent& e) { onPlayerLogin(e); });

        // 订阅方块放置事件
        m_blockPlaceSubscription = event::ServerEventBus::instance().makeSubscription<event::BlockPlaceEvent>(
            [this](const event::BlockPlaceEvent& e) { onBlockPlaced(e); });

        // 订阅僵尸村民治愈事件
        m_curedZombieVillagerSubscription =
            event::ServerEventBus::instance().makeSubscription<event::CuredZombieVillagerEvent>(
                [this](const event::CuredZombieVillagerEvent& e) { onCuredZombieVillager(e); });

        // 订阅玩家睡眠事件
        m_playerSleepSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerSleepEvent>(
            [this](const event::PlayerSleepEvent& e) { onPlayerSleep(e); });

        // 订阅效果变化事件
        m_effectChangedSubscription = event::ServerEventBus::instance().makeSubscription<event::EffectChangedEvent>(
            [this](const event::EffectChangedEvent& e) { onEffectChanged(e); });

        // 订阅玩家位置事件
        m_playerLocationSubscription = event::ServerEventBus::instance().makeSubscription<event::PlayerLocationEvent>(
            [this](const event::PlayerLocationEvent& e) { onPlayerLocation(e); });

        // 订阅维度变化事件
        m_dimensionChangeSubscription =
            event::ServerEventBus::instance().makeSubscription<event::DimensionChangeEvent>(
                [this](const event::DimensionChangeEvent& e) { onDimensionChange(e); });

        initialized_ = true;
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
        initialized_ = false;
    }

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }

private:
    /**
     * @brief 处理物品栏变化事件
     *
     * 触发 InventoryChangedTrigger。
     * 参考 MC 1.16.5: InventoryChangeListener.onInventoryChange
     */
    void onInventoryChanged(const event::InventoryChangedEvent& e)
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
     * 参考 MC 1.16.5: CriteriaTriggers.PLAYER_KILLED_ENTITY
     */
    void onPlayerKillEntity(const event::PlayerKillEntityEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::PlayerKilledEntityTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
        // 参考 TriggerInstantiation.hpp
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
    void onPlayerLogin(const event::PlayerLoginEvent& e)
    {
        // 玩家登录时，PlayerAdvancements 已经在 ServerPlayer 构造函数中初始化
        // 这里可以做一些额外的初始化工作
        MC_UNUSED(e);
    }

    /**
     * @brief 处理方块放置事件
     *
     * 触发 PlacedBlockTrigger。
     * 参考 MC 1.16.5: CriteriaTriggers.PLACED_BLOCK.trigger()
     */
    void onBlockPlaced(const event::BlockPlaceEvent& e)
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
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
     * 参考 MC 1.16.5: CriteriaTriggers.CURED_ZOMBIE_VILLAGER.trigger()
     */
    void onCuredZombieVillager(const event::CuredZombieVillagerEvent& e)
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
        mc::ServerPlayer* serverPlayer = getServerPlayer(playerId);
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
    }

    /**
     * @brief 处理玩家睡眠事件
     *
     * 触发 SleptInBedTrigger。
     * 参考 MC 1.16.5: CriteriaTriggers.SLEPT_IN_BED.trigger()
     */
    void onPlayerSleep(const event::PlayerSleepEvent& e)
    {
        // 获取触发器
        auto* trigger =
            mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::SleptInBedTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
     * 触发 HeroOfTheVillageTrigger 和 VoluntaryExileTrigger。
     * 参考 MC 1.16.5: CriteriaTriggers.EFFECTS_CHANGED.trigger()
     */
    void onEffectChanged(const event::EffectChangedEvent& e)
    {
        // 只处理添加效果的情况
        if (!e.added || e.effect == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
     * 参考 MC 1.16.5: CriteriaTriggers.LOCATION.trigger()
     */
    void onPlayerLocation(const event::PlayerLocationEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::LocationTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
     * 参考 MC 1.16.5: 玩家传送后触发 location 触发器
     */
    void onDimensionChange(const event::DimensionChangeEvent& e)
    {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::LocationTrigger>();

        if (trigger == nullptr) {
            return;
        }

        // 获取 ServerPlayer
        mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
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
     * @brief 从 PlayerId 获取 ServerPlayer
     * @param playerId 玩家ID
     * @return ServerPlayer 指针，如果未找到返回 nullptr
     *
     * 通过 ServerPlayerEntityManager 获取玩家实体。
     * 参考 MC 1.16.5: PlayerList.getPlayerByUUID()
     */
    [[nodiscard]] mc::ServerPlayer* getServerPlayer(PlayerId playerId)
    {
        if (m_server == nullptr) {
            return nullptr;
        }

        // 获取 ServerPlayerEntityManager
        auto& entityManager = m_server->playerEntityManager();

        // 获取 ServerWorld
        auto& world = m_server->world();

        // 通过 PlayerId 获取 Player 实体
        mc::Player* player = entityManager.getPlayerEntity(playerId, world);
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

    // 服务器接口（用于获取 ServerPlayerEntityManager 和 ServerWorld）
    IServer* m_server = nullptr;

    // 玩家管理器（用于 UUID 查找）
    mc::server::core::PlayerManager* m_playerManager = nullptr;

    bool initialized_ = false;
};

} // namespace mc::server::advancement
