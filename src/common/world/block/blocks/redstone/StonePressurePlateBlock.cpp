#include "StonePressurePlateBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../util/AxisAlignedBB.hpp"

namespace mc {
namespace blocks {

// 石头压力板tick延迟
static constexpr i32 STONE_PLATE_DELAY = 10;

StonePressurePlateBlock::StonePressurePlateBlock(const BlockProperties& properties)
    : AbstractPressurePlateBlock(properties) {
}

i32 StonePressurePlateBlock::calculateSignalStrength(IWorld& world, const BlockPos& pos) const {
    // 石头压力板只能被生物触发，物品不会触发
    // 有生物时输出15，无生物时输出0

    // 创建压力板上方的碰撞箱
    AxisAlignedBB detectionBox(
        static_cast<f32>(pos.x) + 0.125f,
        static_cast<f32>(pos.y) + 0.0f,
        static_cast<f32>(pos.z) + 0.125f,
        static_cast<f32>(pos.x) + 0.875f,
        static_cast<f32>(pos.y) + 0.25f,
        static_cast<f32>(pos.z) + 0.875f
    );

    // 查询碰撞箱内的实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(detectionBox, nullptr);

    // 石头压力板只被生物触发（不包括物品）
    for (Entity* entity : entities) {
        if (entity != nullptr) {
            LegacyEntityType type = entity->legacyType();
            // 石头压力板只检测玩家（后续可添加 Mob 类型）
            if (type == LegacyEntityType::Player) {
                return 15;  // 有生物就输出最大信号
            }
            // Item 类型不触发石头压力板
        }
    }

    return 0;
}

i32 StonePressurePlateBlock::getTickDelay(i32 oldSignal, i32 newSignal) const {
    MC_UNUSED(oldSignal);
    MC_UNUSED(newSignal);
    return STONE_PLATE_DELAY;
}

void StonePressurePlateBlock::playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const {
    // 参考 MC 1.16.5: StonePressurePlateBlock.playClickSound
    world.playSound(
        pressed ? SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_ON : SoundEvents::BLOCK_STONE_PRESSURE_PLATE_CLICK_OFF,
        sound::SoundCategory::Blocks,
        pos.center(),
        0.3f,
        0.6f
    );
}

} // namespace blocks
} // namespace mc
