/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR AN OVERRIDING COPYRIGHT NOTICE OR
 * BEING DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "MooshroomEntity.hpp"

#include "../../../../core/Result.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../entity/effect/EffectInstance.hpp"
#include "../../../../entity/effect/EffectType.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/serialization/NbtHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/nbt/Nbt.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/blocks/vegetation/FlowerBlock.hpp"
#include "../../../../world/block/registry/NaturalBlocks.hpp"
#include "../../../../world/block/registry/VanillaBlocks.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../entities/passive/basic/CowEntity.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include <memory>

namespace mc {

using namespace entity::serialization::nbt_helper;

MooshroomEntity::MooshroomEntity(EntityInstanceId id)
    : CowEntity(id)
{
    // 默认红色哞菇
    // 注册 AI 目标（继承自 CowEntity）
    registerGoals();
}

std::unique_ptr<Entity> MooshroomEntity::create(IWorld* /*world*/)
{
    return std::make_unique<MooshroomEntity>(0);
}

std::vector<ItemStack> MooshroomEntity::shear(Player* player)
{
    // 剪毛后变成普通牛，掉落5个蘑菇
    MC_UNUSED(player);

    std::vector<ItemStack> drops;

    // 播放剪切音效
    playSound(SoundEvents::ENTITY_MOOSHROOM_SHEAR, 1.0f, 1.0f);

    IWorld* worldPtr = world();
    if (worldPtr == nullptr || worldPtr->isClientSide()) {
        // 客户端只播放音效和粒子
        return drops;
    }

    // 生成爆炸粒子效果（客户端检查已在上面）
    using namespace mc::particle;
    math::Random& random = worldPtr->getRandom();
    for (i32 i = 0; i < 20; ++i) {
        f32 offsetX = (random.nextFloat() - 0.5f) * width();
        f32 offsetY = random.nextFloat() * height();
        f32 offsetZ = (random.nextFloat() - 0.5f) * width();

        worldPtr->addParticle(
            ParticleTypeId::Explosion, Vector3(x() + offsetX, y() + offsetY, z() + offsetZ), Vector3(0.0f, 0.0f, 0.0f));
    }

    // 获取对应类型的蘑菇物品
    const Block* mushroomBlock = isRed() ? VanillaBlocks::RED_MUSHROOM : VanillaBlocks::BROWN_MUSHROOM;

    const BlockItem* mushroomItem = BlockItemRegistry::instance().getBlockItem(*mushroomBlock);
    if (mushroomItem != nullptr) {
        // 掉落5个蘑菇
        drops.emplace_back(static_cast<const Item*>(mushroomItem), 5);
    }

    // 创建新的牛实体替代哞菇
    auto cow = std::make_unique<CowEntity>(0);

    // 继承位置和朝向
    cow->setPosition(x(), y(), z());
    cow->setRotation(yaw(), pitch());

    // 继承生命值
    cow->setHealth(health());

    // 继承渲染偏航角
    cow->setRenderYawOffset(renderYawOffset());

    // 继承自定义名称
    if (hasCustomName()) {
        cow->setCustomName(customNameText());
        cow->setCustomNameVisible(isCustomNameVisible());
    }

    // 继承持久性
    if (isNoDespawnRequired()) {
        cow->enablePersistence();
    }

    // 继承无敌状态
    cow->setInvulnerable(isInvulnerable());

    // 生成牛实体
    worldPtr->spawnEntity(std::move(cow));

    // 移除哞菇实体
    remove();

    return drops;
}

// ========== 玩家交互 ==========

ActionResultType MooshroomEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();

    // ====== 分支1: 空碗 → 蘑菇汤/迷之炖菜 ======
    if (item == Items::BOWL && !isChild()) {
        // 记住是否有迷之炖菜效果（清除前记录，用于音效判断）
        bool hadStewEffect = hasStewEffect();

        if (m_world != nullptr && !m_world->isClientSide()) {
            ItemStack stewStack;

            if (hadStewEffect) {
                // 棕色哞菇被喂食花朵后，返回迷之炖菜
                stewStack = ItemStack(*Items::SUSPICIOUS_STEW, 1);

                // 将迷之炖菜效果写入物品 NBT
                nlohmann::json& tag = stewStack.getOrCreateTag();
                nlohmann::json effectsArray = nlohmann::json::array();

                nlohmann::json effectJson;
                effectJson["EffectId"] = static_cast<i8>(static_cast<i32>(m_stewEffectType.value()));

                // 持续时间转换：瞬间效果保持原始 tick 数，非瞬间效果秒×20
                i32 durationTicks = m_stewEffectDuration;
                if (!entity::effect::isInstantEffect(m_stewEffectType.value())) {
                    durationTicks = m_stewEffectDuration * 20;
                }
                effectJson["EffectDuration"] = durationTicks;
                effectsArray.push_back(std::move(effectJson));
                tag["Effects"] = std::move(effectsArray);

                // 清除存储的效果（每次取汤只产出一个迷之炖菜）
                clearStewEffect();
            } else {
                // 普通蘑菇汤
                if (Items::MUSHROOM_STEW != nullptr) {
                    stewStack = ItemStack(*Items::MUSHROOM_STEW, 1);
                }
            }

            if (!stewStack.isEmpty()) {
                // 消耗空碗（创造模式不消耗）
                if (!player.abilities().creativeMode) {
                    heldItem.shrink(1);
                }

                // 将汤物品添加到玩家物品栏，装不下则掉落
                i32 remaining = player.inventory().add(stewStack);
                if (remaining > 0) {
                    // 部分物品无法放入背包，掉落在地上
                    ItemStack dropStack = stewStack.copy();
                    dropStack.setCount(remaining);
                    ItemDropHelper::spawnItemEntity(m_world, dropStack, x(), y() + 0.5, z(), getRandom());
                }
            }
        }

        // 播放取汤音效
        if (hadStewEffect) {
            playSound(SoundEvents::ENTITY_MOOSHROOM_SUSPICIOUS_MILK, 1.0f, 1.0f);
        } else {
            playSound(SoundEvents::ENTITY_MOOSHROOM_MILK, 1.0f, 1.0f);
        }

        return ActionResultType::Success;
    }

