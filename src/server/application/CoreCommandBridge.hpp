#pragma once

#include "server/application/MinecraftServer.hpp"
#include "server/core/ServerCore.hpp"
#include "server/core/TimeManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/player/ServerPlayer.hpp"
#include <spdlog/spdlog.h>

namespace mc::server {

/**
 * @brief 统一的服务端命令桥接类
 *
 * 为 IntegratedServer 和 ServerApplication 提供统一的 MinecraftServer 实现。
 * 通过 ServerCore 和可选的 ServerWorld 提供命令执行所需的上下文。
 *
 * 使用示例：
 * @code
 * CoreCommandBridge bridge(m_world, m_serverCore.get());
 * command::ServerCommandSource source(&bridge, ...);
 * registry.execute("/time set day", source);
 * @endcode
 */
class CoreCommandBridge final : public MinecraftServer {
public:
    /**
     * @brief 构造命令桥接
     * @param world 服务端世界（可为 nullptr，用于内置服务器）
     * @param core 服务端核心（必须有效）
     */
    CoreCommandBridge(ServerWorld* world, ServerCore* core)
        : m_world(world)
        , m_core(core)
    {
    }

    // ========== MinecraftServer 接口实现 ==========

    [[nodiscard]] ServerWorld* getWorld() override { return m_world; }

    [[nodiscard]] i64 getSeed() const override {
        if (m_world) {
            return static_cast<i64>(m_world->config().seed);
        }
        if (m_core) {
            return static_cast<i64>(m_core->config().seed);
        }
        return 0;
    }

    [[nodiscard]] i64 getTicks() const override {
        return m_core ? static_cast<i64>(m_core->currentTick()) : 0;
    }

    [[nodiscard]] i64 getDay() const override {
        return m_core ? m_core->gameTime().dayCount() : 0;
    }

    [[nodiscard]] i64 getDayTime() const override {
        return m_core ? m_core->gameTime().dayTime() : 0;
    }

    [[nodiscard]] i64 getGameTime() const override {
        return m_core ? m_core->gameTime().gameTime() : 0;
    }

    [[nodiscard]] std::vector<::mc::ServerPlayer*> getPlayers() override {
        // TODO: IntegratedServer 目前没有 ServerPlayer 实体，只有 ServerPlayerData
        // 暂时返回空列表，后续可以创建 ServerPlayer 包装器
        return {};
    }

    [[nodiscard]] ::mc::ServerPlayer* getPlayer(const String& /*name*/) override {
        // TODO: 从 PlayerManager 查找玩家
        return nullptr;
    }

    [[nodiscard]] size_t playerCount() const override {
        return m_core ? m_core->playerCount() : 0;
    }

    void broadcast(const String& message) override {
        spdlog::info("[Broadcast] {}", message);
    }

    bool setDayTime(i64 time) override {
        if (!m_core) return false;
        m_core->timeManager().setDayTime(time);
        return true;
    }

    bool addDayTime(i64 ticks) override {
        if (!m_core) return false;
        m_core->timeManager().addDayTime(ticks);
        return true;
    }

    bool setWeatherClear(i32 duration) override {
        if (!m_core) return false;
        m_core->weatherManager().setClear(duration);
        return true;
    }

    bool setWeatherRain(i32 duration) override {
        if (!m_core) return false;
        m_core->weatherManager().setRain(duration);
        return true;
    }

    bool setWeatherThunder(i32 duration) override {
        if (!m_core) return false;
        m_core->weatherManager().setThunder(duration);
        return true;
    }

    [[nodiscard]] i32 getWeatherType() const override {
        if (!m_core) return 0;  // Clear
        return static_cast<i32>(m_core->weatherManager().weatherType());
    }

    [[nodiscard]] f32 getRainStrength() const override {
        if (!m_core) return 0.0f;
        return m_core->weatherManager().rainStrength();
    }

    [[nodiscard]] f32 getThunderStrength() const override {
        if (!m_core) return 0.0f;
        return m_core->weatherManager().thunderStrength();
    }

    bool teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch) override {
        // 优先使用 ServerCore（适用于 IntegratedServer）
        if (m_core) {
            m_core->teleportPlayer(playerId, x, y, z, yaw, pitch);
            return true;
        }
        // 回退到 ServerWorld（适用于独立服务器）
        if (m_world) {
            m_world->teleportPlayer(playerId, x, y, z, yaw, pitch);
            return true;
        }
        return false;
    }

    bool setPlayerGameMode(PlayerId playerId, GameMode mode) override {
        // 优先使用 ServerCore（适用于 IntegratedServer）
        if (m_core && m_core->setPlayerGameMode(playerId, mode)) {
            return true;
        }
        // 回退到 ServerWorld（适用于独立服务器）
        return m_world ? m_world->setPlayerGameMode(playerId, mode) : false;
    }

    [[nodiscard]] command::CommandRegistry& getCommandRegistry() override {
        return command::CommandRegistry::getGlobal();
    }

    bool isCommandAllowed(const command::ICommandSource& /*source*/, const String& /*command*/) override {
        return true;
    }

private:
    ServerWorld* m_world;
    ServerCore* m_core;
};

} // namespace mc::server
