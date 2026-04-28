#include "BoneMealItem.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/IGrowable.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {
namespace item::items {

BoneMealItem::BoneMealItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

ActionResultType BoneMealItem::onItemUse(ItemUseContext& context) {
    const BlockPos& pos = context.blockPos();
    IWorld& world = const_cast<IWorld&>(context.world());

    // 尝试应用骨粉
    if (applyBonemeal(const_cast<ItemStack&>(context.itemStack()), world, pos, context.player())) {
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

bool BoneMealItem::applyBonemeal(ItemStack& stack, IWorld& world, const BlockPos& pos, Player* player) {
    MC_UNUSED(player);

    // 获取方块状态
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return false;
    }

    const BlockState& state = *statePtr;
    const Block& block = state.owner();

    // 检查方块是否实现 IGrowable 接口
    // 注意：grow() 是非 const 方法，所以不能用 const 指针
    IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&block));
    if (growable == nullptr) {
        return false;
    }

    // 检查是否可以生长
    // IGrowable::canGrow 需要 IBlockReader，需要从 IWorld 转换
    if (!growable->canGrow(static_cast<IBlockReader&>(world), pos, state, false)) {
        return false;
    }

    // 检查是否可以使用骨粉
    // 使用世界种子和位置派生随机数，确保确定性
    const u64 seed = world.seed() ^ static_cast<u64>(std::hash<BlockPos>{}(pos));
    math::Random random(seed);

    if (growable->canUseBonemeal(world, random, pos, state)) {
        // 执行生长
        growable->grow(world, random, pos, state);

        // 减少物品数量（非创造模式）
        if (stack.getCount() > 0) {
            stack.shrink(1);
        }

        // 生成快乐村民粒子
        spawnBonemealParticles(world, pos);

        return true;
    }

    return false;
}

bool BoneMealItem::growSeagrass(IWorld& world, const BlockPos& pos, math::IRandom& random) {
    // 参考: net.minecraft.item.BoneMealItem#growSeagrass
    // 在水下生成海草的逻辑

    // 检查是否为水
    if (!world.isWaterAt(pos)) {
        return false;
    }

    // 检查下方是否有支撑
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    // TODO: 实现海草生成逻辑
    // 需要:
    // 1. 检查是否可以在该位置生成海草
    // 2. 随机选择海草类型（普通海草或海带）
    // 3. 设置海草方块状态

    MC_UNUSED(random);
    return false;
}

void BoneMealItem::spawnBonemealParticles(IWorld& world, const BlockPos& pos) {
    // 在方块周围生成快乐村民粒子
    // 粒子在方块上方随机分布

    constexpr f32 offsetX = 0.5f;
    constexpr f32 offsetY = 0.5f;
    constexpr f32 offsetZ = 0.5f;

    // 生成 15 个粒子
    // 参考 MC 1.16.5: BoneMealItem 生成 15 个 happy_villager 粒子
    constexpr u32 particleCount = 15;

    world.addParticle(
        client::renderer::trident::particle::ParticleTypeId::HappyVillager,
        Vector3(static_cast<f32>(pos.x) + offsetX,
                static_cast<f32>(pos.y) + offsetY,
                static_cast<f32>(pos.z) + offsetZ),
        Vector3(0.0f, 0.0f, 0.0f),  // 速度为0
        Vector3(1.0f, 1.0f, 1.0f),   // 偏移范围
        particleCount
    );
}

} // namespace item::items
} // namespace mc
