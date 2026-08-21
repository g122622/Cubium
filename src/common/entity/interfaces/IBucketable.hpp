/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy, modify, merge,
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

#pragma once

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>

namespace mc {

// 前向声明
class Player;
class ItemStack;

namespace entity {

/**
 * @brief 可装桶接口 - 用于可以被水桶装起的实体
 *
 * 实现此接口的实体可以由玩家手持水桶右键装入对应鱼桶（对齐 Java 1.21.11
 * net.minecraft.world.entity.animal.Bucketable）。例如：鳕鱼/鲑鱼/河豚/热带鱼/美西螈。
 *
 * 装取流程（对齐 Java Bucketable.bucketMobPickup）：
 *   1. 玩家手持水桶右键实体 → 实体 interactMob 调 IBucketable::bucketMobPickup
 *   2. bucketMobPickup 检测 WATER_BUCKET && isAlive → playSound(getPickupSound())
 *      + getBucketItemStack() 拿对应鱼桶 + saveToBucketTag() 保存实体数据到桶
 *      + 替换玩家手中水桶为鱼桶（创造模式不消耗水桶）+ discard() 实体消失
 *
 * 反向放鱼（鱼桶→鱼）由 FishBucketItem::onItemUse/onItemRightClick 处理，放鱼时
 * setFromBucket(true) 防止消失（见 FishBucketItem::_spawnFish）。
 */
class IBucketable {
public:
    virtual ~IBucketable() = default;

    /**
     * @brief 是否来自桶（从桶放出的鱼不消失）
     *
     * 对应 Java Bucketable.fromBucket()。AbstractFishEntity/AxolotlEntity 已有
     * isFromBucket/setFromBucket 业务字段（同步 FROM_BUCKET 数据参数）。
     * 注：Cubium 沿用既有 isFromBucket 命名（Java 为 fromBucket），语义一致。
     */
    virtual bool isFromBucket() const = 0;
    virtual void setFromBucket(bool fromBucket) = 0;

    /**
     * @brief 获取装取该实体后得到的鱼桶物品
     * @return 对应鱼桶 ItemStack（如鳕鱼→cod_bucket）
     *
     * 对应 Java Bucketable.getBucketItemStack()，各鱼子类 override 返回对应鱼桶。
     */
    virtual ItemStack getBucketItemStack() const = 0;

    /**
     * @brief 获取装取音效
     * @return 装取音效事件（鱼类共用 BUCKET_FILL_FISH，美西螈用 BUCKET_EMPTY_AXOLOTL）
     *
     * 对应 Java Bucketable.getPickupSound()。
     */
    virtual std::optional<ResourceLocation> getPickupSound() const = 0;

    /**
     * @brief 保存实体数据到鱼桶 NBT
     * @param bucketStack 装取后得到的鱼桶物品
     *
     * 对应 Java Bucketable.saveToBucketTag(ItemStack)，保存 Health/NoAI/Silent/NoGravity
     * 等实体状态到桶，反向放鱼时 loadFromBucketTag 恢复。Cubium FishBucketItem._spawnFish
     * 当前不读桶 NBT（直接创建新鱼），故本方法暂为空实现（留 TODO），不影响装取主链路。
     */
    virtual void saveToBucketTag(ItemStack& bucketStack) const = 0;
};

} // namespace entity
} // namespace mc
