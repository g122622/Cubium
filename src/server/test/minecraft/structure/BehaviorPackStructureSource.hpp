/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/world/gen/feature/template/IStructurePackSource.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {
class BehaviorPackList;
} // namespace mc::mod::bedrock::addon

namespace mc::test {

/**
 * @brief 基岩版行为包结构资源源
 *
 * 将 `BehaviorPackList` 适配为 `IStructurePackSource`，供 `TemplateManager` 加载
 * 行为包内的 .mcstructure 结构。按基岩版语义解析 `namespace:path` ->
 * `structures/<namespace>/<path>.mcstructure`，遍历已启用行为包按优先级返回首个命中。
 *
 * 持非拥有 `BehaviorPackList*`，调用方保证生命周期（与 ScriptManager 一致）。
 * 线程安全：依赖 BehaviorPackList 内部 shared_mutex；GameTest 场景在主线程加载结构。
 */
class BehaviorPackStructureSource final : public world::gen::feature::template_::IStructurePackSource {
public:
    /**
     * @brief 构造函数
     * @param packList 行为包列表（非拥有，须存活于 TemplateManager 使用期间）
     *
     * 持非 const 引用：readStructure 需调 BehaviorPackList::getEnabledPacks()（该接口无 const 重载）。
     */
    explicit BehaviorPackStructureSource(mc::mod::bedrock::addon::BehaviorPackList& packList);

    [[nodiscard]] Result<std::vector<u8>> readStructure(
        const std::string& namespaceId, const std::string& path) const override;

private:
    mc::mod::bedrock::addon::BehaviorPackList& m_packList;
};

} // namespace mc::test
