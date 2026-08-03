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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "TNTBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

TNTBlock::TNTBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::UNSTABLE())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::UNSTABLE(), false));
}

bool TNTBlock::isUnstable(const BlockState& state)
{
    return state.get(BlockStateProperties::UNSTABLE());
}

void TNTBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // TNT 放置后检查是否有红石信号或火焰，如果有则自动点燃
    // 对应 MC Java 的 TntBlock.onPlace()：先 prime()，成功后移除方块
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);
    bool hasFire = _hasFlammableNeighbor(world, pos);

    if (hasPower || hasFire) {
        if (prime(world, pos, nullptr)) {
            world.setBlockState(pos, nullptr, 11);
        }
    }
}

void TNTBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查是否有红石信号
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);

    if (hasPower) {
        // 对应 MC Java 的 TntBlock.neighborChanged()：先 prime()，成功后移除方块
        if (prime(world, pos, nullptr)) {
            world.setBlockState(pos, nullptr, 11);
        }
        return;
    }

    // 检查是否有火焰或熔岩
    bool hasFire = _hasFlammableNeighbor(world, pos);
    if (hasFire) {
        if (prime(world, pos, nullptr)) {
            world.setBlockState(pos, nullptr, 11);
        }
    }
}

BlockActionResult TNTBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    ItemStack& heldItem = player.getHeldItem(hand);
    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    bool isFlintAndSteel = (item == Items::FLINT_AND_STEEL);
    bool isFireCharge = (item == Items::FIRE_CHARGE);

    if (!isFlintAndSteel && !isFireCharge) {
        return ActionResultType::Pass;
    }

    // 对应 MC Java 的 TntBlock.useItemOn()
    // 先 prime()，成功后移除方块并消耗物品
    if (prime(world, pos, &player)) {
        // 移除TNT方块
        world.setBlockState(pos, nullptr, 11);

        // 消耗物品
        if (isFlintAndSteel) {
            // 打火石：消耗耐久度
            LivingEntity::hurtAndBreak(heldItem, 1, &player, LivingEntity::handToEquipmentSlot(hand));
        } else {
            // 火焰弹：消耗一个物品（创造模式不消耗）
            if (!player.isCreative()) {
                heldItem.shrink(1);
            }
        }

        // 对应 MC Java 的 player.awardStat(Stats.ITEM_USED.get(item))
        player.awardUsedStat(item->itemLocation(), 1);
    } else if (!world.isClientSide() && !world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        // tntExplodes 游戏规则为 false，不消耗物品，显示 action bar 消息
        // 对应 MC Java: return InteractionResult.PASS（不消耗物品，将交互传递给下一个处理器）
        player.sendStatusMessage("block.minecraft.tnt.disabled", true);
        return ActionResultType::Pass;
    }

    return ActionResultType::Success;
}

void TNTBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    // 对应 MC Java 的 TntBlock.playerWillDestroy()
    // 当玩家破坏不稳定的TNT（UNSTABLE=true）且不在创造模式下时，自动点燃TNT
    // 注意：此处只调用 prime()，不移除方块——方块移除由破坏流程处理
    if (!world.isClientSide() && !player.isCreative() && isUnstable(state)) {
        static_cast<void>(prime(world, pos, nullptr));
    }
}

void TNTBlock::onProjectileHit(
    IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile)
{
    MC_UNUSED(state);

    // 对应 MC Java 的 TntBlock.onProjectileHit()
    // 当燃烧的投掷物命中TNT时，点燃TNT
    if (!world.isClientSide() && projectile.isOnFire()) {
        // 对应 MC Java 的 projectile.mayInteract(serverlevel, blockpos)
        // 检查投掷物是否可以在该位置交互（冒险模式权限判断）
        if (!projectile.mayInteract(world, hitResult.blockPos())) {
            return;
        }

        // 获取投掷物的发射者作为点燃者
        LivingEntity* igniter = nullptr;
        // 尝试从 ProjectileEntity 获取发射者
        auto* projEntity = dynamic_cast<entity::ProjectileEntity*>(&projectile);
        if (projEntity != nullptr) {
            Entity* shooter = projEntity->getShooter();
            igniter = dynamic_cast<LivingEntity*>(shooter);
        }

        // prime() 成功后移除TNT方块
        if (prime(world, hitResult.blockPos(), igniter)) {
            world.setBlockState(hitResult.blockPos(), nullptr, 11);
        }
    }
}

bool TNTBlock::ignite(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    return ignite(world, pos, state, nullptr);
}

bool TNTBlock::ignite(IWorld& world, const BlockPos& pos, const BlockState& state, LivingEntity* igniter)
{
    MC_UNUSED(state);

    // 先尝试 prime（生成实体+音效），成功后移除方块
    if (prime(world, pos, igniter)) {
        world.setBlockState(pos, nullptr, 11);
        return true;
    }
    return false;
}

