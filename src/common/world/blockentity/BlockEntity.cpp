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

#include "world/blockentity/BlockEntity.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/BlockEntityType.hpp"

namespace mc {

const BlockState* BlockEntity::getBlockState() const
{
    if (m_world == nullptr) {
        return nullptr;
    }
    return m_world->getBlockState(m_pos);
}

void BlockEntity::setChanged()
{
    m_changed = true;
    // 子类如 ContainerBlockEntity 会在需要时更新红石比较器
    // 当前基类无需额外操作
}

bool BlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    MC_UNUSED(tag);
    // 基类不处理任何NBT数据
    // 子类应重写此方法以加载自定义数据
    return true;
}

void BlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    // 保存基础信息
    tag.put("id", blockEntityTypeToId(m_type).toString());
    tag.put("x", m_pos.x);
    tag.put("y", m_pos.y);
    tag.put("z", m_pos.z);
    // 子类应重写此方法以保存自定义数据
}

nbt::CompoundTag BlockEntity::getUpdateTag() const
{
    // 默认实现：写入完整状态（含 id/x/y/z 公共字段及子类 saveToNBT 写入的自定义字段）
    nbt::CompoundTag tag;
    saveToNBT(tag);
    return tag;
}

} // namespace mc
