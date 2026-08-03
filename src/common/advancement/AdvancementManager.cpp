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

#include "AdvancementManager.hpp"
#include "common/advancement/Advancement.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <utility>

namespace mc::advancement {

AdvancementManager& AdvancementManager::instance()
{
    static AdvancementManager instance;
    return instance;
}

AdvancementManager::AdvancementManager()
{
    m_list.addListener(this);
}

bool AdvancementManager::registerAdvancement(Advancement::Ptr advancement)
{
    return m_list.add(std::move(advancement));
}

bool AdvancementManager::removeAdvancement(const ResourceLocation& id)
{
    return m_list.remove(id);
}

void AdvancementManager::clear()
{
    m_list.clear();
}

Advancement::Ptr AdvancementManager::get(const ResourceLocation& id) const
{
    return m_list.get(id);
}

bool AdvancementManager::contains(const ResourceLocation& id) const
{
    return m_list.contains(id);
}

void AdvancementManager::reload()
{
    m_list.clear();
    // 实际加载由 AdvancementLoader 完成
}

void AdvancementManager::onAdvancementAdded(Advancement::Ptr advancement)
{
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就添加
}

void AdvancementManager::onAdvancementRemoved(Advancement::Ptr advancement)
{
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就移除
}

void AdvancementManager::onAdvancementUpdated(Advancement::Ptr advancement)
{
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就更新
}

} // namespace mc::advancement