bool TNTBlock::prime(IWorld& world, const BlockPos& pos, LivingEntity* igniter)
{
    // 仅服务端执行
    if (world.isClientSide()) {
        return false;
    }

    // 检查 tntExplodes 游戏规则，如果为 false 则不点燃
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        return false;
    }

    // 生成点燃的TNT实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* tntType = registry.getType(entity::EntityTypeKeys::TNT);

    if (tntType != nullptr && tntType->isValid()) {
        auto tntEntity = tntType->create(&world);
        if (tntEntity != nullptr) {
            // 设置TNT位置（方块中心）
            f32 centerX = static_cast<f32>(pos.x) + 0.5f;
            f32 centerY = static_cast<f32>(pos.y);
            f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

            // 使用动态转换获取 TNTEntity
            auto* tnt = dynamic_cast<entity::TNTEntity*>(tntEntity.get());
            if (tnt != nullptr) {
                // 设置位置
                tnt->setPosition(centerX, centerY, centerZ);

                // 设置随机初始速度
                math::Random& rng = world.getRandom();
                f32 angle = rng.nextFloat() * math::TWO_PI;
                f32 vx = -std::sin(angle) * 0.02f;
                f32 vy = 0.2f;
                f32 vz = -std::cos(angle) * 0.02f;
                tnt->setVelocity(Vector3(vx, vy, vz));

                // 设置点燃者
                if (igniter != nullptr) {
                    tnt->setOwner(igniter);
                }

                // 点燃TNT（设置引信时间）
                tnt->ignite();
            }

            // 生成实体
            world.spawnEntity(std::move(tntEntity));
        }
    }

    // 播放点燃音效
    world.playSound(SoundEvents::ENTITY_TNT_PRIMED,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);

    // 发出 PRIME_FUSE 游戏事件
    world.gameEvent(
        gameevent::GameEvents::PRIME_FUSE, pos, gameevent::GameEvent::Context::of(static_cast<const Entity*>(igniter)));

    return true;
}

void TNTBlock::explode(IWorld& world, const BlockPos& pos, f32 power)
{
    // 检查 tntExplodes 游戏规则
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        // 即使不允许爆炸，也要移除方块（与 MC Java 行为一致）
        world.setBlockState(pos, nullptr, 11);
        return;
    }

    // 移除TNT方块
    world.setBlockState(pos, nullptr, 11);

    // 创建爆炸
    world.createExplosion(
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.0625f, static_cast<f32>(pos.z) + 0.5f),
        power,
        world::explosion::ExplosionMode::Break,
        false,  // 不生成火焰
        nullptr // 无爆炸源实体
    );
}

void TNTBlock::onBlockExploded(
    IWorld& world, const BlockPos& pos, const BlockState& state, const world::explosion::Explosion* explosion) const
{
    MC_UNUSED(state);

    // 当 TNT 方块被其他爆炸摧毁时，如果 tntExplodes 游戏规则为 true，
    // 生成一个随机短引信的点燃 TNT 实体（连锁爆炸）
    // 对应 MC Java 的 TntBlock.wasExploded()
    if (!world.isClientSide() && world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* tntType = registry.getType(entity::EntityTypeKeys::TNT);

        if (tntType != nullptr && tntType->isValid()) {
            auto tntEntity = tntType->create(&world);
            if (tntEntity != nullptr) {
                // 设置TNT位置（方块中心）
                f32 centerX = static_cast<f32>(pos.x) + 0.5f;
                f32 centerY = static_cast<f32>(pos.y);
                f32 centerZ = static_cast<f32>(pos.z) + 0.5f;

                auto* tnt = dynamic_cast<entity::TNTEntity*>(tntEntity.get());
                if (tnt != nullptr) {
                    tnt->setPosition(centerX, centerY, centerZ);

                    // 设置随机短引信：MC Java 的公式为 random.nextInt(fuse / 4) + fuse / 8
                    // 其中 fuse = 80 (DEFAULT_FUSE)，即 random.nextInt(20) + 10，范围 [10, 29] ticks
                    math::Random& rng = world.getRandom();
                    constexpr i32 DEFAULT_FUSE = 80;
                    i32 shortFuse = rng.nextInt(DEFAULT_FUSE / 4) + DEFAULT_FUSE / 8;
                    tnt->ignite(shortFuse);

                    // 对应 MC Java 的 explosion.getIndirectSourceEntity()
                    // 获取爆炸的间接源实体，作为连锁 TNT 的 owner
                    if (explosion != nullptr) {
                        LivingEntity* indirectSource = explosion->getIndirectSourceEntity();
                        if (indirectSource != nullptr) {
                            tnt->setOwner(indirectSource);
                        }
                    }
                }

                world.spawnEntity(std::move(tntEntity));
            }
        }
    }
}

bool TNTBlock::_hasFlammableNeighbor(IWorld& world, const BlockPos& pos) const
{
    // 检查六个方向是否有火焰或熔岩
    for (Direction dir :
        {Direction::North, Direction::East, Direction::South, Direction::West, Direction::Up, Direction::Down}) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState != nullptr) {
            // 检查是否是火焰（包括灵魂火）
            if (neighborState->is(VanillaBlocks::FIRE) || neighborState->is(VanillaBlocks::SOUL_FIRE)) {
                return true;
            }
            // 检查是否是熔岩
            if (neighborState->is(VanillaBlocks::LAVA)) {
                return true;
            }
        }
    }

    return false;
}

} // namespace blocks
} // namespace mc