    // ====== 分支2: 棕色哞菇 + 花朵 → 存储迷之炖菜效果 ======
    if (isBrown()) {
        auto flowerEffect = _getStewEffectFromItem(heldItem);
        if (flowerEffect.has_value()) {
            if (m_world != nullptr && !m_world->isClientSide()) {
                if (hasStewEffect()) {
                    // 已经存储了效果，拒绝（生成烟雾粒子表示拒绝）
                    // 生成2个烟雾粒子
                    using namespace mc::particle;
                    for (i32 i = 0; i < 2; ++i) {
                        f32 offsetX = (getRandom().nextFloat() - 0.5f) * width();
                        f32 offsetY = getRandom().nextFloat() * height();
                        f32 offsetZ = (getRandom().nextFloat() - 0.5f) * width();
                        m_world->addParticle(ParticleTypeId::Smoke,
                            Vector3(x() + offsetX, y() + offsetY, z() + offsetZ),
                            Vector3(0.0f, 0.1f, 0.0f));
                    }
                } else {
                    // 消耗1个花朵物品（创造模式不消耗）
                    if (!player.abilities().creativeMode) {
                        heldItem.shrink(1);
                    }

                    // 存储花朵的效果
                    setStewEffect(flowerEffect->first, flowerEffect->second);

                    // 生成4个附魔粒子效果（表示成功存储）
                    using namespace mc::particle;
                    for (i32 i = 0; i < 4; ++i) {
                        f32 offsetX = (getRandom().nextFloat() - 0.5f) * width();
                        f32 offsetY = getRandom().nextFloat() * height();
                        f32 offsetZ = (getRandom().nextFloat() - 0.5f) * width();
                        // 使用 EntityEffect 粒子表示成功喂食花朵
                        m_world->addParticle(ParticleTypeId::EntityEffect,
                            Vector3(x() + offsetX, y() + offsetY, z() + offsetZ),
                            Vector3(0.0f, 0.0f, 0.0f));
                    }

                    // 播放吃东西音效
                    playSound(SoundEvents::ENTITY_MOOSHROOM_EAT, 1.0f, 1.0f);
                }
            }
            return ActionResultType::Success;
        }
    }

    // 传递给父类处理（繁殖等交互）
    return CowEntity::interactMob(player, hand);
}

// ========== 迷之炖菜效果 ==========

void MooshroomEntity::setStewEffect(entity::effect::EffectType type, i32 duration)
{
    m_stewEffectType = type;
    m_stewEffectDuration = duration;
}

void MooshroomEntity::clearStewEffect()
{
    m_stewEffectType = std::nullopt;
    m_stewEffectDuration = 0;
}

std::optional<std::pair<entity::effect::EffectType, i32>> MooshroomEntity::_getStewEffectFromItem(
    const ItemStack& itemStack)
{
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return std::nullopt;
    }

    // 通过 BlockItemRegistry 获取物品对应的方块
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    if (block == nullptr) {
        return std::nullopt;
    }

    // 检查方块是否为花朵且具有迷之炖菜效果
    const blocks::FlowerBlock* flower = dynamic_cast<const blocks::FlowerBlock*>(block);
    if (flower == nullptr || !flower->hasStewEffect()) {
        return std::nullopt;
    }

    u32 effectId = flower->getSuspiciousStewEffect();
    auto effectType = entity::effect::getEffectById(static_cast<i32>(effectId));
    if (!effectType.has_value()) {
        return std::nullopt;
    }

    return std::make_pair(effectType.value(), flower->getEffectDuration());
}

// ========== 繁殖 ==========

