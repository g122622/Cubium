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

#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::entity::serialization::components {
class ComponentSerializerRegistry;
}

namespace mc::mod::bedrock::addon {

/** @brief 实体的声明属性、待应用属性和按行为包隔离的动态属性组件。 */
class EntityAddonState {
public:
    /** @brief 校验实体 JSON 属性定义并安装默认值；失败时保留原状态。 */
    Result<void> defineProperties(const nlohmann::json& definitions);

    /** @brief 读取本 tick 已经生效的声明属性。 */
    [[nodiscard]] std::optional<nlohmann::json> getProperty(const std::string& name) const;

    /** @brief 校验并暂存声明属性，在下一实体 tick 应用。 */
    Result<void> setProperty(const std::string& name, const nlohmann::json& value);

    /** @brief 暂存默认值并返回它；未声明的属性返回空值，下一 tick 生效。 */
    [[nodiscard]] std::optional<nlohmann::json> resetProperty(const std::string& name);

    /** @brief 在实体 tick 边界应用待处理属性，返回是否存在变化。 */
    bool applyPendingProperties();

    /** @brief 读取指定行为包作用域内的动态属性。 */
    [[nodiscard]] std::optional<nlohmann::json> getDynamicProperty(
        const std::string& pack, const std::string& name) const;

    /** @brief 写入动态属性；null 删除该键，其他包的同名属性不受影响。 */
    Result<void> setDynamicProperty(const std::string& pack, const std::string& name, const nlohmann::json& value);

    /** @brief 先校验整个批次，再一次性写入指定包作用域。 */
    Result<void> setDynamicProperties(const std::string& pack, const nlohmann::json& values);

    /** @brief 列出指定行为包的动态属性名。 */
    [[nodiscard]] std::vector<std::string> dynamicPropertyIds(const std::string& pack) const;

    /** @brief 清空指定行为包的动态属性。 */
    void clearDynamicProperties(const std::string& pack);

    /** @brief 生成带显式类型标记的持久化快照，保留 NBT 中无法区分的布尔类型。 */
    [[nodiscard]] nlohmann::json save() const;

    /** @brief 校验并恢复快照，声明属性需由实体定义预先安装。 */
    Result<void> load(const nlohmann::json& saved);

private:
    /** @brief 按属性定义校验值类型、整数约束、数值范围和枚举集合。 */
    static Result<void> _validateProperty(
        const std::string& name, const nlohmann::json& definition, const nlohmann::json& value);

    /** @brief 校验动态属性允许的标量和三维向量类型。 */
    static Result<void> _validateDynamicValue(const nlohmann::json& value);

    nlohmann::json m_definitions = nlohmann::json::object();
    nlohmann::json m_properties = nlohmann::json::object();
    nlohmann::json m_pending = nlohmann::json::object();
    nlohmann::json m_dynamic = nlohmann::json::object();
};

/** @brief 将实体 addon 属性接入统一组件序列化器。 */
void registerEntityAddonStateSerializer(mc::entity::serialization::components::ComponentSerializerRegistry& registry);

} // namespace mc::mod::bedrock::addon
