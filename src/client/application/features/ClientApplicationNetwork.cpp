#include "../ClientApplication.hpp"

#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace mc::client {

void ClientApplication::setupNetworkCallbacks()
{
    MC_TRACE_EVENT("client.initialization", "SetupNetworkCallbacks");

    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess = [this](PlayerId playerId, const String& username) {
        spdlog::info("Login successful: playerId={}, username={}", playerId, username);
        if (m_player) {
            m_player->setPlayerId(playerId);
        }
        m_knownPlayerNames[playerId] = username;
    };

    callbacks.onLoginFailed = [this](const String& reason) {
        spdlog::error("Login failed: {}", reason);

        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        stop();
    };

    callbacks.onDisconnected = [this](const String& reason) {
        spdlog::info("Disconnected from server: {}", reason);
        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        m_hasServerTimeSync = false;
        if (m_integratedServer) {
            m_integratedServer->stop();
            m_integratedServer.reset();
        }
        stop();
    };

    callbacks.onCommandTree = [this](const String& treeJson) {
        if (!m_commandManager) {
            m_commandManager = std::make_unique<command::ClientCommandManager>();
        }
        auto result = m_commandManager->applyCommandTreeJson(treeJson);
        if (result.failed()) {
            spdlog::warn("Failed to apply command tree: {}", result.error().toString());
            return;
        }
        m_commandManager->setPlayerNameProvider([this]() {
            return collectPlayerCompletionCandidates();
        });
        m_commandManager->setEntityNameProvider([this]() {
            return collectEntityCompletionCandidates();
        });
    };

    callbacks.onGameModeChange = [this](GameMode gameMode) {
        if (!m_player) {
            return;
        }

        spdlog::info("Game mode changed to: {}", static_cast<i32>(gameMode));
        m_player->setGameMode(gameMode);

        closeInventoryScreenIfModeMismatch();
    };

    callbacks.onPlayerAbilities = [this](bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed) {
        spdlog::debug("Player abilities updated: invulnerable={}, flying={}, canFly={}, creativeMode={}",
                      invulnerable, flying, canFly, creativeMode);
        if (m_player) {
            PlayerAbilities& abilities = m_player->abilities();
            abilities.invulnerable = invulnerable;
            abilities.flying = flying;
            abilities.canFly = canFly;
            abilities.creativeMode = creativeMode;
            abilities.flySpeed = flySpeed;
            abilities.walkSpeed = walkSpeed;
        }

        closeInventoryScreenIfModeMismatch();
    };

    callbacks.onLightUpdate = [this](i32 chunkX, i32 chunkZ, i32 sectionY,
                                      const std::vector<u8>& skyLight,
                                      const std::vector<u8>& blockLight,
                                      bool trustEdges) {
        m_world.onLightUpdate(chunkX, chunkZ, sectionY, skyLight, blockLight, trustEdges);
    };

    callbacks.onBlockBreakAnim = [this](EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage) {
        using namespace mc::client::renderer::trident::block;
        auto& manager = BreakProgressManager::instance();

        BlockPos pos(x, y, z);
        u64 currentTick = static_cast<u64>(m_world.gameTime());

        if (stage < 0) {
            manager.removeRemoteProgress(breakerEntityId);
        } else {
            manager.updateRemoteProgress(breakerEntityId, pos, stage, currentTick);
        }
    };

    callbacks.onPlaySound = [this](const ResourceLocation& soundEventId,
                                   mc::sound::SoundCategory category,
                                   f32 x,
                                   f32 y,
                                   f32 z,
                                   f32 volume,
                                   f32 pitch) {
        if (!m_audioService) {
            spdlog::warn("Received sound event '{}' but audio service is not initialized", soundEventId.toString());
            return;
        }

        auto sound = sound::SoundInstance::createLocated(
            soundEventId,
            category,
            x,
            y,
            z,
            volume,
            pitch);

        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
    };

    callbacks.onStopSound = [this](const Optional<ResourceLocation>& soundEventId,
                                   const Optional<mc::sound::SoundCategory>& category) {
        if (!m_audioService) {
            return;
        }

        if (!soundEventId.has_value() && !category.has_value()) {
            m_audioService->stopAll();
            return;
        }

        if (soundEventId.has_value()) {
            m_audioService->stop(*soundEventId);
            return;
        }

        if (category.has_value()) {
            m_audioService->stop(*category);
        }
    };

    m_networkClient->setCallbacks(callbacks);
}

std::vector<String> ClientApplication::collectPlayerCompletionCandidates() const
{
    std::vector<String> candidates;
    candidates.reserve(m_knownPlayerNames.size() + 1);

    for (const auto& [playerId, playerName] : m_knownPlayerNames) {
        MC_UNUSED(playerId);
        if (!playerName.empty()) {
            candidates.push_back(playerName);
        }
    }

    if (m_player) {
        const auto& username = m_player->username();
        if (!username.empty()) {
            candidates.push_back(username);
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    return candidates;
}

std::vector<String> ClientApplication::collectEntityCompletionCandidates() const
{
    return collectPlayerCompletionCandidates();
}

void ClientApplication::handleChatCommand(const String& input)
{
    if (input.empty()) {
        return;
    }

    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;

    if (chatWidget) {
        chatWidget->addMessage(input, 0xFFFFFFFF);
    }

    if (input[0] == '/') {
        String command = input.substr(1);

        spdlog::info("Chat command received: {}", std::string(command.begin(), command.end()));

        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Command executed locally (not connected to server)");
        }
    } else {
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Message sent locally (not connected to server)");
        }
    }
}

} // namespace mc::client