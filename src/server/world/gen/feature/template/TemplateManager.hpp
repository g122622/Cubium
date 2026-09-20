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
 * 模板来源按优先级依次为：基岩版结构包资源源（GameTest 行为包 .mcstructure）
 * → DataPackRepository（数据包内的 structure/<path>.nbt）→ 文件系统兜底目录。
 *
 * 【生命周期】进程内由 JigsawAssembler::s_templateManager 持有唯一实例（见
 * JigsawAssembler::getTemplateManager），而来源指针指向宿主（服务端）持有的对象。
 * 因此来源必须经 TemplateManagerHostBinding 绑定，宿主销毁时由令牌自动解绑，
 * 否则单例会残留悬垂指针（表现为 SIGSEGV 或 std::system_error("mutex lock failed")）。
 */
class TemplateManager {
public:
    TemplateManager();
    ~TemplateManager();

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
    // 模板来源（数据包仓库 / 结构包资源源）全部是宿主对象的非拥有指针，
    // 只允许经 TemplateManagerHostBinding 注入与解绑：令牌把"绑定—解绑"收敛为 RAII，
    // 宿主析构即自动解绑，从结构上排除"宿主已销毁而单例仍持悬垂指针"的用法。
    friend class TemplateManagerHostBinding;

    [[nodiscard]] std::unique_ptr<Template> _loadTemplate(const ResourceLocation& location);

    /**
     * @brief 绑定数据包仓库（模板 .nbt 的主来源）
     *
     * 绑定新仓库时会一并丢弃上一宿主遗留的模板缓存，因为模板内容取决于当前数据包集合。
     */
    void _bindDataPackRepository(const resource::DataPackRepository* dataPackList);

    /**
     * @brief 解绑数据包仓库
     * @return 当前绑定确实来自 dataPackList 并已解绑时为 true
     */
    [[nodiscard]] bool _unbindDataPackRepository(const resource::DataPackRepository* dataPackList);

    /**
     * @brief 绑定基岩版结构包资源源
     *
     * 用于从基岩版行为包加载 .mcstructure 结构（GameTest 场景）。优先级最高，
     * 高于 DataPackRepository 与文件系统（Java .nbt 路径）。
     * 实现方经 IStructurePackSource 抽象解耦，TemplateManager 不直接依赖 BehaviorPack 类型。
     */
    void _bindStructurePackSource(const IStructurePackSource* structurePackSource);

    /**
     * @brief 解绑结构包资源源
     * @return 当前绑定确实来自 structurePackSource 并已解绑时为 true
     */
    [[nodiscard]] bool _unbindStructurePackSource(const IStructurePackSource* structurePackSource);

    /** @brief 清空模板缓存（丢弃上一宿主数据包视图下加载的全部模板） */
    void _clearTemplateCache();

    std::unordered_map<ResourceLocation, std::unique_ptr<Template>> m_templates;
    mutable std::mutex m_mutex;
    std::unique_ptr<Template> m_emptyTemplate;
    const resource::DataPackRepository* m_dataPackList = nullptr;
    const IStructurePackSource* m_structurePackSource = nullptr;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
