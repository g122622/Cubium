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

#include "client/application/ClientApplication.hpp"

#include "client/chat/ChatHistory.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/particle/ParticleManager.hpp"
#include "client/renderer/trident/particle/ParticleRegistry.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/container/ItemPickerMenu.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/IGrowable.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"
#include "common/world/block/registry/MudBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

namespace {

namespace irplay = mc::network::ir::play;

/// 构造一个 Chat IR 包（离线模式：无签名、空 LastSeen）。handleChatCommand 出站用。
mc::network::ir::IrPacket makeChatPacket(const std::string& message)
{
    irplay::Chat chat;
    chat.message = message;
    chat.timestamp = 0;
    chat.salt = 0;
    chat.signature = std::nullopt;
    chat.lastSeenOffset = 0;
    chat.lastSeenAcknowledged = std::array<u8, 3>{0, 0, 0};
    chat.lastSeenChecksum = 0;
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{irplay::Chat{std::move(chat)}}};
}

/// 构造一个 ChatCommand IR 包（无签名命令提交，对齐 vanilla ServerboundChatCommandPacket）。
/// command 不含 '/' 前缀。handleChatCommand 命令分支出站用。
mc::network::ir::IrPacket makeChatCommandPacket(const std::string& command)
{
    irplay::ChatCommand cmd;
    cmd.command = command;
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{irplay::ChatCommand{std::move(cmd)}}};
}

} // namespace

void ClientApplication::setupNetworkCallbacks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "SetupNetworkCallbacks");

    // Step3 原子切换后：入站 Play/Configuration 包由 ClientPlayVisitor 处理（在
    // ClientApplicationSession 构造 m_network 时经 setPlayVisitor 注入），出站发送由
    // 各游戏逻辑点直接调 m_network->send(ir::play::*)。故本函数不再注册任何回调，
    // 保留为空壳以维持 ClientApplicationSession::initializeGameSession 的调用契约。
    // 旧的 68 个 callbacks.onXxx lambda 主体已逐字迁入 ClientPlayVisitor 的对应分支。
}

std::vector<std::string> ClientApplication::collectPlayerCompletionCandidates() const
{
    std::vector<std::string> candidates;
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

std::vector<std::string> ClientApplication::collectEntityCompletionCandidates() const
{
    return collectPlayerCompletionCandidates();
}

void ClientApplication::handleChatCommand(const std::string& input)
{
    if (input.empty()) {
        return;
    }

    auto* chatWidget = m_kageroEngine
        ? static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId))
        : nullptr;

    if (chatWidget) {
        // 本地回显：用户输入的消息显示在聊天窗口中
        chatWidget->addMessage(input, chat::ChatMessageType::Chat);
    }

    if (input[0] == '/') {
        std::string command = input.substr(1);

        spdlog::info("Chat command received: {}", std::string(command.begin(), command.end()));

        if (m_network && m_network->isPlaying()) {
            // 命令走 ChatCommand(id=6，不含 '/')，对齐 vanilla ServerboundChatCommandPacket；
            // 真Java 1.21.11 服务端按 id=6 解码。本地 LocalTransport 直传 IR 不经 codec，
            // 自有服务端 route 的 ChatCommand 分支命中。
            (void)m_network->send(makeChatCommandPacket(command));
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Command executed locally (not connected to server)");
        }
    } else {
        if (m_network && m_network->isPlaying()) {
            (void)m_network->send(makeChatPacket(input));
        } else if (chatWidget) {
            chatWidget->addSystemMessage("Message sent locally (not connected to server)");
        }
    }
}

