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

#include "IStructurePackSource.hpp"
#include "Template.hpp"
#include "TemplateLoader.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mc {

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 模板管理器
 *
 * 管理结构模板的加载、缓存和访问。
 * 支持从资源包加载 .nbt 格式的结构模板文件。
 * 支持从 DataPackRepository 加载结构模板（优先级高于单个资源包）。
 */
class TemplateManager {
public:
    TemplateManager();
    ~TemplateManager();

    /**
     * @brief 设置资源包
     * @param pack 资源包指针
     */
    void setResourcePack(const IResourcePack* pack);

    /**
     * @brief 设置数据包列表
     *
     * DataPackRepository 的优先级高于单个资源包。模板加载时会优先从 DataPackRepository 加载，
     * 如果 DataPackRepository 中没有找到，则回退到单个资源包或文件系统。
     *
     * @param dataPackList 数据包列表指针
     */
    void setDataPackRepository(const resource::DataPackRepository* dataPackList);

    /**
     * @brief 设置基岩版结构包资源源
     *
     * 用于从基岩版行为包加载 .mcstructure 结构（GameTest 场景）。优先级最高，
     * 高于 DataPackRepository / 单个资源包 / 文件系统（Java .nbt 路径）。
     * 实现方经 IStructurePackSource 抽象解耦，TemplateManager 不直接依赖 BehaviorPack 类型。
     *
     * @param source 结构包资源源指针（非拥有，调用方保证生命周期）
     */
    void setStructurePackSource(const IStructurePackSource* source);

    /**
     * @brief 获取模板（如果不存在则尝试加载）
     * @param location 模板资源位置
     * @return 模板指针，如果加载失败返回 nullptr
     */
    [[nodiscard]] const Template* getTemplate(const ResourceLocation& location);

    /**
     * @brief 获取模板（如果不存在则返回空模板）
     * @param location 模板资源位置
     * @return 模板引用
     */
    [[nodiscard]] const Template& getTemplateDefaulted(const ResourceLocation& location);

    /**
     * @brief 检查模板是否存在
     */
    [[nodiscard]] bool hasTemplate(const ResourceLocation& location) const;

    /**
     * @brief 手动添加模板
     */
    void addTemplate(const ResourceLocation& location, std::unique_ptr<Template> templ);

    /**
     * @brief 清除缓存
     */
    void clear();

    /**
     * @brief 获取缓存大小
     */
    [[nodiscard]] size_t cacheSize() const { return m_templates.size(); }

    /**
     * @brief 创建程序化模板
     * @param name 模板名称
     * @param width 宽度
     * @param height 高度
     * @param depth 深度
     * @return 创建的模板
     */
    [[nodiscard]] std::unique_ptr<Template> createProceduralTemplate(
        const std::string& name, i32 width, i32 height, i32 depth);

private:
    [[nodiscard]] std::unique_ptr<Template> _loadTemplate(const ResourceLocation& location);

    std::unordered_map<ResourceLocation, std::unique_ptr<Template>> m_templates;
    mutable std::mutex m_mutex;
    std::unique_ptr<Template> m_emptyTemplate;
    const IResourcePack* m_resourcePack = nullptr;
    const resource::DataPackRepository* m_dataPackList = nullptr;
    const IStructurePackSource* m_structurePackSource = nullptr;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
