#pragma once

#include "common/core/Types.hpp"

#include <memory>
#include <vector>

namespace mc::server::event {
class ServerEventBus;
}

namespace mc::mod::bedrock::addon {
class ScriptEventBus;
}

namespace mc::server {

/**
 * @brief 事件桥接器
 *
 * 将ServerEventBus中的游戏事件桥接到ScriptEventBus，
 * 使JS脚本可以通过world.beforeEvents/afterEvents订阅游戏事件。
 *
 * 桥接流程：
 * ServerEventBus → EventBridge → ScriptEventBus → JS回调
 *
 * BeforeEvents：同步、可取消，脚本可以阻止原始操作
 * AfterEvents：延迟批量处理，在tick结束时分发
 *
 * 使用方式：
 * 1. 创建EventBridge实例
 * 2. 调用initialize()订阅游戏事件
 * 3. 在服务器关闭时调用shutdown()取消订阅
 */
class EventBridge {
public:
    EventBridge();
    ~EventBridge();

    // 禁止拷贝
    EventBridge(const EventBridge&) = delete;
    EventBridge& operator=(const EventBridge&) = delete;

    /**
     * @brief 初始化事件桥接
     *
     * 订阅ServerEventBus中的游戏事件，
     * 将事件转发到ScriptEventBus。
     *
     * @param serverEventBus 服务器事件总线
     * @param scriptEventBus 脚本事件总线
     */
    void initialize(event::ServerEventBus& serverEventBus, mc::mod::bedrock::addon::ScriptEventBus& scriptEventBus);

    /**
     * @brief 关闭事件桥接
     *
     * 取消所有ServerEventBus订阅。
     */
    void shutdown();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const;

private:
    /**
     * @brief 订阅单个游戏事件到afterEvent
     */
    void subscribeAfterEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus);

    /**
     * @brief 订阅单个游戏事件到beforeEvent
     */
    void subscribeBeforeEvents(event::ServerEventBus& bus, mc::mod::bedrock::addon::ScriptEventBus& scriptBus);

    /// RAII订阅句柄
    std::vector<size_t> m_subscriptionIds;

    bool m_initialized = false;
};

} // namespace mc::server
