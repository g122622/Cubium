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

#include "common/network/buffer/RegistryByteBuf.hpp"

namespace mc::network::buffer {

void RegistryByteBuf::writeItemHolder(const Item* item)
{
    // 1.21.11 Item holder 仅 VarInt itemId（空气/空 = 0）；count 与组件 patch 属 ItemStack codec。
    writeVarUInt(item != nullptr ? item->itemId() : 0);
}

Result<const Item*> RegistryByteBuf::readItemHolder()
{
    u32 id = 0;
    MC_TRY_ASSIGN(id, readVarUInt());
    if (id == 0) {
        return static_cast<const Item*>(nullptr);
    }
    if (!hasRegistry()) {
        return Error(
            ErrorCode::InvalidState, "RegistryByteBuf has no bound registry", "RegistryByteBuf::readItemHolder");
    }
    return registry().itemById(id);
}

void RegistryByteBuf::writeBlockStateHolder(const BlockState* state)
{
    // TODO(Phase3): BlockState 在项目内尚无统一整数 stateId getter，此处先用占位。
    //               区块 palette 与 BlockUpdate 的 stateId 编码待 Phase6 对齐 1.21.11 时补全。
    writeVarUInt(0);
    (void)state;
}

Result<const BlockState*> RegistryByteBuf::readBlockStateHolder()
{
    u32 id = 0;
    MC_TRY_ASSIGN(id, readVarUInt());
    if (!hasRegistry()) {
        return Error(ErrorCode::InvalidState, "RegistryByteBuf 未绑定注册表", "RegistryByteBuf::readBlockStateHolder");
    }
    return registry().blockStateById(id);
}

void RegistryByteBuf::writeEntityTypeHolder(const entity::EntityType* type)
{
    writeVarUInt(type != nullptr ? type->id() : 0);
}

Result<const entity::EntityType*> RegistryByteBuf::readEntityTypeHolder()
{
    u32 id = 0;
    MC_TRY_ASSIGN(id, readVarUInt());
    if (id == 0) {
        return static_cast<const entity::EntityType*>(nullptr);
    }
    if (!hasRegistry()) {
        return Error(ErrorCode::InvalidState, "RegistryByteBuf 未绑定注册表", "RegistryByteBuf::readEntityTypeHolder");
    }
    return registry().entityTypeById(id);
}

} // namespace mc::network::buffer
