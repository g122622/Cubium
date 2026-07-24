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

#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"

namespace mc {
namespace nbt {
namespace tags {
struct tag;
} // namespace tags
} // namespace nbt

namespace item {
namespace component {
namespace detail {

/// 单组件 payload → NBT tag（NBT patch 与 wire 共用）
[[nodiscard]] std::unique_ptr<nbt::tags::tag> payloadToNbt(DataComponentType type, const DataComponentPayload& payload);

/// NBT tag → 单组件 payload（NBT patch 与 wire 共用）
[[nodiscard]] DataComponentPayload nbtToPayload(DataComponentType type, const nbt::tags::tag& tag);

} // namespace detail
} // namespace component
} // namespace item
} // namespace mc
