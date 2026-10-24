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

#include "common/mod/bedrock/addon/content/item/ItemAddonProperties.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc::mod::bedrock::addon {
namespace {
constexpr const char* _PROPERTIES_TAG = "cubium:addon_dynamic_properties";
} // namespace

Result<AddonDynamicProperties> ItemAddonProperties::read(const mc::ItemStack& item)
{
    if (item.isEmpty()) {
        return Error(ErrorCode::InvalidState, "Item stack is empty");
    }

    AddonDynamicProperties properties;
    const auto* tag = item.getTag();
    if (tag != nullptr) {
        if (const auto found = tag->find(_PROPERTIES_TAG); found != tag->end()) {
            MC_TRY(properties.load(*found));
        }
    }
    return properties;
}

Result<void> ItemAddonProperties::write(mc::ItemStack& item, const AddonDynamicProperties& properties)
{
    if (item.isEmpty()) {
        return Error(ErrorCode::InvalidState, "Item stack is empty");
    }
    if (item.getMaxStackSize() > 1) {
        return Error(ErrorCode::InvalidArgument, "Dynamic properties require a non-stackable item");
    }

    // 复用 custom_data 的保存和传输链路，避免维护与物品副本不同步的外部状态表。
    auto saved = properties.save();
    if (saved.empty()) {
        item.removeChildTag(_PROPERTIES_TAG);
    } else {
        item.getOrCreateTag()[_PROPERTIES_TAG] = std::move(saved);
    }
    return Result<void>::ok();
}

} // namespace mc::mod::bedrock::addon
