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

#include "TNTBlock.hpp"

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"

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
    // 检查是否有红石信号或火焰
    bool hasPower = world::redstone::RedstonePower::isPowered(world, pos);
    bool hasFire = _hasFlammableNeighbor(world, pos);

    if (hasPower || hasFire) {
        // ignite() 返回是否成功点燃；此处不处理返回值，
        // 如果 tntExplodes 规则为 false，方块将保留在原位不点燃
        static_cast<void>(ignite(world, pos, state));
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
        // ignite() 返回是否成功点燃；此处不处理返回值，
        // 如果 tntExplodes 规则为 false，方块将保留在原位不点燃
        static_cast<void>(ignite(world, pos, *state));
        return;
    }

    // 检查是否有火焰或熔岩
    bool hasFire = _hasFlammableNeighbor(world, pos);
    if (hasFire) {
        static_cast<void>(ignite(world, pos, *state));
    }
}

bool TNTBlock::ignite(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 仅服务端执行
    if (world.isClientSide()) {
        return false;
    }

    // 检查 tntExplodes 游戏规则，如果为 false 则不点燃
    // 对应 MC Java 的 TntBlock.prime() 中的 GameRules.TNT_EXPLODES 检查
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        return false;
    }

    // 移除TNT方块
    world.setBlockState(pos, nullptr, 11);

    // 生成点燃的TNT实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* tntType = registry.getType(entity::EntityTypes::TNT);

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

void TNTBlock::onBlockExploded(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(state);

    // 对应 MC Java 的 TntBlock.wasExploded()
    // 当 TNT 方块被其他爆炸摧毁时，如果 tntExplodes 游戏规则为 true，
    // 生成一个随机短引信的点燃 TNT 实体（连锁爆炸）
    if (!world.isClientSide() && world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
        auto& registry = entity::EntityRegistry::instance();
        const entity::EntityType* tntType = registry.getType(entity::EntityTypes::TNT);

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
