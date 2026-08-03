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
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <memory>

namespace mc {

/**
 * @brief 骷髅实体
 *
 * 使用弓箭进行远程攻击的亡灵怪物。
 */
class SkeletonEntity : public AbstractSkeletonEntity {
public:
    SkeletonEntity(EntityInstanceId id);
    ~SkeletonEntity() override = default;

    SkeletonEntity(const SkeletonEntity&) = delete;
    SkeletonEntity& operator=(const SkeletonEntity&) = delete;
    SkeletonEntity(SkeletonEntity&&) = delete;
    SkeletonEntity& operator=(SkeletonEntity&&) = delete;

    static std::unique_ptr<Entity> create(IWorld* world);

    [[nodiscard]] f32 eyeHeight() const override { return 1.74f; }
    [[nodiscard]] f32 width() const override { return 0.6f; }
    [[nodiscard]] f32 height() const override { return 1.99f; }

    // ========== 流浪者转化 ==========

    /**
     * @brief 是否正在转化为流浪者
     */
    [[nodiscard]] bool isConvertingToStray() const { return m_strayConversionTime > 0; }

    /**
     * @brief 获取转化倒计时（ticks）
     */
    [[nodiscard]] i32 getStrayConversionTime() const { return m_strayConversionTime; }

    /**
     * @brief 开始流浪者转化
     * @param conversionTime 转化时间（ticks）
     */
    void startStrayConversion(i32 conversionTime) { m_strayConversionTime = conversionTime; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

    // ========== NBT 序列化 ==========

    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /// 流浪者转化倒计时（ticks），0 表示未在转化
    i32 m_strayConversionTime = 0;
};

} // namespace mc
