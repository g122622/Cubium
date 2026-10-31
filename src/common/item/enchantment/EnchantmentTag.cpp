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

#include "EnchantmentTag.hpp"

namespace mc {

EnchantmentTag::EnchantmentTag(ResourceLocation id) noexcept
    : m_id(std::move(id))
{}

void EnchantmentTag::add(const ResourceLocation& enchantmentId)
{
    m_enchantmentIds.insert(enchantmentId);
}

bool EnchantmentTag::contains(const ResourceLocation& enchantmentId) const noexcept
{
    return m_enchantmentIds.find(enchantmentId) != m_enchantmentIds.end();
}

bool EnchantmentTag::contains(const std::string& enchantmentId) const
{
    return contains(ResourceLocation(enchantmentId));
}

void EnchantmentTag::clear()
{
    m_enchantmentIds.clear();
}

} // namespace mc