void ClientApplication::_handleWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data)
{
    using namespace mc::world;
    using namespace mc::sound;
    using namespace mc::client::renderer::trident::particle;

    const f32 px = static_cast<f32>(x) + 0.5f;
    const f32 py = static_cast<f32>(y) + 0.5f;
    const f32 pz = static_cast<f32>(z) + 0.5f;
    math::Random random;

    switch (eventId) {
        // ========================================================================
        // 音效事件 (1000-1043)
        // ========================================================================
        case WorldEvents::DISPENSER_DISPENSE_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_DISPENSE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::DISPENSER_FAIL_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_FAIL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::DISPENSER_LAUNCH_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_DISPENSER_LAUNCH, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::FIRE_EXTINGUISH_SOUND:
            if (m_audioService) {
                if (data == 0) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(SoundEvents::BLOCK_FIRE_EXTINGUISH,
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            0.5f,
                            2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
                } else {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(SoundEvents::ENTITY_GENERIC_EXTINGUISH_FIRE,
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            0.7f,
                            1.6f + (random.nextFloat() - random.nextFloat()) * 0.4f)));
                }
            }
            break;

        case WorldEvents::GHAST_WARN_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::ENTITY_GHAST_WARN,
                        SoundCategory::Hostile,
                        px,
                        py,
                        pz,
                        10.0f,
                        random.nextFloat() * 0.2f + 0.85f)));
            }
            break;

        case WorldEvents::BLAZE_SHOOT_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::ENTITY_BLAZE_SHOOT,
                        SoundCategory::Hostile,
                        px,
                        py,
                        pz,
                        1.0f,
                        random.nextFloat() * 0.2f + 0.85f)));
            }
            break;

        case WorldEvents::ANVIL_DESTROYED_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_ANVIL_DESTROY, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::ANVIL_LAND_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_ANVIL_LAND, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::PHANTOM_BITE_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ENTITY_PHANTOM_BITE, SoundCategory::Hostile, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::ZOMBIE_CONVERT_TO_DROWNED_SOUND:
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ENTITY_ZOMBIE_VILLAGER_CONVERTED, SoundCategory::Hostile, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::CRAFTER_CRAFT_SOUND:
            // 合成器合成成功音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_CRAFTER_CRAFT, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

        case WorldEvents::CRAFTER_FAIL_SOUND:
            // 合成器合成失败音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_CRAFTER_FAIL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;

            // ========================================================================
            // 特殊效果事件 (1500-1505)
            // ========================================================================

        case WorldEvents::COMPOSTER_FILLED_UP: {
            // 堆肥桶填充事件
            // data > 0: 堆肥成功升级，播放 COMPOSTER_FILL_SUCCESS 音效
            // data <= 0: 仅填充未升级，播放 COMPOSTER_FILL 音效
            // 无论成功与否，都生成 10 个 HAPPY_VILLAGER 粒子
            if (m_audioService) {
                const auto& soundEvent =
                    (data > 0) ? SoundEvents::BLOCK_COMPOSTER_FILL_SUCCESS : SoundEvents::BLOCK_COMPOSTER_FILL;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(soundEvent, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }

            // 计算堆肥桶填充高度处的粒子位置
            // 由于客户端可能还没有最新方块状态，使用方块中心偏上作为近似位置
            const f32 particleBaseY = static_cast<f32>(y) + 0.53125f;
            for (int i = 0; i < 10; ++i) {
                f32 ppx = static_cast<f32>(x) + 0.1875f + 0.625f * random.nextFloat();
                f32 ppy = particleBaseY + random.nextFloat() * 0.46875f;
                f32 ppz = static_cast<f32>(z) + 0.1875f + 0.625f * random.nextFloat();
                f32 vx = static_cast<f32>(random.nextGaussian()) * 0.02f;
                f32 vy = static_cast<f32>(random.nextGaussian()) * 0.02f;
                f32 vz = static_cast<f32>(random.nextGaussian()) * 0.02f;

                m_world.addParticle(ParticleTypeId::HappyVillager, Vector3(ppx, ppy, ppz), Vector3(vx, vy, vz));
            }
            break;
        }

        case WorldEvents::LAVA_EXTINGUISH: {
            // 岩浆熄灭事件：播放音效 + 8个大烟雾粒子
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_LAVA_EXTINGUISH,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        0.5f,
                        2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
            }

            for (int i = 0; i < 8; ++i) {
                f32 lpx = static_cast<f32>(x) + random.nextFloat();
                f32 lpy = static_cast<f32>(y) + 1.2f;
                f32 lpz = static_cast<f32>(z) + random.nextFloat();
                m_world.addParticle(ParticleTypeId::LargeSmoke, Vector3(lpx, lpy, lpz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::BONEMEAL_PARTICLES: {
            // 骨粉粒子效果
            // data 为粒子数量，0 则生成 15 个
            i32 count = (data == 0) ? 15 : data;
            m_world.addParticle(ParticleTypeId::HappyVillager,
                Vector3(px, py, pz),
                Vector3(0.0f, 0.0f, 0.0f),
                Vector3(1.0f, 1.0f, 1.0f),
                static_cast<u32>(count));

            // 播放骨粉使用音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::ITEM_BONE_MEAL_USE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;
        }

        case WorldEvents::BREAK_BLOCK_EFFECTS: {
            // 方块破坏效果：根据方块状态ID获取正确的破坏音效和破坏粒子
            // data = 方块状态ID（BlockState::stateId()）
            // 对应 MC ClientLevel.addDestroyBlockEffect / LevelEventHandler case 2001
            const BlockState* blockState = BlockRegistry::instance().getBlockState(static_cast<u32>(data));
            if (blockState && !blockState->isAir()) {
                const BlockSoundType& soundType = blockState->getSoundType();
                if (m_audioService) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(soundType.getBreakSound(),
                            SoundCategory::Blocks,
                            px,
                            py,
                            pz,
                            (soundType.getVolume() + 1.0f) / 2.0f,
                            soundType.getPitch() * 0.8f)));
                }

                // 生成方块破碎粒子
                // 算法对齐 MC ClientLevel.addDestroyBlockEffect：
                // 获取方块的形状，对每个AABB按0.25格间距均匀分布粒子
                // 标准完整方块(1x1x1)生成 4x4x4 = 64 个粒子
                const auto& shape = blockState->getShape();
                for (const auto& box : shape.boxes()) {
                    const f32 d1 = std::min(1.0f, box.maxX - box.minX); // AABB宽度
                    const f32 d2 = std::min(1.0f, box.maxY - box.minY); // AABB高度
                    const f32 d3 = std::min(1.0f, box.maxZ - box.minZ); // AABB深度

                    const i32 countX = std::max(2, static_cast<i32>(std::ceil(d1 / 0.25)));
                    const i32 countY = std::max(2, static_cast<i32>(std::ceil(d2 / 0.25)));
                    const i32 countZ = std::max(2, static_cast<i32>(std::ceil(d3 / 0.25)));

                    for (i32 ix = 0; ix < countX; ++ix) {
                        for (i32 iy = 0; iy < countY; ++iy) {
                            for (i32 iz = 0; iz < countZ; ++iz) {
                                // 归一化位置（0~1），位于网格单元中心
                                const f32 nx = (static_cast<f32>(ix) + 0.5f) / static_cast<f32>(countX);
                                const f32 ny = (static_cast<f32>(iy) + 0.5f) / static_cast<f32>(countY);
                                const f32 nz = (static_cast<f32>(iz) + 0.5f) / static_cast<f32>(countZ);

                                // 粒子世界位置：AABB内偏移 + 方块位置
                                const f32 particleX = px + nx * d1 + box.minX;
                                const f32 particleY = py + ny * d2 + box.minY;
                                const f32 particleZ = pz + nz * d3 + box.minZ;

                                // 粒子速度：从中心向外扩散
                                const f32 vx = nx - 0.5f;
                                const f32 vy = ny - 0.5f;
                                const f32 vz = nz - 0.5f;

                                m_world.addBlockParticle(ParticleTypeId::Breaking,
                                    Vector3(particleX, particleY, particleZ),
                                    Vector3(vx, vy, vz),
                                    *blockState);
                            }
                        }
                    }
                }
            }
            break;
        }

        case WorldEvents::DISPENSER_SMOKE: {
            // 发射器烟雾粒子，data 为方向（Direction.getIndex()）
            {
                Direction dir = static_cast<Direction>(data);
                i32 stepX = Directions::xOffset(dir);
                i32 stepY = Directions::yOffset(dir);
                i32 stepZ = Directions::zOffset(dir);
                for (int i = 0; i < 10; ++i) {
                    f32 speed = static_cast<f32>(random.nextDouble() * 0.2 + 0.01);
                    f32 spx = static_cast<f32>(x) + static_cast<f32>(stepX) * 0.6f + 0.5f +
                        static_cast<f32>(stepX) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepZ) * 0.5f;
                    f32 spy = static_cast<f32>(y) + static_cast<f32>(stepY) * 0.6f + 0.5f +
                        static_cast<f32>(stepY) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepY) * 0.5f;
                    f32 spz = static_cast<f32>(z) + static_cast<f32>(stepZ) * 0.6f + 0.5f +
                        static_cast<f32>(stepZ) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepX) * 0.5f;
                    f32 svx = static_cast<f32>(stepX) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svy = static_cast<f32>(stepY) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svz = static_cast<f32>(stepZ) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    m_world.addParticle(ParticleTypeId::Smoke, Vector3(spx, spy, spz), Vector3(svx, svy, svz));
                }
            }
            break;
        }

        case WorldEvents::MOB_SPAWNER_PARTICLES: {
            // 刷怪笼成功生成实体时爆发烟雾和火焰粒子
            // 客户端在方块中心2格范围内随机生成20个烟雾粒子和20个火焰粒子
            {
                f32 cx = static_cast<f32>(x) + 0.5f;
                f32 cy = static_cast<f32>(y) + 0.5f;
                f32 cz = static_cast<f32>(z) + 0.5f;
                for (i32 i = 0; i < 20; ++i) {
                    f32 spx = cx + (random.nextFloat() - 0.5f) * 2.0f;
                    f32 spy = cy + (random.nextFloat() - 0.5f) * 2.0f;
                    f32 spz = cz + (random.nextFloat() - 0.5f) * 2.0f;
                    m_world.addParticle(ParticleTypeId::Smoke, Vector3(spx, spy, spz), Vector3(0.0f, 0.0f, 0.0f));
                    m_world.addParticle(ParticleTypeId::Flame, Vector3(spx, spy, spz), Vector3(0.0f, 0.0f, 0.0f));
                }
            }
            break;
        }

        case WorldEvents::SPAWN_EXPLOSION_PARTICLE: {
            // 爆炸粒子
            m_world.addParticle(ParticleTypeId::HugeExplosion, Vector3(px, py, pz), Vector3(0.0f, 0.0f, 0.0f));
            break;
        }

        case WorldEvents::WET_SPONGE_DRY: {
            // 湿海绵在下界变干：8个云粒子（蒸汽） + 火焰熄灭音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_FIRE_EXTINGUISH,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        0.5f,
                        2.6f + (random.nextFloat() - random.nextFloat()) * 0.8f)));
            }

            for (int i = 0; i < 8; ++i) {
                f32 lpx = static_cast<f32>(x) + random.nextFloat();
                f32 lpy = static_cast<f32>(y) + 1.2f;
                f32 lpz = static_cast<f32>(z) + random.nextFloat();
                m_world.addParticle(ParticleTypeId::LargeSmoke, Vector3(lpx, lpy, lpz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::DRIPSTONE_DRIP: {
            // 滴石滴水粒子效果
            // 事件由服务端在 maybeTransferFluid 中触发（钟乳石成功向炼药锅传输流体时）
            // 客户端需要根据钟乳石上方的流体类型选择正确的粒子类型
            {
                // 获取钟乳石尖端位置的方块状态
                const BlockState* dripstoneState = m_world.getBlockState(x, y, z);
                if (dripstoneState != nullptr) {
                    // 使用 PointedDripstoneBlock 静态方法计算粒子位置和检测流体类型
                    BlockPos tipPos(x, y, z);
                    Vector3 particlePos = blocks::PointedDripstoneBlock::getDripParticlePosition(tipPos);

                    // 检测流体类型：沿钟乳石向上搜索非滴石方块，然后检查其流体状态
                    // 由于 ClientWorld 不继承 IWorld，无法直接调用 getFluidAboveStalactite，
                    // 因此在此处内联流体检测逻辑
                    ParticleTypeId dripType = ParticleTypeId::DrippingDripstoneWater; // 默认水滴

                    if (blocks::PointedDripstoneBlock::isStalactite(*dripstoneState)) {
                        i32 searchX = x, searchY = y + 1, searchZ = z;
                        const BlockState* aboveState = nullptr;
                        for (i32 i = 0; i < 11; ++i) {
                            aboveState = m_world.getBlockState(searchX, searchY, searchZ);
                            if (aboveState == nullptr ||
                                !aboveState->is(block_registry::CaveBlocks::POINTED_DRIPSTONE)) {
                                break;
                            }
                            searchY++;
                        }
                        // aboveState 现在是根方块上方的方块
                        if (aboveState != nullptr) {
                            // 检查是否是泥巴（Mud），泥巴视为水源
                            if (aboveState->is(block_registry::MudBlocks::MUD)) {
                                dripType = ParticleTypeId::DrippingDripstoneWater;
                            } else {
                                // 检查流体状态
                                const fluid::FluidState* fluidState = aboveState->getFluidState();
                                if (fluidState != nullptr && !fluidState->isEmpty()) {
                                    const fluid::Fluid& fluid = fluidState->getFluid();
                                    if (fluid.isIn(fluid::FluidTags::LAVA())) {
                                        dripType = ParticleTypeId::DrippingDripstoneLava;
                                    }
                                }
                            }
                        }
                    }
                    m_world.addParticle(dripType, particlePos, Vector3(0.0f, 0.0f, 0.0f));
                }
            }
            break;
        }

        case WorldEvents::PLANT_GROWTH_EFFECT: {
            // 植物生长粒子与音效事件（由骨粉使用触发）
            // data 为粒子数量，0 则生成 15 个
            // 与 BONEMEAL_PARTICLES(2005) 的区别：1505 根据 IGrowable::getBoneMealType()
            // 决定粒子分布方式，同时播放骨粉使用音效。
            {
                i32 count = (data == 0) ? 15 : data;

                // 查询位置处的方块是否为 IGrowable，根据骨粉类型决定粒子分布
                const BlockState* blockState = m_world.getBlockState(x, y, z);
                if (blockState != nullptr) {
                    const Block& block = blockState->owner();
                    const IGrowable* growable = dynamic_cast<const IGrowable*>(&block);

                    if (growable != nullptr) {
                        // 获取粒子生成位置（NEIGHBOR_SPREADER 类型在方块上方，GROWER 类型在方块自身）
                        BlockPos particlePos = growable->getParticlePos(BlockPos(x, y, z));

                        switch (growable->getBoneMealType()) {
                            case IGrowable::BoneMealType::NEIGHBOR_SPREADER:
                                // 邻居传播型（草方块、菌岩等）：粒子水平扩散 3 倍数量
                                m_world.addParticle(ParticleTypeId::HappyVillager,
                                    Vector3(static_cast<f32>(particlePos.x) + 0.5f,
                                        static_cast<f32>(particlePos.y),
                                        static_cast<f32>(particlePos.z) + 0.5f),
                                    Vector3(0.0f, 0.0f, 0.0f),
                                    Vector3(3.0f, 1.0f, 3.0f),
                                    static_cast<u32>(count * 3));
                                break;

                            case IGrowable::BoneMealType::GROWER:
                            default:
                                // 自身成长型（作物、树苗等）：粒子在方块形状高度内生成
                                // 使用方块碰撞箱的 Y 轴最大值作为垂直范围
                                {
                                    // 获取方块的碰撞形状高度
                                    f32 shapeHeight = 1.0f;
                                    const CollisionShape& shape = blockState->getShape();
                                    if (!shape.isEmpty() && !shape.isFullBlock()) {
                                        // 获取碰撞箱的最大 Y 值
                                        for (const auto& box : shape.boxes()) {
                                            if (box.maxY > shapeHeight) {
                                                shapeHeight = box.maxY;
                                            }
                                        }
                                    }
                                    m_world.addParticle(ParticleTypeId::HappyVillager,
                                        Vector3(static_cast<f32>(particlePos.x) + 0.5f,
                                            static_cast<f32>(particlePos.y),
                                            static_cast<f32>(particlePos.z) + 0.5f),
                                        Vector3(0.0f, 0.0f, 0.0f),
                                        Vector3(0.5f, shapeHeight, 0.5f),
                                        static_cast<u32>(count));
                                }
                                break;
                        }
                    } else {
                        // 非 IGrowable 方块（如水面）：使用与 NEIGHBOR_SPREADER 相同的分布
                        m_world.addParticle(ParticleTypeId::HappyVillager,
                            Vector3(px, py, pz),
                            Vector3(0.0f, 0.0f, 0.0f),
                            Vector3(3.0f, 1.0f, 3.0f),
                            static_cast<u32>(count * 3));
                    }
                } else {
                    // 方块状态不可用时，使用默认的 GROWER 分布
                    m_world.addParticle(ParticleTypeId::HappyVillager,
                        Vector3(px, py, pz),
                        Vector3(0.0f, 0.0f, 0.0f),
                        Vector3(0.5f, 1.0f, 0.5f),
                        static_cast<u32>(count));
                }

                // 播放骨粉使用音效
                if (m_audioService) {
                    m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                        SoundEvents::ITEM_BONE_MEAL_USE, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
                }
            }
            break;
        }

        case WorldEvents::POINTED_DRIPSTONE_LAND_SOUND: {
            // 滴石尖锥落地音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_POINTED_DRIPSTONE_LAND, SoundCategory::Blocks, px, py, pz, 2.0f, pitch)));
            }
            break;
        }

        case WorldEvents::DRIP_LAVA_INTO_CAULDRON_SOUND: {
            // 熔岩滴入炼药锅音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_LAVA_INTO_CAULDRON,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        2.0f,
                        pitch)));
            }
            break;
        }

        case WorldEvents::DRIP_WATER_INTO_CAULDRON_SOUND: {
            // 水滴入炼药锅音效
            if (m_audioService) {
                f32 pitch = random.nextFloat() * 0.1f + 0.9f;
                m_audioService->play(std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(SoundEvents::BLOCK_POINTED_DRIPSTONE_DRIP_WATER_INTO_CAULDRON,
                        SoundCategory::Blocks,
                        px,
                        py,
                        pz,
                        2.0f,
                        pitch)));
            }
            break;
        }

        case WorldEvents::END_PORTAL_FRAME_FILL: {
            // 末地传送门框填充：播放音效
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_END_PORTAL_FRAME_FILL, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }
            break;
        }

        case WorldEvents::REDSTONE_TORCH_BURNOUT: {
            // 红石火把烧断：播放音效 + 烟雾粒子
            if (m_audioService) {
                m_audioService->play(std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                    SoundEvents::BLOCK_REDSTONE_TORCH_BURNOUT, SoundCategory::Blocks, px, py, pz, 1.0f, 1.0f)));
            }

            for (int i = 0; i < 3; ++i) {
                f32 rspx = static_cast<f32>(x) + 0.5f + (random.nextFloat() - 0.5f) * 0.3f;
                f32 rspy = static_cast<f32>(y) + 0.7f;
                f32 rspz = static_cast<f32>(z) + 0.5f + (random.nextFloat() - 0.5f) * 0.3f;
                m_world.addParticle(ParticleTypeId::Smoke, Vector3(rspx, rspy, rspz), Vector3(0.0f, 0.0f, 0.0f));
            }
            break;
        }

        case WorldEvents::SMASH_ATTACK: {
            // 重锤砸地攻击粒子效果（对应 MC LevelEvent.PARTICLES_SMASH_ATTACK = 2013）
            // 使用 DustPillar 粒子携带方块状态纹理，分两层分布：
            //   - 内层簇（count/3 个）：高斯分布聚集在中心
            //   - 外层环（count/1.5 个）：半径 3.5 的圆形均匀分布
            {
                // 获取冲击位置方块的状态，用于 DustPillar 粒子纹理
                const BlockState* blockState = m_world.getBlockState(x, y, z);
                if (blockState == nullptr || blockState->isAir()) {
                    // 方块状态不可用或为空气，跳过粒子生成
                    break;
                }

                // 中心点位于方块中心偏上 0.5 格
                f32 cx = static_cast<f32>(x) + 0.5f;
                f32 cy = static_cast<f32>(y) + 1.0f;
                f32 cz = static_cast<f32>(z) + 0.5f;

                i32 count = (data == 0) ? 750 : data;

                // 内层簇：count/3 个粒子，高斯分布在中心附近
                // 速度会被 DustPillarProvider 覆盖：X/Z → gaussian/30，Y → 传入Y + gaussian/2
                i32 innerCount = static_cast<i32>(count / 3.0f);
                for (i32 i = 0; i < innerCount; ++i) {
                    f32 px = cx + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 py = cy;
                    f32 pz = cz + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 vx = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    f32 vy = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    f32 vz = static_cast<f32>(random.nextGaussian()) * 0.2f;
                    m_world.addBlockParticle(
                        ParticleTypeId::DustPillar, Vector3(px, py, pz), Vector3(vx, vy, vz), *blockState);
                }

                // 外层环：count/1.5 个粒子，半径 3.5 的均匀圆形分布
                i32 outerCount = static_cast<i32>(count / 1.5f);
                for (i32 j = 0; j < outerCount; ++j) {
                    f32 angle = static_cast<f32>(j) * math::TWO_PI / static_cast<f32>(outerCount);
                    f32 px = cx + 3.5f * std::cos(angle) + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 py = cy;
                    f32 pz = cz + 3.5f * std::sin(angle) + static_cast<f32>(random.nextGaussian()) / 2.0f;
                    f32 vx = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    f32 vy = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    f32 vz = static_cast<f32>(random.nextGaussian()) * 0.05f;
                    m_world.addBlockParticle(
                        ParticleTypeId::DustPillar, Vector3(px, py, pz), Vector3(vx, vy, vz), *blockState);
                }
            }
            break;
        }

        case WorldEvents::SHOOT_WHITE_SMOKE: {
            // 白烟粒子效果（方向性），与 DISPENSER_SMOKE(2000) 类似但为白色烟雾
            // data 为烟雾方向（Direction.getIndex()）
            {
                Direction dir = static_cast<Direction>(data);
                i32 stepX = Directions::xOffset(dir);
                i32 stepY = Directions::yOffset(dir);
                i32 stepZ = Directions::zOffset(dir);
                for (int i = 0; i < 10; ++i) {
                    f32 speed = static_cast<f32>(random.nextDouble() * 0.2 + 0.01);
                    f32 spx = static_cast<f32>(x) + static_cast<f32>(stepX) * 0.6f + 0.5f +
                        static_cast<f32>(stepX) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepZ) * 0.5f;
                    f32 spy = static_cast<f32>(y) + static_cast<f32>(stepY) * 0.6f + 0.5f +
                        static_cast<f32>(stepY) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepY) * 0.5f;
                    f32 spz = static_cast<f32>(z) + static_cast<f32>(stepZ) * 0.6f + 0.5f +
                        static_cast<f32>(stepZ) * 0.01f +
                        static_cast<f32>(random.nextFloat() - 0.5f) * static_cast<f32>(stepX) * 0.5f;
                    f32 svx = static_cast<f32>(stepX) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svy = static_cast<f32>(stepY) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    f32 svz = static_cast<f32>(stepZ) * speed + static_cast<f32>(random.nextGaussian()) * 0.01f;
                    m_world.addParticle(ParticleTypeId::WhiteSmoke, Vector3(spx, spy, spz), Vector3(svx, svy, svz));
                }
            }
            break;
        }

        case WorldEvents::PLANT_GROWTH_PARTICLES: {
            // 植物生长粒子效果（蜜蜂授粉促进作物生长时触发）
            // data 为粒子数量（通常为 15），0 则生成 15 个
            // 与 BONEMEAL_PARTICLES(2005) 的区别：不播放骨粉使用音效
            {
                i32 count = (data == 0) ? 15 : data;
                m_world.addParticle(ParticleTypeId::HappyVillager,
                    Vector3(px, py, pz),
                    Vector3(0.0f, 0.0f, 0.0f),
                    Vector3(0.5f, 1.0f, 0.5f),
                    static_cast<u32>(count));
            }
            break;
        }

        case WorldEvents::TURTLE_EGG_PLACEMENT: {
            // 海龟蛋放置粒子效果
            // 与 PLANT_GROWTH_PARTICLES(2011) 逻辑相同，均为 HappyVillager 粒子
            {
                i32 count = (data == 0) ? 15 : data;
                m_world.addParticle(ParticleTypeId::HappyVillager,
                    Vector3(px, py, pz),
                    Vector3(0.0f, 0.0f, 0.0f),
                    Vector3(0.5f, 1.0f, 0.5f),
                    static_cast<u32>(count));
            }
            break;
        }

        default:
            // 未知事件ID，忽略
            break;
    }
}

} // namespace mc::client
