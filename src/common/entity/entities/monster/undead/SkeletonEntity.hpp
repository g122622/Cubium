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

#include "../../../../core/Types.hpp"
#include "AbstractSkeletonEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 骷髅实体
 *
 * 使用弓箭进行远程攻击的亡灵怪物。
 *
 * 参考 MC 1.16.5 SkeletonEntity。
 */
class SkeletonEntity : public AbstractSkeletonEntity {
public:
    SkeletonEntity(LegacyEntityType type, EntityId id);
    ~SkeletonEntity() override = default;

    SkeletonEntity(const SkeletonEntity&) = delete;
    SkeletonEntity& operator=(const SkeletonEntity&) = delete;
    SkeletonEntity(SkeletonEntity&&) = default;
    SkeletonEntity& operator=(SkeletonEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] f32 eyeHeight() const override { return 1.74f; }
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.99f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
