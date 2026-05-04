#include "ChorusFruitItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/Vector3.hpp"

namespace mc {
namespace item::items {

ChorusFruitItem::ChorusFruitItem(const food::Food* food, ItemProperties properties)
    : FoodItem(food, std::move(properties)) {
}

ItemStack ChorusFruitItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) {
    // 调用父类方法处理基本的食用逻辑
    ItemStack result = FoodItem::onItemUseFinish(stack, world, entity);

    // MC 1.16.5: 随机传送
    // 参考: net.minecraft.item.ChorusFruitItem#onItemUseFinish
    if (!world.isClientSide()) {
        // 使用实体ID和时间生成随机数
        math::Random rng(static_cast<u64>(entity.id()) ^
                        static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

        // 记录原始位置用于播放音效
        Vector3 originalPos = entity.position();
        f64 d0 = originalPos.x;
        f64 d1 = originalPos.y;
        f64 d2 = originalPos.z;

        bool teleported = false;

        // 尝试16次传送
        for (i32 i = 0; i < 16; ++i) {
            // 计算目标位置
            // 水平方向: 以实体为中心，±8格范围
            // 垂直方向: 以实体为中心，±8格范围
            f64 d3 = d0 + (rng.nextDouble() - 0.5) * 16.0;
            f64 d4 = math::clamp(d1 + static_cast<f64>(rng.nextInt(16) - 8), 0.0, 255.0);
            f64 d5 = d2 + (rng.nextDouble() - 0.5) * 16.0;

            // 如果实体正在骑乘，停止骑乘
            if (entity.isRiding()) {
                entity.stopRiding();
            }

            // 尝试传送到目标位置
            // TODO: 实现 attemptTeleport 方法来检查目标位置是否安全
            // 目前使用简化的传送实现
            // entity.attemptTeleport(d3, d4, d5, true);

            // 简化实现：直接设置位置
            // 注意：这不会检查碰撞和安全性
            entity.setPosition(static_cast<f32>(d3), static_cast<f32>(d4), static_cast<f32>(d5));
            teleported = true;

            // 播放传送音效
            entity.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT, 1.0f, 1.0f);
            world.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT,
                           sound::SoundCategory::Players,
                           Vector3(static_cast<f32>(d0), static_cast<f32>(d1), static_cast<f32>(d2)),
                           1.0f, 1.0f);
            break;
        }

        // 设置冷却时间（仅玩家）
        // TODO: 实现冷却追踪器 CooldownTracker
        // if (auto* player = dynamic_cast<Player*>(&entity)) {
        //     player->getCooldownTracker().setCooldown(this, 20);
        // }
    }

    return result;
}

} // namespace item::items
} // namespace mc
