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

#include "common/entity/inventory/container/MerchantContainerMenu.hpp"
#include "common/network/buffer/ByteBuf.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "server/network/handshake/RegistryDataBuilder.hpp"
#include <array>

#include "server/network/outbound/PacketBuilders.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/Vector3.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/network/outbound/CommandTreeEncoder.hpp"
#include <optional>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server::net {

mc::network::ir::IrPacket buildPlaySoundIr(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    // 1.21.11 PlaySound：Holder<SoundEvent>(结构化内联) + source + 坐标×8 + volume + pitch + seed。
    // soundHolder 用内联 SoundEvent（direct=true，identifier=soundEventId），对齐 vanilla wire。
    mc::network::ir::play::PlaySound pkt;
    pkt.soundHolder.direct = true;
    pkt.soundHolder.identifier = soundEventId.toString();
    pkt.soundHolder.hasFixedRange = false;
    pkt.source = static_cast<i32>(category);
    pkt.x = static_cast<i32>(position.x * 8.0f);
    pkt.y = static_cast<i32>(position.y * 8.0f);
    pkt.z = static_cast<i32>(position.z * 8.0f);
    pkt.volume = volume;
    pkt.pitch = pitch;
    pkt.seed = 0;
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

mc::network::ir::IrPacket buildPermissionLevelChangeIr(i32 entityId, i32 permissionLevel)
{
    // 1.21.11 权限等级走 EntityEvent（OP_PERMISSION_LEVEL_0..3 = 24..27）。
    mc::network::ir::play::EntityEvent pkt;
    pkt.entityId = entityId;
    pkt.eventId = static_cast<u8>(24 + permissionLevel);
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

std::optional<mc::network::ir::IrPacket> buildCommandsIr(mc::command::CommandRegistry& registry)
{
    // 1.21.11 ClientboundCommandsPacket：二进制 CommandNode 树。对齐 Java 线格式
    // （VarInt(nodeCount) + nodes + VarInt(rootIndex)，每节点 flags/children/redirect/stub）。
    mc::network::ir::play::Commands pkt;
    auto snapshot = registry.getCommandTreeSnapshot();
    auto encoded = mc::server::net::encodeCommandTree(snapshot);
    if (!encoded.success()) {
        spdlog::error("CommandTreeEncoder: failed to encode command tree: {}", encoded.error().toString());
        return std::nullopt;
    }
    pkt.payload = std::move(encoded.value());
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

mc::network::ir::IrPacket buildLevelParticlesIr(
    const mc::network::ir::play::ParticleOptions& options, const Vector3& pos, const Vector3& offset, u32 count)
{
    // 1.21.11 LevelParticles 外层字段组装。maxSpeed 沿用旧实现固定 0
    // （客户端按 offset 扇出，不消费该字段）。
    mc::network::ir::play::LevelParticles pkt;
    pkt.overrideLimiter = false;
    pkt.alwaysShow = false;
    pkt.x = pos.x;
    pkt.y = pos.y;
    pkt.z = pos.z;
    pkt.xDist = offset.x;
    pkt.yDist = offset.y;
    pkt.zDist = offset.z;
    pkt.maxSpeed = 0.0f;
    pkt.count = static_cast<i32>(count);
    pkt.particle = options;
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
}

mc::network::ir::IrPacket buildLevelParticlesIr(particle::ParticleTypeId type, const Vector3& pos, u32 count)
{
    // 简单粒子（SimpleParticleType）无附加选项：只填 type，偏移归零。
    mc::network::ir::play::ParticleOptions options;
    options.type = type;
    return buildLevelParticlesIr(options, pos, Vector3(0.0f, 0.0f, 0.0f), count);
}

namespace {

Result<mc::network::ir::play::TradeItemCost> _tradeCost(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "Merchant offer has an empty cost");
    }
    const auto view = mc::network::ir::toItemStackView(stack);
    mc::network::buffer::ByteBuf predicate;
    if (view.componentsPatch.empty()) {
        predicate.writeVarInt(0);
    } else {
        mc::network::buffer::ByteBuf patch(view.componentsPatch.data(), view.componentsPatch.size());
        i32 added = 0;
        i32 removed = 0;
        MC_TRY_ASSIGN(added, patch.readVarInt());
        MC_TRY_ASSIGN(removed, patch.readVarInt());
        if (removed != 0) {
            return Error(ErrorCode::InvalidArgument, "Trade costs cannot require absent components");
        }
        predicate.writeVarInt(added);
        predicate.writeBytes(patch.data() + patch.readPosition(), patch.readableBytes());
    }
    return mc::network::ir::play::TradeItemCost{view.itemId, view.count, predicate.bytes()};
}

} // namespace

Result<mc::network::ir::IrPacket> buildMerchantOffersIr(const MerchantContainerMenu& menu)
{
    mc::network::ir::play::MerchantOffersUpdate packet{};
    packet.containerId = static_cast<i32>(menu.getId());
    packet.level = menu.getTraderLevel();
    packet.xp = menu.getTraderXp();
    packet.showProgress = menu.showProgressBar();
    packet.canRestock = menu.canRestock();
    const auto& offers = menu.getOffers();
    packet.offers.reserve(offers.size());
    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto& offer = *offers.getOffer(static_cast<i32>(i));
        mc::network::ir::play::TradeOfferView view{};
        MC_TRY_ASSIGN(view.costA, _tradeCost(offer.getBuyA()));
        if (offer.getBuyB().has_value()) {
            mc::network::ir::play::TradeItemCost cost{};
            MC_TRY_ASSIGN(cost, _tradeCost(*offer.getBuyB()));
            view.costB = std::move(cost);
        }
        view.result = mc::network::ir::toItemStackView(offer.getSell());
        if (view.result.count <= 0) {
            return Error(ErrorCode::InvalidArgument, "Merchant offer has an empty result");
        }
        view.outOfStock = offer.isOutOfStock();
        view.uses = offer.getUses();
        view.maxUses = offer.getMaxUses();
        view.xp = offer.getXp();
        view.specialPrice = offer.getSpecialPrice();
        view.priceMultiplier = offer.getPriceMultiplier();
        view.demand = offer.getDemand();
        packet.offers.push_back(std::move(view));
    }
    return mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(packet)}};
}

Result<mc::network::ir::IrPacket> buildShowDialogIr(const nlohmann::json& definition)
{
    if (!definition.is_object() || !definition.contains("title") || !definition.contains("type") ||
        !definition["type"].is_string())
        return Error(ErrorCode::InvalidData, "Dialog requires type and title");
    auto tag = mc::nbt::jsonToNbtStrictInt(definition);
    const auto& compound = dynamic_cast<const mc::nbt::tags::compound_tag&>(*tag);
    return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{
            mc::network::ir::play::ShowDialog{mc::network::buffer::nbt_io::serializeRootCompoundToBytes(compound)}}};
}

Result<mc::network::ir::IrPacket> buildShowDialogIr(const ResourceLocation& name)
{
    constexpr std::array<std::string_view, 3> names{
        "minecraft:server_links", "minecraft:custom_options", "minecraft:quick_actions"};
    for (i32 i = 0; i < static_cast<i32>(names.size()); ++i)
        if (name.toString() == names[i])
            return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
                mc::network::ir::PlayPacket{mc::network::ir::play::ShowDialog{i}}};
    auto definition = loadDialogDefinition(name.toString());
    if (definition.failed()) return definition.error();
    return buildShowDialogIr(definition.value());
}

} // namespace mc::server::net
