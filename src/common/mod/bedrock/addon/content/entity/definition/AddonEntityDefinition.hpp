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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/content/entity/events/AddonEntityEvents.hpp"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace mc {
class Entity;
class IWorld;

namespace ecs {
class EntityRegistry;
}
} // namespace mc

namespace mc::mod::bedrock::addon {

/** @brief 已验证的实体定义，供注册工厂和实例共用。 */
class AddonEntityDefinition {
public:
    /** @brief 校验实体 JSON 结构、属性和基础组件；未支持语义明确返回 Unsupported。 */
    [[nodiscard]] static Result<AddonEntityDefinition> parse(const nlohmann::json& document);
    /** @brief 校验一个基础组件表或组件组，不隐式补齐其他组件。 */
    static Result<void> validateComponents(const nlohmann::json& components);
    /** @brief 获取基础组件表和组件组定义。 */
    [[nodiscard]] const nlohmann::json& baseComponents() const;
    [[nodiscard]] const nlohmann::json& componentGroups() const;
    /** @brief 任一可用组件配置含健康组件时使用生物继承链。 */
    [[nodiscard]] bool isLiving() const;
    /** @brief 通过完整定义创建实体，实例保留定义的共享所有权。 */
    [[nodiscard]] static std::unique_ptr<mc::Entity> create(std::shared_ptr<const AddonEntityDefinition> definition,
        mc::IWorld* world,
        mc::ecs::EntityRegistry& registry,
        std::weak_ptr<EntityEventSink> sink);
    /** @brief 获取实体资源标识。 */
    [[nodiscard]] const std::string& identifier() const;
    /** @brief 获取完整源定义，用于判断重复加载是否改变定义。 */
    [[nodiscard]] const nlohmann::json& document() const;
    /** @brief 获取碰撞箱宽度和高度。 */
    [[nodiscard]] f32 width() const;
    [[nodiscard]] f32 height() const;
    /** @brief 判断命令和脚本是否可召唤此类型。 */
    [[nodiscard]] bool canSummon() const;
    /** @brief 判断实体是否具有火焰免疫组件。 */
    [[nodiscard]] bool fireImmune() const;

private:
    /** @brief 安装预先校验过的声明属性和基础组件状态。 */
    void _initialize(mc::Entity& entity) const;

    nlohmann::json m_document;
    nlohmann::json m_properties;
    nlohmann::json m_components;
    nlohmann::json m_componentGroups;
    std::shared_ptr<const EntityEventProgram> m_events;
    std::string m_identifier;
    f32 m_width;
    f32 m_height;
    bool m_canSummon;
};

} // namespace mc::mod::bedrock::addon
