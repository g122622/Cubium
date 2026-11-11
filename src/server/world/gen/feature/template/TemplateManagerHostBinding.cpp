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

#include "TemplateManagerHostBinding.hpp"
#include "TemplateManager.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

TemplateManagerHostBinding::TemplateManagerHostBinding(TemplateManager& manager)
    : m_manager(manager)
{}

TemplateManagerHostBinding::~TemplateManagerHostBinding()
{
    releaseAll();
}

void TemplateManagerHostBinding::bindDataPackRepository(const resource::DataPackRepository& repository)
{
    m_manager._bindDataPackRepository(&repository);
    m_dataPackRepository = &repository;
}

void TemplateManagerHostBinding::bindStructurePackSource(const IStructurePackSource& source)
{
    m_manager._bindStructurePackSource(&source);
    m_structurePackSource = &source;
}

void TemplateManagerHostBinding::unbindStructurePackSource()
{
    if (m_structurePackSource == nullptr) {
        return;
    }

    const IStructurePackSource* source = m_structurePackSource;
    m_structurePackSource = nullptr;

    // 只摘除来源指针，**不清空模板缓存**。
    // 解绑发生在服务端关闭阶段，此时区块生成 worker 仍在跑（下界化石等结构的后处理会读
    // 已缓存的 Template*）。清空缓存会把这些 Template 立即析构，worker 随即解引用悬垂指针
    // （表现为 nextInt bound>0 断言、访问违例或栈溢出）。
    // 缓存的 Template 是从数据包解析出的自包含数据，不再引用来源对象，因此保留是安全的；
    // 下一个宿主在 initializeRegistries 里 bind 新仓库时再整体作废旧视图。
    static_cast<void>(m_manager._unbindStructurePackSource(source));
}

void TemplateManagerHostBinding::releaseAll()
{
    if (m_structurePackSource != nullptr) {
        const IStructurePackSource* source = m_structurePackSource;
        m_structurePackSource = nullptr;
        static_cast<void>(m_manager._unbindStructurePackSource(source));
    }

    if (m_dataPackRepository != nullptr) {
        const resource::DataPackRepository* repository = m_dataPackRepository;
        m_dataPackRepository = nullptr;
        static_cast<void>(m_manager._unbindDataPackRepository(repository));
    }

    // 只解绑非拥有指针，**不清空模板缓存**：本方法在服务端关闭阶段被调用，此时区块生成
    // worker 可能仍持有缓存中 Template 的裸指针。清理动作留给下一个宿主 bind 新仓库时执行
    // （见 _bindDataPackRepository），那里发生在任何区块生成之前。
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
