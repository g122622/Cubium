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

namespace mc::mod::bedrock::addon {

/** @brief 由所属世界、实体或物品持有，按行为包 UUID 隔离的动态属性。 */
class AddonDynamicProperties {
public:
    /** @brief 读取指定包的属性，缺失时返回空值。 */
    [[nodiscard]] std::optional<nlohmann::json> get(const std::string& pack, const std::string& name) const;
    /** @brief 写入一个属性，null 表示删除。 */
    Result<void> set(const std::string& pack, const std::string& name, const nlohmann::json& value);
    /** @brief 校验整个批次后一次性应用写入。 */
    Result<void> setMany(const std::string& pack, const nlohmann::json& values);
    /** @brief 列出指定包当前存储的键。 */
    [[nodiscard]] std::vector<std::string> ids(const std::string& pack) const;
    /** @brief 清空指定包的数据。 */
    void clear(const std::string& pack);
    /** @brief 编码整个属性表，保留布尔及向量类型。 */
    [[nodiscard]] nlohmann::json save() const;
    /** @brief 完整校验后替换属性表，失败时保留原值。 */
    Result<void> load(const nlohmann::json& saved);

    /** @brief 当前数据是否有尚未成功保存的修改。 */
    [[nodiscard]] bool isDirty() const;
    /** @brief 仅在保存成功后确认当前版本已落盘。 */
    void markSaved();

private:
    nlohmann::json m_values = nlohmann::json::object();
    bool m_dirty = false;
};

} // namespace mc::mod::bedrock::addon
