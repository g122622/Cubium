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

#include "../../../../util/math/Vector3.hpp"
#include "../../../../world/block/BlockPos.hpp"

#include <memory>

namespace mc {

class LivingEntity;

namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief Brain 位置目标抽象
 *
 * 对齐 MC 1.16.5 IPosWrapper，用于在记忆中统一保存“看向/走向”的目标。
 */
class IPositionTarget {
public:
    virtual ~IPositionTarget() = default;

    [[nodiscard]] virtual Vector3 getPosition() const = 0;
    [[nodiscard]] virtual BlockPos getBlockPos() const = 0;
    [[nodiscard]] virtual bool isVisibleTo(const LivingEntity& viewer) const = 0;
};

using PositionTargetPtr = std::shared_ptr<IPositionTarget>;

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
