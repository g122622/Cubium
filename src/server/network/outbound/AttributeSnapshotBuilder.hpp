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

#pragma once

#include "common/entity/attribute/AttributeMap.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/network/backend/java/mappings/JavaAttributeIdMap.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"

#include <string>
#include <utility>

namespace mc::server::net {

/**
 * @brief 把生物的当前属性收集成属性同步包负载
 *
 * 只收 JavaAttributeIdMap 里**已映射**且 vanilla 标为可同步的属性：项目尚未实现的 vanilla
 * 属性没有取值来源；标为不可同步的属性（attack_damage 等）本就不该下发。
 *
 * 客户端对该包的语义是「整条属性全量替换」——先设基础值，再清空该属性的全部修饰符并逐个
 * 加入本次下发的那些。故每次都必须带全部修饰符，不能只发基础值或只发增量。
 *
 * 调用点有两处，缺任一处客户端都拿不到对应的属性：实体进入玩家视野时（EntityTracker），
 * 以及玩家自己加入时（LoginFlow，玩家自身的实体由 login 包建立、不经视野流程）。
 */
[[nodiscard]] inline mc::network::ir::play::UpdateAttributes buildAttributeSnapshot(const LivingEntity& living)
{
    using mc::network::backend::java::JavaAttributeIdMap;

    mc::network::ir::play::UpdateAttributes packet;
    packet.entityId = static_cast<i32>(living.id());

    living.attributes().forEachInstance(
        [&packet](const std::string& attributeName, const entity::attribute::AttributeInstance& instance) {
            const auto& idMap = JavaAttributeIdMap::instance();
            const auto javaId = idMap.toJavaRegistryId(attributeName);
            if (!javaId.has_value() || !idMap.isClientSyncable(*javaId)) {
                return;
            }

            mc::network::ir::play::AttributeSnapshot snapshot;
            snapshot.attributeRegistryId = static_cast<i32>(*javaId);
            snapshot.base = instance.baseValue();
            for (const auto& modifier : instance.modifiers()) {
                mc::network::ir::play::AttributeModifierWire wire;
                wire.id = modifier.id();
                wire.amount = modifier.amount();
                wire.operation = static_cast<i32>(modifier.operation());
                snapshot.modifiers.push_back(std::move(wire));
            }
            packet.attributes.push_back(std::move(snapshot));
        });
    return packet;
}

} // namespace mc::server::net
