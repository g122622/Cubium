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

#include "TurtleEggBlock.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../entity/core/EntityTypeIdNumber.hpp"
#include "../../../../entity/entities/passive/special/TurtleEntity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../gamerule/GameRules.hpp"
#include "../../../lighting/InternalLightUtils.hpp"
#include "../../BlockRegistry.hpp"

namespace mc {
namespace blocks {

TurtleEggBlock::TurtleEggBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
                         .add(BlockStateProperties::EGGS_1_4())
                         .add(BlockStateProperties::HATCH_0_2())
                         .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                             return std::make_unique<BlockState>(block, std::move(values), id);
                         });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(
        defaultState().with(BlockStateProperties::EGGS_1_4(), 1).with(BlockStateProperties::HATCH_0_2(), 0));

    // 创建各蛋数量的形状 (MC 1.16.5: box(3, 0, 3, 12, 7, 12) for 1 egg, box(1, 0, 1, 15, 7, 15) for 4)
    // 1个蛋: 3/16=0.1875, 12/16=0.75, 7/16=0.4375
    // MC实际形状: 1 egg: (3, 0, 3, 12, 7, 12), 2 eggs: (1, 0, 3, 15, 7, 12), etc
    m_shapesByEggCount[0] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.75f, 0.4375f, 0.75f);     // 1 egg
    m_shapesByEggCount[1] = CollisionShape::box(0.0625f, 0.0f, 0.1875f, 0.9375f, 0.4375f, 0.75f);   // 2 eggs
    m_shapesByEggCount[2] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 3 eggs
    m_shapesByEggCount[3] = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.4375f, 0.9375f); // 4 eggs
}

i32 TurtleEggBlock::getEggs(const BlockState& state) const
{
    return state.get(BlockStateProperties::EGGS_1_4());
}

BlockState TurtleEggBlock::withEggs(i32 count) const
{
    return defaultState().with(BlockStateProperties::EGGS_1_4(), std::clamp(count, 1, 4));
}

i32 TurtleEggBlock::getHatch(const BlockState& state) const
{
    return state.get(BlockStateProperties::HATCH_0_2());
}

BlockState TurtleEggBlock::withHatch(i32 hatch) const
{
    return defaultState().with(BlockStateProperties::HATCH_0_2(), std::clamp(hatch, 0, 2));
}

BlockState TurtleEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // MC 1.16.5: 如果放置在已有的海龟蛋上，增加蛋数量
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentEggs = existingState->get(BlockStateProperties::EGGS_1_4());
        if (currentEggs < 4) {
            return existingState->with(BlockStateProperties::EGGS_1_4(), currentEggs + 1);
        }
        return *existingState;
    }

    return defaultState();
}

bool TurtleEggBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // MC 1.16.5: 海龟蛋只能放在沙子类方块上
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 检查是否为沙子类方块 (沙子、红沙、灵魂沙)
    return BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::canGrow(IWorld& world, math::IRandom& random) const
{
    // MC 1.16.5 TurtleEggBlock.canGrow():
    // float f = world.func_242415_f(1.0F);  // getCelestialAngle
    // if ((double)f < 0.69D && (double)f > 0.65D) {
    //     return true;  // 黎明时分（天体角度 0.65-0.69）
    // } else {
    //     return world.rand.nextInt(500) == 0;  // 其他时间 1/500 概率
    // }
    //
    // MC 原版天体角度：
    // - 0.0 = 正午
    // - 0.25 = 日落
    // - 0.5 = 午夜
    // - 0.75 = 日出
    //
    // 天体角度 0.65-0.69 对应黎明时分（约 dayTime 22000-22600）
    // 这是海龟蛋孵化的最佳时间
    f32 celestialAngle = InternalLightUtils::getCelestialAngleMC(world.dayTime());

    if (celestialAngle < 0.69 && celestialAngle > 0.65) {
        // 黎明时分，100% 孵化
        return true;
    } else {
        // 其他时间，1/500 随机概率
        return random.nextInt(500) == 0;
    }
}

void TurtleEggBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // MC 1.16.5: 孵化逻辑
    // 检查是否在沙子上
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    if (!hasProperHabitat(blockReader, pos)) {
        return;
    }

    // 检查孵化条件
    if (!canGrow(world, random)) {
        return;
    }

    i32 hatch = getHatch(state);
    if (hatch < 2) {
        // 孵化进度增加
        // MC 1.16.5: 播放裂开音效
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::ENTITY_TURTLE_EGG_CRACK,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f);
        }
        const BlockState& newState = state.with(BlockStateProperties::HATCH_0_2(), hatch + 1);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 孵化完成，生成海龟
        // MC 1.16.5: 播放孵化音效
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::ENTITY_TURTLE_EGG_HATCH,
                sound::SoundCategory::Blocks,
                pos.center(),
                0.7f,
                0.9f + random.nextFloat() * 0.2f);
        }
        i32 eggs = getEggs(state);

        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }

        // MC 1.16.5: 为每个蛋生成一只小海龟
        // 参考: TurtleEggBlock.randomTick
        // for(int j = 0; j < state.get(EGGS); ++j) {
        //     TurtleEntity turtleentity = EntityType.TURTLE.create(worldIn);
        //     turtleentity.setGrowingAge(-24000);
        //     turtleentity.setHome(pos);
        //     turtleentity.setLocationAndAngles(
        //         (double)pos.getX() + 0.3D + (double)j * 0.2D,
        //         (double)pos.getY(),
        //         (double)pos.getZ() + 0.3D,
        //         0.0F, 0.0F);
        //     worldIn.addEntity(turtleentity);
        // }
        for (i32 i = 0; i < eggs; ++i) {
            auto turtle = std::make_unique<TurtleEntity>(EntityId(0));
            if (turtle) {
                // MC 1.16.5: 设置为幼体（-24000 ticks = 20分钟）
                turtle->setChild(true);

                // MC 1.16.5: 设置出生位置（小海龟会记住这个位置作为"家"）
                turtle->setHomePos(pos);

                // 设置位置：多个蛋时错开位置
                // x = pos.x + 0.3 + i * 0.2
                // z = pos.z + 0.3
                turtle->setPosition(static_cast<f32>(pos.x) + 0.3f + static_cast<f32>(i) * 0.2f,
                    static_cast<f32>(pos.y),
                    static_cast<f32>(pos.z) + 0.3f);
                turtle->setRotation(0.0f, 0.0f);

                // 生成到世界
                world.spawnEntity(std::move(turtle));
            }
        }
    }
}

