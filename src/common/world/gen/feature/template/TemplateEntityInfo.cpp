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

// ============================================================================
// 注意：本文件不在 CMakeLists.txt 编译列表中，仅供参考。
// 实际实现在 Template.hpp/cpp 中。
// 修改逻辑时，务必修改 Template.hpp/cpp，而非仅修改本文件。
// ============================================================================

#include "TemplateEntityInfo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

// ============================================================================
// TemplateEntityInfo
// ============================================================================

TemplateEntityInfo::TemplateEntityInfo()
    : posx(0.0)
    , posy(0.0)
    , posz(0.0)
    , blockPos()
{}

TemplateEntityInfo::TemplateEntityInfo(const TemplateEntityInfo& other)
    : typeId(other.typeId)
    , posx(other.posx)
    , posy(other.posy)
    , posz(other.posz)
    , blockPos(other.blockPos)
{
    if (other.nbt) {
        nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
    }
}

TemplateEntityInfo::TemplateEntityInfo(TemplateEntityInfo&& other) noexcept
    : typeId(std::move(other.typeId))
    , posx(other.posx)
    , posy(other.posy)
    , posz(other.posz)
    , blockPos(std::move(other.blockPos))
    , nbt(std::move(other.nbt))
{}

TemplateEntityInfo& TemplateEntityInfo::operator=(const TemplateEntityInfo& other)
{
    if (this != &other) {
        typeId = other.typeId;
        posx = other.posx;
        posy = other.posy;
        posz = other.posz;
        blockPos = other.blockPos;
        if (other.nbt) {
            nbt = std::make_unique<nbt::CompoundTag>(*other.nbt);
        } else {
            nbt.reset();
        }
    }
    return *this;
}

TemplateEntityInfo& TemplateEntityInfo::operator=(TemplateEntityInfo&& other) noexcept
{
    if (this != &other) {
        typeId = std::move(other.typeId);
        posx = other.posx;
        posy = other.posy;
        posz = other.posz;
        blockPos = std::move(other.blockPos);
        nbt = std::move(other.nbt);
    }
    return *this;
}

TemplateEntityInfo::~TemplateEntityInfo() = default;

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
