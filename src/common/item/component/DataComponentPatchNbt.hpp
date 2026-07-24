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

#include "common/core/Result.hpp"
#include "common/item/component/DataComponentMap.hpp"

namespace mc {
namespace nbt {
namespace tags {
struct compound_tag;
} // namespace tags
} // namespace nbt

namespace item {
namespace component {

/**
 * @brief 将 DataComponentPatch 写入 NBT compound（1.21.11 DataComponentPatch.NBT）
 *
 * 写出格式：键为组件资源位置名，'!' 前缀表 removed（值为一个空 compound 占位），
 * 其余键表 added（值为该组件的 NBT 表示）。空 patch 不写入任何键。
 *
 * 仅处理本项目落地的 9 个组件；调用方保证 patch 内 typeId 均在落地子集内
 * （未知 typeId 会被跳过并记为无效，但不抛异常）。
 *
 * @param out 输出 compound（追加键，不清空已有键）
 * @param patch 组件补丁
 */
void writePatchToNbt(nbt::tags::compound_tag& out, const DataComponentPatch& patch);

/**
 * @brief 从 NBT compound 读取 DataComponentPatch
 *
 * 读入格式同 writePatchToNbt。未知组件名（未落地子集）跳过；值解析失败的字段跳过。
 * 调用方须保证传入的是合法的 patch compound。
 *
 * @param tag NBT compound
 * @return 解析出的 patch（解析失败的字段被跳过，整体不报错）
 */
[[nodiscard]] DataComponentPatch readPatchFromNbt(const nbt::tags::compound_tag& tag);

} // namespace component
} // namespace item
} // namespace mc
