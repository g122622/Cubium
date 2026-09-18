/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/play/EntityActionHandler.hpp"

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

void EntityActionHandler::handleInteractPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // Interact action：0=INTERACT 1=ATTACK 2=INTERACT_AT。
    // 对齐 Java ServerGamePacketListenerImpl.handleInteract：世界边界 → AABB 距离 →
    // ATTACK 实体黑名单 → 交互成功触发 player_interacted_with_entity 成就 + 挥手。
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Interact>(&play);
    if (evt == nullptr) {
        return;
    }

    auto* world = m_server.getPlayerWorld(playerId);
    if (world == nullptr) {
        return;
    }

    auto* playerEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *world);
    if (playerEntity == nullptr) {
        return;
    }

    auto* target = world->getEntity(static_cast<EntityInstanceId>(evt->entityId));
    if (target == nullptr) {
        return;
    }

    // 副操作（潜行）状态同步：vanilla player.setShiftKeyDown(usingSecondaryAction)。
    playerEntity->setSneaking(evt->usingSecondaryAction);

    // 世界边界校验：目标方块位越界直接拒绝。
    if (!world->worldBorder().contains(target->onPos())) {
        return;
    }

    // 距离校验：对齐 vanilla ServerGamePacketListenerImpl.handleInteract
    // (ServerGamePacketListenerImpl.java:729)：player.isWithinEntityInteractionRange(entity, 3.0)。
    // isWithinEntityInteractionRange 内部按 entityInteractionRange() 属性 + padding 计算
    // （生存 3.0、创造 5.0，padding 3.0 为容差），而非此处原先硬编码的 3.0+3.0=6.0。
    // 这样 generic.entity_interaction_range 属性（含创造模式 +2.0 修饰符）才真正生效。
    // ATTACK 走同样阈值（vanilla ATTACK 用 isWithinAttackRange，项目暂无 attack range
    // attribute，统一用交互阈值——TODO: 待引入 entity_attack_range 属性后分离）。
    if (!playerEntity->isWithinEntityInteractionRange(*target, 3.0)) {
        return;
    }

    const Hand hand = (evt->hand == static_cast<i32>(Hand::OffHand)) ? Hand::OffHand : Hand::MainHand;

    switch (evt->action) {
        case 0: { // INTERACT
            const ItemStack itemBefore = playerEntity->getHeldItem(hand);
            const ActionResultType result = playerEntity->interactOn(*target, hand);
            if (result == ActionResultType::Success) {
                _triggerPlayerInteractedWithEntity(*playerEntity, itemBefore, *target);
                playerEntity->swing(hand);
            }
            break;
        }
        case 1: { // ATTACK
            // 实体黑名单：掉落物/经验球/自身/箭矢一律不可攻击。
            const auto* type = target->entityType();
            const bool blacklisted = type == entity::VanillaEntityTypeKeys::ITEM ||
                type == entity::VanillaEntityTypeKeys::EXPERIENCE_ORB || type == entity::VanillaEntityTypeKeys::ARROW ||
                type == entity::VanillaEntityTypeKeys::SPECTRAL_ARROW || target == playerEntity;
            if (blacklisted) {
                spdlog::warn("Interact: player {} tried to attack invalid entity", playerId);
                break;
            }
            playerEntity->attack(*target);
            break;
        }
        case 2: { // INTERACT_AT（带命中点）
            const Vector3 hitPosition(evt->hitX, evt->hitY, evt->hitZ);
            const ItemStack itemBefore = playerEntity->getHeldItem(hand);
            const ActionResultType result = target->applyPlayerInteraction(*playerEntity, hitPosition, hand);
            if (result == ActionResultType::Success) {
                _triggerPlayerInteractedWithEntity(*playerEntity, itemBefore, *target);
                playerEntity->swing(hand);
            }
            break;
        }
        default:
            spdlog::info("Interact: player {} sent unknown action {}", playerId, evt->action);
            break;
    }
}

void EntityActionHandler::_triggerPlayerInteractedWithEntity(Player& player, const ItemStack& item, Entity& entity)
{
    // 触发 player_interacted_with_entity 成就。对齐 vanilla
    // CriteriaTriggers.PLAYER_INTERACTED_WITH_ENTITY.trigger(player, item, entity)。
    // PlayerInteractedWithEntityTrigger::trigger 在 common 层为空桩，须按
    // AdvancementEventHandler 既定模式直接调基类 trigger 模板。
    auto* serverPlayer = player.asServerPlayer();
    if (serverPlayer == nullptr) {
        return;
    }
    auto* advancements = serverPlayer->getAdvancements();
    if (advancements == nullptr) {
        return;
    }
    auto* trigger =
        mc::advancement::CriterionTriggers::instance().getTrigger<mc::advancement::PlayerInteractedWithEntityTrigger>();
    if (trigger == nullptr) {
        return;
    }
    trigger->AbstractCriterionTrigger<mc::advancement::PlayerInteractedWithEntityTriggerInstance>::trigger(
        *advancements, [&item, &entity](const mc::advancement::PlayerInteractedWithEntityTriggerInstance& instance) {
            return instance.test(item, entity);
        });
}

} // namespace mc::server::net