std::unique_ptr<AnimalEntity> MooshroomEntity::spawnBaby(AnimalEntity& partner)
{
    // 创建幼年哞菇
    auto baby = std::make_unique<MooshroomEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 遗传皮肤类型
    // 如果双亲类型相同，有 1/1024 概率变异为另一种类型
    // 否则，随机继承双亲之一的类型
    math::Random& rng = getRandom();

    MooshroomEntity* partnerMooshroom = dynamic_cast<MooshroomEntity*>(&partner);
    if (partnerMooshroom != nullptr) {
        MooshroomType myType = getMooshroomType();
        MooshroomType partnerType = partnerMooshroom->getMooshroomType();

        MooshroomType babyType;
        if (myType == partnerType && rng.nextInt(1024) == 0) {
            // 相同类型有 1/1024 概率变异
            babyType = (myType == MooshroomType::Red) ? MooshroomType::Brown : MooshroomType::Red;
        } else {
            // 随机继承双亲类型
            babyType = rng.nextBoolean() ? myType : partnerType;
        }
        baby->setMooshroomType(babyType);
    } else {
        // 如果配偶不是哞菇（不应该发生），继承自己的类型
        baby->setMooshroomType(getMooshroomType());
    }

    // 设置位置
    baby->setPosition(x(), y(), z());

    return baby;
}

// ========== 雷击 ==========

void MooshroomEntity::onStruckByLightning()
{
    // 红色哞菇 -> 棕色哞菇
    // 棕色哞菇 -> 红色哞菇
    // 播放转换音效并生成粒子效果

    // 切换类型
    MooshroomType newType = isRed() ? MooshroomType::Brown : MooshroomType::Red;
    setMooshroomType(newType);

    // 播放转换音效
    playSound(SoundEvents::ENTITY_MOOSHROOM_CONVERT, 2.0f, 1.0f);

    // 生成爆炸粒子效果（客户端）
    if (world() != nullptr && world()->isClientSide()) {
        using namespace mc::particle;
        math::Random& random = world()->getRandom();

        // 生成多个爆炸粒子
        constexpr i32 particleCount = 20;
        for (i32 i = 0; i < particleCount; ++i) {
            f32 offsetX = (random.nextFloat() - 0.5f) * width();
            f32 offsetY = random.nextFloat() * height();
            f32 offsetZ = (random.nextFloat() - 0.5f) * width();

            world()->addParticle(ParticleTypeId::Explosion,
                Vector3(x() + offsetX, y() + offsetY, z() + offsetZ),
                Vector3(0.0f, 0.0f, 0.0f));
        }
    }
}

void MooshroomEntity::registerGoals()
{
    // 调用父类方法（牛的行为）
    CowEntity::registerGoals();

    // 哞菇没有额外行为，完全继承牛的行为
}

std::optional<ResourceLocation> MooshroomEntity::getAmbientSound() const
{
    // 哞菇复用牛的环境音，对齐原版 Mooshroom（继承 AbstractCow.getAmbientSound 返回 COW_AMBIENT）。
    // 默认 makeSoundEventId 会拼接出 entity.mooshroom.ambient（sounds.json 不存在）。
    return SoundEvents::ENTITY_COW_AMBIENT;
}

// ========== 寻路权重 ==========

f32 MooshroomEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 哞菇偏好菌丝：站在菌丝上返回10.0f，否则返回亮度相关值
    const IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return 0.0f;
    }

    BlockPos posBelow(static_cast<i32>(x), static_cast<i32>(y) - 1, static_cast<i32>(z));
    const BlockState* groundBlock = worldPtr->getBlockState(posBelow);
    if (groundBlock != nullptr && groundBlock->is(block_registry::NaturalBlocks::MYCELIUM)) {
        return 10.0f;
    }

    return AnimalEntity::getPathWeight(x, y, z);
}

// ========== NBT序列化 ==========

void MooshroomEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    CowEntity::addAdditionalSaveData(tag);

    // 保存哞菇类型
    tag.put("Type", static_cast<i8>(static_cast<u8>(m_mooshroomType)));

    // 保存迷之炖菜效果（棕色哞菇用）
    if (m_stewEffectType.has_value()) {
        nbt::tags::compound_tag effectTag;
        effectTag.put("EffectId", static_cast<i8>(static_cast<i32>(m_stewEffectType.value())));
        effectTag.put("EffectDuration", m_stewEffectDuration);
        tag.value.emplace("StewEffect", std::make_unique<nbt::tags::compound_tag>(std::move(effectTag)));
    }
}

Result<void> MooshroomEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    MC_TRY(CowEntity::readAdditionalSaveData(tag));

    // 读取哞菇类型
    if (auto typeVal = tryGetByte(tag, "Type")) {
        m_mooshroomType = static_cast<MooshroomType>(static_cast<u8>(*typeVal));
    }

    // 读取迷之炖菜效果
    if (const auto* effectTag = tryGetCompound(tag, "StewEffect")) {
        if (auto effectId = tryGetByte(*effectTag, "EffectId")) {
            auto effectType = entity::effect::getEffectById(static_cast<i32>(*effectId));
            if (effectType.has_value()) {
                m_stewEffectType = effectType.value();
                m_stewEffectDuration = tryGetInt(*effectTag, "EffectDuration").value_or(0);
            }
        }
    }

    return Result<void>::ok();
}

} // namespace mc
