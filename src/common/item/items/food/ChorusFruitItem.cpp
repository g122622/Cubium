#include "ChorusFruitItem.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/passive/special/FoxEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item::items {

ChorusFruitItem::ChorusFruitItem(const food::Food* food, ItemProperties properties)
    : FoodItem(food, std::move(properties))
{}

ItemStack ChorusFruitItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 调用父类方法处理基本的食用逻辑
    ItemStack result = FoodItem::onItemUseFinish(stack, world, entity);

    // MC 1.16.5: 随机传送
    // 参考: net.minecraft.item.ChorusFruitItem#onItemUseFinish
    if (!world.isClientSide()) {
        // 记录原始位置用于播放音效
        Vector3 originalPos = entity.position();
        f64 d0 = originalPos.x;
        f64 d1 = originalPos.y;
        f64 d2 = originalPos.z;

        bool teleported = false;

        // 尝试随机传送
        // 紫颂果传送范围：水平方向 ±8 格，垂直方向 ±8 格
        // 参考 MC 1.16.5: ChorusFruitItem 使用 randomTeleport(16.0)
        // 但需要检查是否是狐狸（狐狸使用不同的音效）
        teleported = entity.randomTeleport(16.0, false, true);

        if (teleported) {
            // 播放传送音效
            // MC 1.16.5: 狐狸使用 ENTITY_FOX_TELEPORT，其他使用 ITEM_CHORUS_FRUIT_TELEPORT
            const bool isFox = dynamic_cast<FoxEntity*>(&entity) != nullptr;
            const auto& soundEvent = isFox ? SoundEvents::ENTITY_FOX_TELEPORT : SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT;

            // 在原位置播放音效
            world.playSound(soundEvent,
                sound::SoundCategory::Players,
                Vector3(static_cast<f32>(d0), static_cast<f32>(d1), static_cast<f32>(d2)),
                1.0f,
                1.0f);

            // 在新位置播放音效（实体自己播放）
            entity.playSound(soundEvent, 1.0f, 1.0f);
        }

        // MC 1.16.5: 设置冷却时间（仅玩家）
        // 冷却时间：20 ticks = 1 秒
        if (auto* player = dynamic_cast<Player*>(&entity)) {
            player->cooldownTracker().setCooldown(this, 20);
        }
    }

    return result;
}

} // namespace item::items
} // namespace mc