void TurtleEggBlock::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // MC 1.16.5: 实体走过时尝试踩破蛋
    tryTrample(world, pos, state, entity, 100);
}

void TurtleEggBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{

    // MC 1.16.5: 实体摔落时尝试踩破蛋
    // 僵尸类生物不会踩破蛋（它们会直接走过去）
    MC_UNUSED(fallDistance);

    // 检查是否为僵尸类（僵尸、尸壳、溺尸等）
    // 僵尸类不会踩破蛋
    if (isZombieType(entity)) {
        return;
    }

    tryTrample(world, pos, state, entity, 3);
}

bool TurtleEggBlock::hasProperHabitat(IBlockReader& world, const BlockPos& pos) const
{
    // MC 1.16.5: 检查下方是否为沙子
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    return belowState != nullptr && BlockTags::SAND().contains(*belowState);
}

bool TurtleEggBlock::canTrample(IWorld& world, Entity& entity) const
{
    // MC 1.16.5: 只有玩家或满足 mobGriefing 的生物才能踩破蛋
    // 海龟和蝙蝠不能踩破蛋

    // MC 1.16.5 TurtleEggBlock.canTrample():
    // if (!(trampler instanceof TurtleEntity) && !(trampler instanceof BatEntity)) {
    //     if (!(trampler instanceof LivingEntity)) {
    //         return false;
    //     } else {
    //         return trampler instanceof PlayerEntity ||
    //         net.minecraftforge.event.ForgeEventFactory.getMobGriefingEvent(worldIn, trampler);
    //     }
    // } else {
    //     return false;
    // }

    // 获取实体类型
    entity::EntityTypeId type = entity.typeId();

    // 海龟和蝙蝠不能踩破蛋
    if (type == entity::EntityTypeIdNumber::TURTLE || type == entity::EntityTypeIdNumber::BAT) {
        return false;
    }

    // 非生物实体不能踩破（物品、箭矢等）
    // 检查是否为生物实体：玩家和怪物类实体可以踩破
    // 使用动态类型检查判断是否为 LivingEntity
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return false;
    }

    // 玩家总是可以踩破
    if (type == entity::EntityTypeIdNumber::PLAYER) {
        return true;
    }

    // 检查 mobGriefing 游戏规则
    return world.getGameRules().getBoolean(mc::world::gamerule::GameRuleKeys::MOB_GRIEFING);
}

void TurtleEggBlock::tryTrample(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, i32 chance) const
{
    // MC 1.16.5: 尝试踩破蛋
    if (!canTrample(world, entity)) {
        return;
    }

    // 随机检查
    if (world.getRandom().nextInt(chance) != 0) {
        return;
    }

    // 踩破一个蛋
    removeOneEgg(world, pos, state);
}

void TurtleEggBlock::removeOneEgg(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    // MC 1.16.5: 移除一个蛋
    // 播放破碎音效
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::ENTITY_TURTLE_EGG_BREAK,
            sound::SoundCategory::Blocks,
            pos.center(),
            0.7f,
            0.9f + world.getRandom().nextFloat() * 0.2f);
    }

    i32 eggs = getEggs(state);
    if (eggs > 1) {
        // 减少蛋数量，重置孵化进度
        const BlockState& newState =
            state.with(BlockStateProperties::EGGS_1_4(), eggs - 1).with(BlockStateProperties::HATCH_0_2(), 0);
        world.setBlockState(pos, &newState, 2);
    } else {
        // 移除方块
        const BlockState* airState = BlockRegistry::instance().airState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState, 2);
        }
    }
}

bool TurtleEggBlock::isZombieType(Entity& entity) const
{
    // MC 1.16.5: 检查实体是否为僵尸类
    // 使用 instanceof ZombieEntity 检查，由于 ZombieEntity 是基类，
    // HuskEntity、DrownedEntity 等是子类
    // 但在当前项目中，这些是独立的实体类型，需要通过 typeId 检查

    entity::EntityTypeId type = entity.typeId();

    // MC 1.16.5: 只有 ZombieEntity 及其子类（Husk、Drowned）会踩破海龟蛋
    // 注意：MC 中 ZombieVillager 也是 ZombieEntity 的子类，但当前项目中未定义
    // Skeleton、Stray、WitherSkeleton 虽然是亡灵，但不是僵尸类，不会踩破蛋
    if (type == entity::EntityTypeIdNumber::ZOMBIE ||
        type == entity::EntityTypeIdNumber::HUSK ||
        type == entity::EntityTypeIdNumber::DROWNED) {
        // 僵尸及其变种会踩破海龟蛋
        return true;
    }
    return false;
}

const CollisionShape& TurtleEggBlock::getShape(const BlockState& state) const
{
    i32 eggs = getEggs(state);
    return m_shapesByEggCount[static_cast<std::size_t>(std::min(eggs - 1, 3))];
}

} // namespace blocks
} // namespace mc
