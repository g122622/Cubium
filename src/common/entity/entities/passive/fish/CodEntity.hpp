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

#include "AbstractGroupFishEntity.hpp"

#include <memory>
#include <optional>

namespace mc {

namespace resource {
class ResourceLocation;
}

/**
 * @brief 鳕鱼实体
 *
 * 鳕鱼属于群游鱼类，沿用 AbstractGroupFishEntity 的默认群体大小语义。
 */
class CodEntity : public AbstractGroupFishEntity {
public:
    /**
     * @brief 构造鳕鱼实体
     * @param id 实体 ID
     */
    CodEntity(EntityInstanceId id);
    ~CodEntity() override = default;

    CodEntity(const CodEntity&) = delete;
    CodEntity& operator=(const CodEntity&) = delete;
    CodEntity(CodEntity&&) = delete;
    CodEntity& operator=(CodEntity&&) = delete;

    /**
     * @brief 创建鳕鱼实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.15f; }

    /**
     * @brief 获取扑腾声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getFlopSound() const override;

    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取死亡声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 获取受伤声音
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

protected:
    void registerAttributes() override;
};

} // namespace mc
