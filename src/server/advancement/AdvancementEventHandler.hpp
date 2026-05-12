#pragma once

#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "server/advancement/TriggerInstantiation.hpp"

// 前向声明
namespace mc::server::core {
    class PlayerManager;
}

namespace mc::server::advancement {

/**
 * @brief 成就事件处理器
 *
 * 订阅服务端事件并触发相应的成就触发器。
 * 参考 MC 1.16.5 的 CriteriaTriggers 触发机制。
 */
class AdvancementEventHandler {
public:
    /**
     * @brief 设置玩家管理器
     * @param playerManager 玩家管理器指针
     */
    void setPlayerManager(core::PlayerManager* playerManager) {
        m_playerManager = playerManager;
    }

    /**
     * @brief 初始化事件处理器
     *
     * 订阅所有相关的事件。
     */
    void initialize() {
        // 订阅物品栏变化事件
        m_inventoryChangedSubscription =
            event::ServerEventBus::instance().makeSubscription<event::InventoryChangedEvent>(
                [this](const event::InventoryChangedEvent& e) {
                    onInventoryChanged(e);
                }
            );

        // 订阅玩家击杀实体事件
        m_playerKillSubscription =
            event::ServerEventBus::instance().makeSubscription<event::PlayerKillEntityEvent>(
                [this](const event::PlayerKillEntityEvent& e) {
                    onPlayerKillEntity(e);
                }
            );

        // 订阅玩家登录事件（初始化成就监听器）
        m_playerLoginSubscription =
            event::ServerEventBus::instance().makeSubscription<event::PlayerLoginEvent>(
                [this](const event::PlayerLoginEvent& e) {
                    onPlayerLogin(e);
                }
            );

        initialized_ = true;
    }

    /**
     * @brief 关闭事件处理器
     *
     * 取消所有事件订阅。
     */
    void shutdown() {
        m_inventoryChangedSubscription.unsubscribe();
        m_playerKillSubscription.unsubscribe();
        m_playerLoginSubscription.unsubscribe();
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
    void onInventoryChanged(const event::InventoryChangedEvent& e) {
        // 获取触发器
        auto* trigger = mc::advancement::CriterionTriggers::instance()
            .getTrigger<mc::advancement::InventoryChangedTrigger>();

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
        trigger->triggerWithPredicate(*advancements, [&e](const mc::advancement::InventoryChangedTriggerInstance& instance) {
            return instance.testWithInventory(
                mc::PlayerInventory::TOTAL_SIZE,
                [&e](i32 slot) -> mc::ItemStack {
                    return e.inventory->getItem(slot);
                }
            );
        });
    }

    /**
     * @brief 处理玩家击杀实体事件
     *
     * 触发 PlayerKilledEntityTrigger。
     * 参考 MC 1.16.5: CriteriaTriggers.PLAYER_KILLED_ENTITY
     *
     * 注意：此方法已准备就绪，等待以下依赖完成：
     * 1. PlayerKilledEntityTrigger 需要注册到 CriterionTriggers
     *    （需要先实现 DistancePredicate.hpp）
     * 2. PlayerManager 需要提供 ServerPlayer 访问接口
     */
    void onPlayerKillEntity(const event::PlayerKillEntityEvent& e) {
        // TODO: 当 DistancePredicate 实现后，取消注释以下代码
        // 获取触发器
        // auto* trigger = mc::advancement::CriterionTriggers::instance()
        //     .getTrigger<mc::advancement::PlayerKilledEntityTrigger>();
        //
        // if (trigger == nullptr) {
        //     return;
        // }
        //
        // // 获取 ServerPlayer
        // mc::ServerPlayer* serverPlayer = getServerPlayer(e.playerId);
        // if (serverPlayer == nullptr) {
        //     return;
        // }
        //
        // // 检查是否有监听器
        // auto* advancements = serverPlayer->getAdvancements();
        // if (advancements == nullptr) {
        //     return;
        // }
        //
        // // 检查受害实体和伤害源是否有效
        // if (e.victim == nullptr || e.cause == nullptr) {
        //     return;
        // }
        //
        // // 触发检测
        // trigger->trigger(*serverPlayer, *e.victim, *e.cause);
        MC_UNUSED(e);
    }

    /**
     * @brief 处理玩家登录事件
     *
     * 初始化玩家的成就监听器。
     */
    void onPlayerLogin(const event::PlayerLoginEvent& e) {
        // 玩家登录时，PlayerAdvancements 已经在 ServerPlayer 构造函数中初始化
        // 这里可以做一些额外的初始化工作
        MC_UNUSED(e);
    }

    /**
     * @brief 从 PlayerId 获取 ServerPlayer
     * @param playerId 玩家ID
     * @return ServerPlayer 指针，如果未找到返回 nullptr
     *
     * 注意：此方法需要 PlayerManager 提供访问 ServerPlayer 的接口
     */
    [[nodiscard]] mc::ServerPlayer* getServerPlayer(PlayerId playerId) {
        if (m_playerManager == nullptr) {
            return nullptr;
        }

        auto* playerData = m_playerManager->getPlayer(playerId);
        if (playerData == nullptr) {
            return nullptr;
        }

        // TODO: ServerPlayerData 需要提供访问 ServerPlayer 的方法
        // 或者 PlayerManager 应该提供 getPlayerEntity(playerId) 方法
        MC_UNUSED(playerData);
        return nullptr;
    }

    // 事件订阅
    event::ServerEventBus::Subscription<event::InventoryChangedEvent> m_inventoryChangedSubscription;
    event::ServerEventBus::Subscription<event::PlayerKillEntityEvent> m_playerKillSubscription;
    event::ServerEventBus::Subscription<event::PlayerLoginEvent> m_playerLoginSubscription;

    // 玩家管理器
    core::PlayerManager* m_playerManager = nullptr;

    bool initialized_ = false;
};

} // namespace mc::server::advancement
