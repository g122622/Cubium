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
#include "../../../../entity/core/EntityRegistry.hpp"
#include "../../../../entity/entities/misc/MiscEntities.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../explosion/Explosion.hpp"
#include "../../../explosion/ExplosionMode.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../VanillaBlocks.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

TNTBlock::TNTBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::UNSTABLE())
                         .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), id);
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
    bool hasFire = hasFlammableNeighbor(world, pos);

    if (hasPower || hasFire) {
        ignite(world, pos, state);
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
        ignite(world, pos, *state);
        return;
    }

    // 检查是否有火焰或熔岩
    bool hasFire = hasFlammableNeighbor(world, pos);
    if (hasFire) {
        ignite(world, pos, *state);
    }
}

void TNTBlock::ignite(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 仅服务端执行
    if (world.isClientSide()) {
        return;
    }

    // 移除TNT方块
    world.setBlockState(pos, nullptr, 11);

    // 生成点燃的TNT实体
    // 参考 MC 1.16.5 TNTBlock.onCaughtFire
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
                // MC 1.16.5: 随机水平方向 + 向上 0.2
                math::Random& rng = world.getRandom();
                f32 angle = rng.nextFloat() * static_cast<f32>(math::TWO_PI);
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

    // MC 1.16.5: 播放点燃音效
    world.playSound(SoundEvents::ENTITY_TNT_PRIMED,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

void TNTBlock::explode(IWorld& world, const BlockPos& pos, f32 power)
{
    // 移除TNT方块
    world.setBlockState(pos, nullptr, 11);

    // 创建爆炸
    // 参考 MC 1.16.5: world.createExplosion(pos, power, Explosion.Mode.BREAK)
    world.createExplosion(
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.0625f, static_cast<f32>(pos.z) + 0.5f),
        power,
        world::explosion::ExplosionMode::Break,
        false,  // 不生成火焰
        nullptr // 无爆炸源实体
    );
}

bool TNTBlock::hasFlammableNeighbor(IWorld& world, const BlockPos& pos) const
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
