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

#include "common/item/component/DataComponentPatchNbt.hpp"

#include "common/core/Types.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPayloadCodec.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace item {
namespace component {

void writePatchToNbt(nbt::tags::compound_tag& out, const DataComponentPatch& patch)
{
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue; // 未落地组件：跳过
        }
        auto name = componentName(*type);
        if (!name.has_value()) {
            continue;
        }
        auto valueTag = detail::payloadToNbt(*type, entry.value);
        out.value.emplace(std::string(*name), std::move(valueTag));
    }
    for (i32 typeId : patch.removed()) {
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            continue;
        }
        auto name = componentName(*type);
        if (!name.has_value()) {
            continue;
        }
        // removed 键以 '!' 前缀；值为占位空 compound
        out.value.emplace(std::string("!") + std::string(*name), std::make_unique<nbt::tags::compound_tag>());
    }
}

DataComponentPatch readPatchFromNbt(const nbt::tags::compound_tag& tag)
{
    DataComponentPatch patch;
    for (const auto& [key, valueTag] : tag.value) {
        if (key.empty()) {
            continue;
        }
        if (key[0] == '!') {
            auto type = componentTypeByName(key.substr(1));
            if (type.has_value()) {
                patch.remove(*type);
            }
            continue;
        }
        auto type = componentTypeByName(key);
        if (!type.has_value()) {
            continue; // 未知组件名：跳过
        }
        DataComponentPayload payload = detail::nbtToPayload(*type, *valueTag);
        patch.add(*type, std::move(payload));
    }
    return patch;
}

} // namespace component
} // namespace item
} // namespace mc
