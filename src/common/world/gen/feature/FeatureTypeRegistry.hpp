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
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <nlohmann/json_fwd.hpp>

namespace mc {

class ConfiguredFeatureBase;

namespace world::gen::feature {

/**
 * @brief 特征类型注册表
 *
 * configured_feature 的 type 字符串 → C++ 特征构造工厂映射。
 * ConfiguredFeatureLoader 解析 JSON 时，读 `type` 字段，调 `create(type, configJson, id)` 得到
 * `unique_ptr<ConfiguredFeatureBase>`，再注册到 ConfiguredFeatureRegistry。
 *
 * 严格报错策略：遇到未注册的 type 返回 Error（中断加载），便于运行时按报错定位未实现的 feature type。
 */
class FeatureTypeRegistry {
public:
    /**
     * @brief 特征工厂函数类型
     *
     * 从 JSON config 构造配置化特征。返回的 feature 尚未设置 id（由 Loader 调 setId）。
     *
     * @param configJson config 字段的 JSON 对象（可能为空）
     * @return 构造的特征，或错误信息
     */
    using Factory = std::function<Result<std::unique_ptr<ConfiguredFeatureBase>>(const nlohmann::json& configJson)>;

    static FeatureTypeRegistry& instance();

    /**
     * @brief 注册特征类型工厂
     *
     * @param type 特征类型名（不含 minecraft: 前缀，如 "monster_room"）
     * @param factory 工厂函数
     */
    void registerType(const std::string& type, Factory factory);

    /**
     * @brief 按类型名构造配置化特征
     *
     * 严格报错：type 未注册时返回 Error。
     *
     * @param type 特征类型名（可带或不带 minecraft: 前缀）
     * @param configJson config 字段的 JSON 对象
     * @return 构造的特征，或错误
     */
    [[nodiscard]] Result<std::unique_ptr<ConfiguredFeatureBase>> create(
        const std::string& type, const nlohmann::json& configJson) const;

    /**
     * @brief 是否已注册指定类型
     */
    [[nodiscard]] bool has(const std::string& type) const noexcept;

    /**
     * @brief 清除所有已注册工厂
     */
    void clear() noexcept;

private:
    FeatureTypeRegistry() = default;
    ~FeatureTypeRegistry() = default;
    FeatureTypeRegistry(const FeatureTypeRegistry&) = delete;
    FeatureTypeRegistry& operator=(const FeatureTypeRegistry&) = delete;

    std::unordered_map<std::string, Factory> m_factories;
};

/**
 * @brief 注册当前已实现的内置 feature type 工厂
 *
 * 在 ConfiguredFeatureLoader 加载数据包前调用，使对应 type 的 configured_feature JSON
 * 能被解析。未注册的 type 在 ConfiguredFeatureLoader 中会 warn + skip 该文件。
 * 随着更多 feature type 落地，在此逐步追加 registerType 调用。
 */
void initializeBuiltinFeatureTypes();

} // namespace world::gen::feature
} // namespace mc
