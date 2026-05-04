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
            // MC 1.16.5: 水平方向: 以实体为中心，±8格范围
            // 垂直方向: 以实体为中心，±8格范围，但限制在0-255之间
            f64 d3 = d0 + (rng.nextDouble() - 0.5) * 16.0;
            f64 d4 = math::clamp(d1 + static_cast<f64>(rng.nextInt(16) - 8), 0.0, 255.0);
            f64 d5 = d2 + (rng.nextDouble() - 0.5) * 16.0;

            // 如果实体正在骑乘，停止骑乘
            if (entity.isRiding()) {
                entity.stopRiding();
            }

            // MC 1.16.5: 使用 entity.attemptTeleport(d3, d4, d5, true) 检查目标位置是否安全
            // 项目当前缺少 attemptTeleport 方法，使用简化的传送逻辑
            // TODO: 当实现 attemptTeleport 后，替换为:
            // if (entity.attemptTeleport(d3, d4, d5, true)) {
            //     teleported = true;
            //     break;
            // }

            // 简化实现：直接设置位置
            // 注意：这不会检查碰撞和安全性，需要后续改进
            entity.setPosition(static_cast<f32>(d3), static_cast<f32>(d4), static_cast<f32>(d5));
            teleported = true;

            // 播放传送音效
            // MC 1.16.5: 狐狸使用 ENTITY_FOX_TELEPORT，其他使用 ITEM_CHORUS_FRUIT_TELEPORT
            entity.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT, 1.0f, 1.0f);
            world.playSound(SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT,
                           sound::SoundCategory::Players,
                           Vector3(static_cast<f32>(d0), static_cast<f32>(d1), static_cast<f32>(d2)),
                           1.0f, 1.0f);
            break;
        }

        // MC 1.16.5: 设置冷却时间（仅玩家）
        // 参考: net.minecraft.entity.player.PlayerEntity.getCooldownTracker()
        // TODO: 实现物品冷却追踪器 (CooldownTracker)
        // 当前的实现需要:
        // 1. Player 类添加 CooldownTracker 成员
        // 2. CooldownTracker::setCooldown(Item*, int) 方法
        // 3. CooldownTracker::tick() 在 Player::tick() 中调用
        // 4. 物品渲染时检查冷却状态（冷却中的物品显示冷却进度）
        //
        // 临时方案：使用 ItemCooldown 组件或简单的 map<ItemId, int>
        // if (auto* player = dynamic_cast<Player*>(&entity)) {
        //     player->getCooldownTracker().setCooldown(this, 20); // 20 ticks = 1秒
        // }
    }

    return result;
}

} // namespace item::items
} // namespace mc
