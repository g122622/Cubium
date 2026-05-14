#include "InfestedBlock.hpp"
#include "../../../../entity/entities/monster/arthropod/EndermiteEntity.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

InfestedBlock::InfestedBlock(u32 hostBlock, const BlockProperties& properties)
    : Block(properties)
    , m_hostBlock(hostBlock)
{
    // 被感染方块没有特殊状态
}

void InfestedBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // MC 1.16.5: InfestedBlock.spawnAdditionalDrops()
    // 当被破坏时，有概率生成蠹虫
    // 注意：实际生成条件需要检查游戏规则 doTileDrops 和精准采集附魔
    // 这些检查在 onBlockHarvested 或 spawnAdditionalDrops 中进行
    // 这里简化处理：直接生成蠹虫

    MC_UNUSED(state);

    // 只在服务端生成
    if (world.isClientSide()) {
        return;
    }

    // MC 1.16.5: 创建蠹虫实体
    // SilverfishEntity silverfishentity = EntityType.SILVERFISH.create(world);
    // silverfishentity.setLocationAndAngles(pos.getX() + 0.5D, pos.getY(), pos.getZ() + 0.5D, 0.0F, 0.0F);
    // world.addEntity(silverfishentity);
    // silverfishentity.spawnExplosionParticle();

    auto silverfish = std::make_unique<SilverfishEntity>(LegacyEntityType::Silverfish, EntityId(0));
    if (silverfish) {
        // 设置位置（方块中心）
        silverfish->setPosition(
            static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
        silverfish->setRotation(0.0f, 0.0f);

        // 生成到世界
        world.spawnEntity(std::move(silverfish));
    }
}

} // namespace blocks
} // namespace mc
