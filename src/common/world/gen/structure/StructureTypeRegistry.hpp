/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#include "Structure.hpp"
#include "StructureDefinitionLoader.hpp"
#include "common/core/Result.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mc {
namespace world::gen::structure {

/**
 * @brief 结构类型注册表
 *
 * structure 的 type 字符串 → C++ 结构构造工厂映射。
 * StructureDefinitionLoader 解析 JSON 时，读 `type` 字段，调 `create(type, def)` 得到
 * `unique_ptr<Structure>`，再注册到 StructureRegistry。
 *
 * 与 FeatureTypeRegistry 的差异：工厂输入是已解析的 StructureDefinition（而非原始 JSON），
 * 因为 structure JSON 字段已在 Loader 阶段解析为强类型成员（startPool/size/startHeight/
 * poolAliases/terrainAdaptation/step 等）。
 *
 * 数据驱动覆盖语义：工厂构造子类后，从 def 注入 biomes/step/terrainAdaptation
 * （经 Structure 基类 setBiomeTag/setDecorationStage/setTerrainAdaptation），覆盖子类硬编码
 * 默认值；硬编码 initialize() 路径不注入，回退到子类 defaultXxx()，行为不变。
 *
 * 严格报错策略：未注册的 type 返回 Error（中断加载），便于运行时定位未实现的结构 type。
 */
class StructureTypeRegistry {
public:
    /**
     * @brief 结构工厂函数类型
     *
     * 从已解析的结构定义构造结构子类实例。工厂内部按 def.id 分流变体（如 village 5 变体、
     * ruined_portal 7 变体），并从 def 注入数据驱动覆盖字段。
     *
     * @param def 已解析的结构定义
     * @return 构造的结构，或错误信息
     */
    using Factory = std::function<Result<std::unique_ptr<Structure>>(const StructureDefinition& def)>;

    static StructureTypeRegistry& instance();

    /**
     * @brief 注册结构类型工厂
     *
     * @param type 结构类型名（不含 minecraft: 前缀，如 "jigsaw"、"mineshaft"）
     * @param factory 工厂函数
     */
    void registerType(const std::string& type, Factory factory);

    /**
     * @brief 按类型名构造结构
     *
     * 严格报错：type 未注册时返回 Error。
     *
     * @param type 结构类型名（可带或不带 minecraft: 前缀）
     * @param def 已解析的结构定义
     * @return 构造的结构，或错误
     */
    [[nodiscard]] Result<std::unique_ptr<Structure>> create(
        const std::string& type, const StructureDefinition& def) const;

    /**
     * @brief 是否已注册指定类型
     */
    [[nodiscard]] bool has(const std::string& type) const noexcept;

    /**
     * @brief 清除所有已注册工厂
     */
    void clear() noexcept;

private:
    StructureTypeRegistry() = default;
    ~StructureTypeRegistry() = default;
    StructureTypeRegistry(const StructureTypeRegistry&) = delete;
    StructureTypeRegistry& operator=(const StructureTypeRegistry&) = delete;

    std::unordered_map<std::string, Factory> m_factories;
};

/**
 * @brief 注册当前已实现的内置 structure type 工厂
 *
 * 在 StructureDefinitionLoader 加载数据包前调用，使对应 type 的 structure JSON 能被解析。
 * 未注册的 type 在 Loader 中会 warn + skip 该文件。
 * 随着更多 structure type 落地，在此逐步追加 registerType 调用。
 */
void initializeBuiltinStructureTypes();

} // namespace world::gen::structure
} // namespace mc
