/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "BucketableUtils.hpp"

#include "IBucketable.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {
namespace entity {

const char* const BUCKET_ENTITY_DATA_KEY = "BucketEntityData";

void saveDefaultDataToBucketTag(const MobEntity& entity, ItemStack& bucketStack)
{
    // 对齐 Java Bucketable.saveDefaultDataToBucketTag(Mob, ItemStack)：
    //   自定义名 → CUSTOM_NAME 组件；NoAI/Silent/NoGravity/Glowing/Invulnerable/Health
    //   → BUCKET_ENTITY_DATA 子标签。
    // Cubium 将 BUCKET_ENTITY_DATA 落到鱼桶 ItemStack 的 BucketEntityData 子标签
    // （getOrCreateChildTag），自定义名以纯文本形式一并存入该子标签。
    nlohmann::json& data = bucketStack.getOrCreateChildTag(BUCKET_ENTITY_DATA_KEY);

    if (!entity.customNameText().empty()) {
        data["CustomName"] = entity.customNameText();
    }
    if (entity.isNoAi()) {
        data["NoAI"] = true;
    }
    if (entity.isSilent()) {
        data["Silent"] = true;
    }
    if (entity.hasNoGravity()) {
        data["NoGravity"] = true;
    }
    if (entity.isGlowing()) {
        data["Glowing"] = true;
    }
    if (entity.isInvulnerable()) {
        data["Invulnerable"] = true;
    }
    data["Health"] = entity.health();
}

void loadDefaultDataFromBucketTag(MobEntity& entity, const nlohmann::json& tag)
{
    // 对齐 Java Bucketable.loadDefaultDataFromBucketTag(Mob, CompoundTag)。
    // tag 可能为空 json（桶无数据）——此时各字段均缺席，实体保持默认状态。
    if (!tag.is_object()) {
        return;
    }

    if (tag.contains("CustomName") && tag["CustomName"].is_string()) {
        entity.setCustomName(tag["CustomName"].get<std::string>());
    }
    if (tag.contains("NoAI") && tag["NoAI"].is_boolean()) {
        entity.setNoAi(tag["NoAI"].get<bool>());
    }
    if (tag.contains("Silent") && tag["Silent"].is_boolean()) {
        entity.setSilent(tag["Silent"].get<bool>());
    }
    if (tag.contains("NoGravity") && tag["NoGravity"].is_boolean()) {
        entity.setNoGravity(tag["NoGravity"].get<bool>());
    }
    if (tag.contains("Glowing") && tag["Glowing"].is_boolean()) {
        entity.setGlowing(tag["Glowing"].get<bool>());
    }
    if (tag.contains("Invulnerable") && tag["Invulnerable"].is_boolean()) {
        entity.setInvulnerable(tag["Invulnerable"].get<bool>());
    }
    if (tag.contains("Health") && tag["Health"].is_number()) {
        entity.setHealth(tag["Health"].get<f32>());
    }
}

ActionResultType bucketMobPickup(Player& player, Entity& target, Hand hand)
{
    // 对齐 Java Bucketable.bucketMobPickup：
    //   ItemStack itemstack = player.getItemInHand(hand);
    //   if (itemstack.getItem() == Items.WATER_BUCKET && target.isAlive()) {
    //       target.playSound(getPickupSound(), 1, 1);
    //       ItemStack bucket = target.getBucketItemStack();
    //       target.saveToBucketTag(bucket);
    //       ItemStack result = ItemUtils.createFilledResult(itemstack, player, bucket, false);
    //       player.setItemInHand(hand, result);
    //       target.discard();
    //       return SUCCESS;
    //   }
    //   return EMPTY;  // 调用方或默认 super.mobInteract

    // 目标必须是可装桶实体。
    auto* bucketable = dynamic_cast<IBucketable*>(&target);
    if (bucketable == nullptr) {
        return ActionResultType::Pass;
    }

    ItemStack& heldItem = player.getHeldItem(hand);

    // 手持水桶且目标存活才装取（对齐 Java itemstack.getItem()==WATER_BUCKET && isAlive）。
    if (heldItem.isEmpty() || heldItem.getItem() != Items::WATER_BUCKET || !target.isAlive()) {
        return ActionResultType::Pass;
    }

    // 播放装取音效（对齐 target.playSound(getPickupSound(), 1, 1)）。
    auto soundId = bucketable->getPickupSound();
    if (soundId.has_value()) {
        target.playSound(*soundId, 1.0f, 1.0f);
    }

    // 获取对应鱼桶并保存实体数据到桶 NBT。
    ItemStack bucketStack = bucketable->getBucketItemStack();
    bucketable->saveToBucketTag(bucketStack);

    // 替换玩家手中物品为鱼桶（对齐 Java ItemUtils.createFilledResult）。
    //   - 非创造模式：手中水桶替换为鱼桶（消耗水桶得鱼桶）。
    //   - 创造模式：手中水桶保留（创造模式物品无限，不消耗），对齐 Java createFilledResult
    //     创造模式返原 stack 的语义（创造模式装取实体但水桶不消耗）。
    if (!player.abilities().creativeMode) {
        heldItem = bucketStack;
    }

    // 实体消失（对齐 target.discard()）。
    target.discard();

    return ActionResultType::Success;
}

} // namespace entity
} // namespace mc
