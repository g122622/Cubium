#pragma once

#include "server/event/ServerEventBus.hpp"
#include "server/event/events/ServerEvents.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/advancement/PlayerAdvancements.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/InventoryChangedTrigger.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "server/advancement/TriggerInstantiation.hpp"

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
     * @brief 初始化事件处理器
     *
     * 订阅所有相关的事件。
     */
    void initialize() {
        // 订阅物品栏变化事件
        m_inventoryChangedSubscription =
            ServerEventBus::instance().makeSubscription<event::InventoryChangedEvent>(
                [this](const event::InventoryChangedEvent& e) {
                    onInventoryChanged(e);
                }
            );

        // 订阅玩家击杀实体事件
        m_playerKillSubscription =
            ServerEventBus::instance().makeSubscription<event::PlayerKillEntityEvent>(
                [this](const event::PlayerKillEntityEvent& e) {
                    onPlayerKillEntity(e);
                }
            );

        // 订阅玩家登录事件（初始化成就监听器）
        m_playerLoginSubscription =
            ServerEventBus::instance().makeSubscription<event::PlayerLoginEvent>(
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
        m_inventoryChangedSubscription.release();
        m_playerKillSubscription.release();
        m_playerLoginSubscription.release();
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

        if (!trigger->hasListeners(*advancements)) {
            return;
        }

        // 触发检测 - 使用 triggerWithPredicate 方法
        trigger->triggerWithPredicate(*advancements, [&e](const mc::advancement::InventoryChangedTriggerInstance& instance) {
            return instance.testWithInventory(
                mc::PlayerInventory::TOTAL_SIZE,
                [&e](i32 slot) -> const mc::ItemStack& {
                    return e.inventory->getItem(slot);
                }
            );
        });
    }

    /**
     * @brief 处理玩家击杀实体事件
     *
     * 触发 PlayerKilledEntityTrigger。
     */
    void onPlayerKillEntity(const event::PlayerKillEntityEvent& e) {
        // TODO: 实现 PlayerKilledEntityTrigger 触发
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

    // 事件订阅
    ServerEventBus::Subscription<event::InventoryChangedEvent> m_inventoryChangedSubscription;
    ServerEventBus::Subscription<event::PlayerKillEntityEvent> m_playerKillSubscription;
    ServerEventBus::Subscription<event::PlayerLoginEvent> m_playerLoginSubscription;

    bool initialized_ = false;
};

} // namespace mc::server::advancement
