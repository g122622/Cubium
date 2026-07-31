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

#include "common/network/ir/ItemStackBridge.hpp"

#include "common/item/component/DataComponentPatchWire.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/buffer/ByteBuf.hpp"

namespace mc {
namespace network {
namespace ir {

play::ItemStackView toItemStackView(const class ::mc::ItemStack& stack)
{
    play::ItemStackView view{};
    if (stack.isEmpty()) {
        return view; // itemId=0, count=0
    }
    // ItemStackView.itemId 是 wire 上的 vanilla BuiltInRegistries.ITEM 注册序（与项目内部
    // Item::itemId() 无关）。边界处由 JavaItemIdMap 把项目 Item 翻译为 vanilla id，业务侧
    // ItemStack 与 codec 均零感知（贯彻 IR 思想：上层业务零 wire 感知，仅 Java backend 编码
    // 成 vanilla wire）。minecraft:item 注册表不在 23 个 SYNCHRONIZED_REGISTRIES，真客户端
    // 用内置 vanilla core 包按注册序解析 wire itemId。
    const ::mc::Item* item = stack.getItem();
    view.itemId = item != nullptr ? ::mc::network::backend::java::JavaItemIdMap::instance().toJavaRegistryId(*item) : 0;
    view.count = stack.getCount();

    // 导出组件补丁并序列化为 1.21.11 wire 字节。即使 patch 为空也必须写出（vanilla
    // DataComponentPatch.STREAM_CODEC 对空 patch 写 VarInt(0)+VarInt(0)，即 0x00 0x00），
    // 否则 wire 上会缺失 patch 区段，导致真 Java 客户端把后续字段误当 addedCount 解析而错位。
    auto patch = stack.toComponentPatch();
    buffer::ByteBuf buf;
    auto writeResult = item::component::writePatchToWire(buf, patch);
    if (writeResult.success()) {
        view.componentsPatch = buf.bytes();
    }
    return view;
}

Result<class ::mc::ItemStack> fromItemStackView(const play::ItemStackView& view)
{
    if (view.itemId == 0 || view.count <= 0) {
        return ::mc::ItemStack{};
    }
    // wire 上的 itemId 是 vanilla registry id，须经 JavaItemIdMap 反查为项目内部 ItemId。
    const ItemId internalItemId =
        ::mc::network::backend::java::JavaItemIdMap::instance().fromJavaRegistryId(view.itemId);
    Item* item = ItemRegistry::instance().getItem(internalItemId);
    if (item == nullptr) {
        return Error(ErrorCode::InvalidItem, "Unknown item id in ItemStackView: " + std::to_string(view.itemId));
    }

    ::mc::ItemStack stack(*item, view.count);

    // 反序列化组件补丁并应用
    if (!view.componentsPatch.empty()) {
        buffer::ByteBuf buf(view.componentsPatch.data(), view.componentsPatch.size());
        auto patchResult = item::component::readPatchFromWire(buf);
        if (patchResult.failed()) {
            return patchResult.error();
        }
        stack.applyComponentPatch(patchResult.value());
    }
    return stack;
}

} // namespace ir
} // namespace network
} // namespace mc
