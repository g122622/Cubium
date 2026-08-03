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

#include "EnchantmentRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "enchantments/AllEnchantments.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

// 匿名 namespace：仅在当前编译单元可见
namespace {
// 引用型附魔存储（用于全局静态变量注册的附魔，注册表不拥有所有权）
std::unordered_map<std::string, const mc::item::enchant::Enchantment*> s_enchantmentRefs;
} // namespace

namespace mc {
namespace item {
namespace enchant {

// 静态成员定义
std::unordered_map<std::string, std::unique_ptr<Enchantment>> EnchantmentRegistry::s_enchantments;
bool EnchantmentRegistry::s_initialized = false;
std::recursive_mutex EnchantmentRegistry::s_mutex;

// ============================================================================
// EnchantmentRegistry 实现
// ============================================================================

void EnchantmentRegistry::initialize()
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);

    if (s_initialized) {
        spdlog::warn("EnchantmentRegistry already initialized");
        return;
    }

    spdlog::info("Initializing enchantment registry...");

    // 使用 AllEnchantments::registerAll() 注册所有附魔
    AllEnchantments::registerAll();

    s_initialized = true;
    spdlog::info("Registered {} enchantments", s_enchantments.size() + s_enchantmentRefs.size());
}

bool EnchantmentRegistry::registerEnchantment(std::unique_ptr<Enchantment> enchantment)
{
    if (!enchantment) {
        spdlog::error("Cannot register null enchantment");
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(s_mutex);
    return registerEnchantmentInternal(std::move(enchantment));
}

bool EnchantmentRegistry::registerEnchantment(const Enchantment& enchantment)
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);

    std::string id = enchantment.id();

    if (s_enchantments.find(id) != s_enchantments.end() || s_enchantmentRefs.find(id) != s_enchantmentRefs.end()) {
        spdlog::warn("Enchantment {} already registered", id);
        return false;
    }

    s_enchantmentRefs[id] = &enchantment;
    return true;
}

bool EnchantmentRegistry::registerEnchantmentInternal(std::unique_ptr<Enchantment> enchantment)
{
    // 注意：调用者必须持有 s_mutex 锁
    if (!enchantment) {
        return false;
    }

    std::string id = enchantment->id();

    if (s_enchantments.find(id) != s_enchantments.end() || s_enchantmentRefs.find(id) != s_enchantmentRefs.end()) {
        spdlog::warn("Enchantment {} already registered", id);
        return false;
    }

    s_enchantments[id] = std::move(enchantment);
    return true;
}

const Enchantment* EnchantmentRegistry::get(const std::string& id)
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);

    auto it = s_enchantments.find(id);
    if (it != s_enchantments.end()) {
        return it->second.get();
    }

    auto refIt = s_enchantmentRefs.find(id);
    if (refIt != s_enchantmentRefs.end()) {
        return refIt->second;
    }
    return nullptr;
}

bool EnchantmentRegistry::has(const std::string& id)
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);
    return s_enchantments.find(id) != s_enchantments.end() || s_enchantmentRefs.find(id) != s_enchantmentRefs.end();
}

const std::unordered_map<std::string, std::unique_ptr<Enchantment>>& EnchantmentRegistry::all()
{
    return s_enchantments;
}

std::vector<const Enchantment*> EnchantmentRegistry::getByType(EnchantmentType type)
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);

    std::vector<const Enchantment*> result;
    for (const auto& [id, enchantment] : s_enchantments) {
        if (enchantment->type() == type) {
            result.push_back(enchantment.get());
        }
    }
    for (const auto& [id, enchantment] : s_enchantmentRefs) {
        if (enchantment->type() == type) {
            result.push_back(enchantment);
        }
    }
    return result;
}

std::vector<const Enchantment*> EnchantmentRegistry::getAvailableForItem(u32 itemType)
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);

    std::vector<const Enchantment*> result;
    for (const auto& [id, enchantment] : s_enchantments) {
        if (enchantment->canApplyTo(itemType)) {
            result.push_back(enchantment.get());
        }
    }
    for (const auto& [id, enchantment] : s_enchantmentRefs) {
        if (enchantment->canApplyTo(itemType)) {
            result.push_back(enchantment);
        }
    }
    return result;
}

void EnchantmentRegistry::clear()
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);
    s_enchantments.clear();
    s_enchantmentRefs.clear();
    s_initialized = false;
}

bool EnchantmentRegistry::isInitialized()
{
    std::lock_guard<std::recursive_mutex> lock(s_mutex);
    return s_initialized;
}

} // namespace enchant
} // namespace item
} // namespace mc
