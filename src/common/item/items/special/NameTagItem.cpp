#include "NameTagItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/core/MobEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/text/ITextComponent.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace item::items {

NameTagItem::NameTagItem(ItemProperties properties)
    : Item(std::move(properties)) {
}

bool NameTagItem::itemInteractionForEntity(ItemStack& stack, Player& player,
                                            LivingEntity& target, Hand hand) {
    // MC 1.16.5: NameTagItem.itemInteractionForEntity()

    // 检查物品是否有自定义名称
    if (!stack.hasCustomName()) {
        return false;
    }

    // 不能给玩家命名
    if (dynamic_cast<Player*>(&target) != nullptr) {
        return false;
    }

    // 只能对 MobEntity 命名（非玩家的生物实体）
    auto* mob = dynamic_cast<MobEntity*>(&target);
    if (mob == nullptr) {
        return false;
    }

    // 确保实体存活
    if (!mob->isAlive()) {
        return false;
    }

    // 获取物品的自定义名称
    const text::ITextComponent* customName = stack.getCustomNameComponent();
    if (customName == nullptr) {
        return false;
    }

    // 设置实体的自定义名称
    mob->setCustomNameComponent(customName->deepCopy());

    // MC 1.16.5: 命名牌命名后，实体变为持久化（不会消失）
    mob->enablePersistence();

    // 消耗一个物品（非创造模式）
    if (!player.isCreative()) {
        stack.shrink(1);
    }

    spdlog::debug("NameTagItem: Named mob {} with '{}', persistence enabled",
                  mob->id(), mob->customNameText());

    return true;
}

} // namespace item::items
} // namespace mc
