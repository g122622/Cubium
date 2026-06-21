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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MooshroomEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/registry/NaturalBlocks.hpp"
#include "../../../../world/block/registry/VanillaBlocks.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../entities/passive/basic/CowEntity.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <memory>

namespace mc {

MooshroomEntity::MooshroomEntity(EntityId id)
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
    using namespace mc::client::renderer::trident::particle;
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

bool MooshroomEntity::canBeStewed(const ItemStack& itemStack) const
{
    // 检查是否是空碗且成年
    if (isChild()) {
        return false; // 幼年哞菇不能被取汤
    }
    const Item* item = itemStack.getItem();
    return item == Items::BOWL;
}

ItemStack MooshroomEntity::getStew()
{
    // 返回蘑菇汤
    // TODO: 棕色哞菇可以返回迷之炖菜（需要效果系统支持）
    if (Items::MUSHROOM_STEW != nullptr) {
        return ItemStack(*Items::MUSHROOM_STEW, 1);
    }
    return ItemStack();
}

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
        using namespace mc::client::renderer::trident::particle;
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

// ========== 寻路权重 ==========

f32 MooshroomEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 哞菇偏好菌丝：站在菌丝上返回10.0f，否则返回亮度相关值
    // 对应 MC MushroomCow.getWalkTargetValue:
    //   return level.getBlockState(pos.below()).is(Blocks.MYCELIUM) ? 10.0F
    //        : level.getPathfindingCostFromLightLevels(pos);
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

} // namespace mc
