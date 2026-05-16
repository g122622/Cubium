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

#include "EntityPredicate.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== EntityPredicate ==========

bool EntityPredicate::test(const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }

    // 检查实体类型
    // MC 1.16.5: 通过实体类型注册表比较类型ID
    // 项目中使用 entity.getTypeId() 返回资源位置字符串（如 "minecraft:zombie"）
    if (m_type.has_value()) {
        std::string entityTypeId = entity.getTypeId();
        if (entityTypeId != m_type.value().toString()) {
            return false;
        }
    }

    // 检查效果
    if (!m_effects.test(entity)) {
        return false;
    }

    return true;
}

bool EntityPredicate::test(const Entity& entity, const DamageSource& source) const
{
    MC_UNUSED(source);
    return test(entity);
}

Result<EntityPredicate> EntityPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EntityPredicate{};
    }

    std::optional<ResourceLocation> type;

    if (json.contains("type")) {
        type = ResourceLocation(json["type"].get<std::string>());
    }

    // 解析效果条件
    MobEffectsPredicate effects;
    if (json.contains("effects")) {
        auto effectsResult = MobEffectsPredicate::fromJson(json["effects"]);
        if (effectsResult.failed()) {
            return effectsResult.error();
        }
        effects = effectsResult.value();
    }

    EntityPredicate predicate;
    predicate.m_type = std::move(type);
    predicate.m_effects = std::move(effects);
    predicate.m_isAny = !predicate.m_type.has_value() && predicate.m_effects.isAny();
    return predicate;
}

nlohmann::json EntityPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;
    if (m_type.has_value()) {
        json["type"] = m_type.value().toString();
    }
    if (!m_effects.isAny()) {
        json["effects"] = m_effects.toJson();
    }
    return json;
}

// ========== DamageSourcePredicate ==========

bool DamageSourcePredicate::test(const DamageSource& source) const
{
    if (m_isAny) {
        return true;
    }

    // MC 1.16.5 DamageSourcePredicate.test()
    // 检查每个属性，如果设置了检查条件但实际值不匹配，则返回 false

    // 检查是否为投射物伤害
    if (m_isProjectile.has_value() && source.isProjectile() != m_isProjectile.value()) {
        return false;
    }

    // 检查是否为爆炸伤害
    if (m_isExplosion.has_value() && source.isExplosion() != m_isExplosion.value()) {
        return false;
    }

    // 检查是否绕过护甲
    if (m_bypassesArmor.has_value() && source.bypassesArmor() != m_bypassesArmor.value()) {
        return false;
    }

    // 检查是否绕过无敌模式（创造模式）
    if (m_bypassesInvulnerability.has_value() && source.canDamageCreative() != m_bypassesInvulnerability.value()) {
        return false;
    }

    // 检查是否绕过魔法保护
    if (m_bypassesMagic.has_value() && source.isDamageAbsolute() != m_bypassesMagic.value()) {
        return false;
    }

    // 检查是否为火焰伤害
    if (m_isFire.has_value() && source.isFire() != m_isFire.value()) {
        return false;
    }

    // 检查是否为魔法伤害
    if (m_isMagic.has_value() && source.isMagic() != m_isMagic.value()) {
        return false;
    }

    // 检查是否为闪电伤害
    // MC 1.16.5: source == DamageSource.LIGHTNING_BOLT
    // 项目中通过 DamageType::LightningBolt 判断
    if (m_isLightning.has_value()) {
        bool isLightning = (source.type() == DamageType::LightningBolt);
        if (isLightning != m_isLightning.value()) {
            return false;
        }
    }

    return true;
}

Result<DamageSourcePredicate> DamageSourcePredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return DamageSourcePredicate{};
    }

    DamageSourcePredicate predicate;

    // 解析可选布尔值
    // MC 1.16.5 JSON 字段名映射:
    // is_projectile -> m_isProjectile
    // is_explosion -> m_isExplosion
    // bypasses_armor -> m_bypassesArmor
    // bypasses_invulnerability -> m_bypassesInvulnerability
    // bypasses_magic -> m_bypassesMagic
    // is_fire -> m_isFire
    // is_magic -> m_isMagic
    // is_lightning -> m_isLightning

    if (json.contains("is_projectile")) {
        predicate.m_isProjectile = json["is_projectile"].get<bool>();
    }
    if (json.contains("is_explosion")) {
        predicate.m_isExplosion = json["is_explosion"].get<bool>();
    }
    if (json.contains("bypasses_armor")) {
        predicate.m_bypassesArmor = json["bypasses_armor"].get<bool>();
    }
    if (json.contains("bypasses_invulnerability")) {
        predicate.m_bypassesInvulnerability = json["bypasses_invulnerability"].get<bool>();
    }
    if (json.contains("bypasses_magic")) {
        predicate.m_bypassesMagic = json["bypasses_magic"].get<bool>();
    }
    if (json.contains("is_fire")) {
        predicate.m_isFire = json["is_fire"].get<bool>();
    }
    if (json.contains("is_magic")) {
        predicate.m_isMagic = json["is_magic"].get<bool>();
    }
    if (json.contains("is_lightning")) {
        predicate.m_isLightning = json["is_lightning"].get<bool>();
    }

    // 计算是否为任意匹配
    predicate.m_isAny = !predicate.m_isProjectile.has_value() &&
                        !predicate.m_isExplosion.has_value() &&
                        !predicate.m_bypassesArmor.has_value() &&
                        !predicate.m_bypassesInvulnerability.has_value() &&
                        !predicate.m_bypassesMagic.has_value() &&
                        !predicate.m_isFire.has_value() &&
                        !predicate.m_isMagic.has_value() &&
                        !predicate.m_isLightning.has_value();

    return predicate;
}

nlohmann::json DamageSourcePredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;

    // 只添加非 null 的属性
    if (m_isProjectile.has_value()) {
        json["is_projectile"] = m_isProjectile.value();
    }
    if (m_isExplosion.has_value()) {
        json["is_explosion"] = m_isExplosion.value();
    }
    if (m_bypassesArmor.has_value()) {
        json["bypasses_armor"] = m_bypassesArmor.value();
    }
    if (m_bypassesInvulnerability.has_value()) {
        json["bypasses_invulnerability"] = m_bypassesInvulnerability.value();
    }
    if (m_bypassesMagic.has_value()) {
        json["bypasses_magic"] = m_bypassesMagic.value();
    }
    if (m_isFire.has_value()) {
        json["is_fire"] = m_isFire.value();
    }
    if (m_isMagic.has_value()) {
        json["is_magic"] = m_isMagic.value();
    }
    if (m_isLightning.has_value()) {
        json["is_lightning"] = m_isLightning.value();
    }

    return json;
}

} // namespace mc::advancement
