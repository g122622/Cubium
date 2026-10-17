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

#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include <nlohmann/json.hpp>

namespace mc {

// 前向声明
class Player;
class Entity;
class MobEntity;
class ItemStack;

namespace entity {

/// 鱼桶携带实体数据的子标签键（对齐 vanilla ItemStack 中 BucketEntityData 子键）。
extern const char* const BUCKET_ENTITY_DATA_KEY;

/**
 * @brief 保存可装桶实体的默认数据到鱼桶
 *
 * 对齐 Java Bucketable.saveDefaultDataToBucketTag(Mob, ItemStack)：将实体的
 * 自定义名/NoAI/Silent/NoGravity/发光/无敌/生命值写入桶的 BucketEntityData 子标签。
 *
 * @param entity 被装取的实体
 * @param bucketStack 装取后得到的鱼桶（写出目标）
 */
void saveDefaultDataToBucketTag(const MobEntity& entity, ItemStack& bucketStack);

/**
 * @brief 从鱼桶的默认数据恢复实体（saveDefaultDataToBucketTag 的逆操作）
 *
 * 对齐 Java Bucketable.loadDefaultDataFromBucketTag(Mob, CompoundTag)：读 BucketEntityData
 * 子标签中的 NoAI/Silent/NoGravity/发光/无敌/生命值并写回实体。
 *
 * @param entity 放出的实体
 * @param tag 桶携带的 BucketEntityData 子标签（无数据时为 null）
 */
void loadDefaultDataFromBucketTag(MobEntity& entity, const nlohmann::json& tag);

/**
 * @brief 水桶装取实体工具（对齐 Java 1.21.11 Bucketable.bucketMobPickup 静态方法）
 *
 * 由可装桶实体（AbstractFishEntity/AxolotlEntity）的 interactMob 调用。
 *
 * @param player 执行装取的玩家
 * @param target 装取目标实体（必须同时实现 IBucketable，否则返 Pass）
 * @param hand 玩家使用的手
 * @return Success（装取成功）/ Pass（手中非水桶/目标非 IBucketable/目标已死亡）
 *
 * 流程（对齐 Java Bucketable.bucketMobPickup）：
 *   1. 手持 WATER_BUCKET && target.isAlive() → playSound(getPickupSound())
 *   2. getBucketItemStack() 拿对应鱼桶 + saveToBucketTag() 保存实体数据
 *   3. 非创造模式：手中水桶替换为鱼桶；创造模式：水桶保留（对齐 Java createFilledResult）
 *   4. target.discard() 实体消失
 */
ActionResultType bucketMobPickup(Player& player, Entity& target, Hand hand);

} // namespace entity
} // namespace mc
