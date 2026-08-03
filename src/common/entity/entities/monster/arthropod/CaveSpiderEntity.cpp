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

#include "CaveSpiderEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "common/entity/entities/monster/arthropod/SpiderEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include <memory>
#include <optional>

namespace mc {

CaveSpiderEntity::CaveSpiderEntity(EntityInstanceId id)
    : SpiderEntity(id)
{
    // 注册属性
    registerAttributes();
}

std::optional<ResourceLocation> CaveSpiderEntity::getAmbientSound() const
{
    // 洞穴蜘蛛复用普通蜘蛛的环境音，对齐原版 CaveSpider（继承 Spider.getAmbientSound 返回 SPIDER_AMBIENT）。
    // sounds.json 中无 entity.cave_spider.*（洞穴蜘蛛共享 spider.* 音效），默认拼接会生成不存在的 cave_spider.ambient。
    return SoundEvents::ENTITY_SPIDER_AMBIENT;
}

std::unique_ptr<Entity> CaveSpiderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CaveSpiderEntity>(EntityInstanceId(0));
}

void CaveSpiderEntity::registerAttributes()
{
    // 调用父类方法
    SpiderEntity::registerAttributes();

    // 洞穴蜘蛛的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 12.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

bool CaveSpiderEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法执行基础攻击
    if (!SpiderEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 根据难度应用中毒效果
    Difficulty difficulty = m_world->difficulty();

    // 简单难度无中毒，普通难度7秒，困难难度15秒
    i32 poisonDuration = 0;
    if (difficulty == Difficulty::Normal) {
        poisonDuration = 7;
    } else if (difficulty == Difficulty::Hard) {
        poisonDuration = 15;
    }

    if (poisonDuration > 0) {
        // 应用中毒效果（等级0，持续时间以tick为单位）
        target.addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Poison,
            poisonDuration * 20, // 转换为tick
            0,                   // 等级0（中毒I）
            false,               // 不是来自药水
            true,                // 显示粒子
            true                 // 显示图标
            ));
    }

    return true;
}

} // namespace mc
