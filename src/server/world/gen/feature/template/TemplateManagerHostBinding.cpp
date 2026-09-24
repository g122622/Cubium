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

    if (m_manager._unbindStructurePackSource(source)) {
        // 来源已摘除，缓存中的模板不再对应任何有效数据包视图，一并作废
        m_manager._clearTemplateCache();
    }
}

void TemplateManagerHostBinding::releaseAll()
{
    bool released = false;

    if (m_structurePackSource != nullptr) {
        const IStructurePackSource* source = m_structurePackSource;
        m_structurePackSource = nullptr;
        released = m_manager._unbindStructurePackSource(source) || released;
    }

    if (m_dataPackRepository != nullptr) {
        const resource::DataPackRepository* repository = m_dataPackRepository;
        m_dataPackRepository = nullptr;
        released = m_manager._unbindDataPackRepository(repository) || released;
    }

    if (released) {
        m_manager._clearTemplateCache();
    }
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
