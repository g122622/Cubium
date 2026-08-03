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
#include "common/advancement/trigger/conditions/EntityEquipmentPredicate.hpp"
#include "common/advancement/trigger/conditions/EntityFlagsPredicate.hpp"
#include "common/advancement/trigger/conditions/LocationPredicate.hpp"
#include "common/advancement/trigger/conditions/MobEffectsPredicate.hpp"
#include "common/advancement/trigger/conditions/NBTPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== EntityPredicate ==========

void EntityPredicate::_updateIsAny()
{
    m_isAny = !m_type.has_value() && m_distance.isAny() && m_location.isAny() && m_effects.isAny() && m_nbt.isAny() &&
        m_flags.isAny() && m_equipment.isAny();
}

bool EntityPredicate::test(const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }

    // 1. 检查实体类型
    if (m_type.has_value()) {
        std::string entityTypeId = entity.getTypeId();
        if (entityTypeId != m_type.value().toString()) {
            return false;
        }
    }

    // 2. 检查效果
    if (!m_effects.test(entity)) {
        return false;
    }

    // 3. 检查NBT
    if (!m_nbt.test(entity)) {
        return false;
    }

    // 4. 检查标志
    if (!m_flags.test(entity)) {
        return false;
    }

    // 5. 检查装备
    if (!m_equipment.test(entity)) {
        return false;
    }

    // 注意：距离和位置检查需要参考位置，在无参考位置版本中跳过。
    // 需要距离和位置检查的调用者应使用 test(world, x, y, z, entity) 重载。

    return true;
}

bool EntityPredicate::test(const IWorld& world, f64 x, f64 y, f64 z, const Entity& entity) const
{
    if (m_isAny) {
        return true;
    }

    // 1. 检查实体类型
    if (m_type.has_value()) {
        std::string entityTypeId = entity.getTypeId();
        if (entityTypeId != m_type.value().toString()) {
            return false;
        }
    }

    // 2. 检查距离（参考点到实体的距离）
    if (!m_distance.isAny()) {
        if (!m_distance.test(x, y, z, entity.x(), entity.y(), entity.z())) {
            return false;
        }
    }

    // 3. 检查位置（实体当前位置）
    if (!m_location.isAny()) {
        if (!m_location.test(world, entity.x(), entity.y(), entity.z())) {
            return false;
        }
    }

    // 4. 检查效果
    if (!m_effects.test(entity)) {
        return false;
    }

    // 5. 检查NBT
    if (!m_nbt.test(entity)) {
        return false;
    }

    // 6. 检查标志
    if (!m_flags.test(entity)) {
        return false;
    }

    // 7. 检查装备
    if (!m_equipment.test(entity)) {
        return false;
    }

    return true;
}

bool EntityPredicate::test(const Entity& entity, const DamageSource& source) const
{
    // 注意：EntityPredicate 不检查 DamageSource，DamageSource 由 DamageSourcePredicate 独立检查。
    // 参考 MC Java: EntityPredicate.matches() 不接受 DamageSource 参数，
    // DamageSource 的匹配由 KilledTrigger.TriggerInstance 中独立的 killingBlow 谓词完成。
    // 此重载仅为调用方便而保留，委托给 test(entity)。
    MC_UNUSED(source);
    return test(entity);
}

Result<EntityPredicate> EntityPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return EntityPredicate{};
    }

    EntityPredicate predicate;

    // 解析实体类型
    if (json.contains("type")) {
        predicate.m_type = ResourceLocation(json["type"].get<std::string>());
    }

    // 解析距离条件
    if (json.contains("distance")) {
        auto result = DistancePredicate::fromJson(json["distance"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_distance = result.value();
    }

    // 解析位置条件
    if (json.contains("location")) {
        auto result = LocationPredicate::fromJson(json["location"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_location = result.value();
    }

    // 解析效果条件
    if (json.contains("effects")) {
        auto result = MobEffectsPredicate::fromJson(json["effects"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_effects = result.value();
    }

    // 解析NBT条件
    if (json.contains("nbt")) {
        auto result = NBTPredicate::fromJson(json["nbt"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_nbt = result.value();
    }

    // 解析标志条件
    if (json.contains("flags")) {
        auto result = EntityFlagsPredicate::fromJson(json["flags"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_flags = result.value();
    }

    // 解析装备条件
    if (json.contains("equipment")) {
        auto result = EntityEquipmentPredicate::fromJson(json["equipment"]);
        if (result.failed()) {
            return result.error();
        }
        predicate.m_equipment = result.value();
    }

    predicate._updateIsAny();
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
    if (!m_distance.isAny()) {
        json["distance"] = m_distance.toJson();
    }
    if (!m_location.isAny()) {
        json["location"] = m_location.toJson();
    }
    if (!m_effects.isAny()) {
        json["effects"] = m_effects.toJson();
    }
    if (!m_nbt.isAny()) {
        json["nbt"] = m_nbt.toJson();
    }
    if (!m_flags.isAny()) {
        json["flags"] = m_flags.toJson();
    }
    if (!m_equipment.isAny()) {
        json["equipment"] = m_equipment.toJson();
    }

    return json;
}

// ========== DamageSourcePredicate ==========

bool DamageSourcePredicate::test(const DamageSource& source) const
{
    if (m_isAny) {
        return true;
    }

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
    predicate.m_isAny = !predicate.m_isProjectile.has_value() && !predicate.m_isExplosion.has_value() &&
        !predicate.m_bypassesArmor.has_value() && !predicate.m_bypassesInvulnerability.has_value() &&
        !predicate.m_bypassesMagic.has_value() && !predicate.m_isFire.has_value() && !predicate.m_isMagic.has_value() &&
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
