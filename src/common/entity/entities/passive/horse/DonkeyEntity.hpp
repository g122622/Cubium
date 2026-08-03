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

#include "AbstractChestedHorseEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"
#include "common/item/core/ItemStack.hpp"

#include <memory>

namespace mc {

// Forward declarations
class HorseEntity;
class MuleEntity;

/**
 * @brief 驴实体
 *
 * 驴属于可装备箱子的马类，和骡共享 AbstractChestedHorseEntity 层。
 */
class DonkeyEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 构造驴实体
     * @param id 实体 ID
     */
    DonkeyEntity(EntityInstanceId id);
    ~DonkeyEntity() override = default;

    DonkeyEntity(const DonkeyEntity&) = delete;
    DonkeyEntity& operator=(const DonkeyEntity&) = delete;
    DonkeyEntity(DonkeyEntity&&) = delete;
    DonkeyEntity& operator=(DonkeyEntity&&) = delete;

    /**
     * @brief 创建驴实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 检查物品是否可用于繁殖
     *
     * 使用基类的食物列表，只有金苹果和金胡萝卜可以触发繁殖。
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查是否可以与另一动物交配
     *
     * 驴可以与驴或马交配。
     */
    [[nodiscard]] bool canMateWith(const AnimalEntity& other) const override;

    /**
     * @brief 生成幼体
     *
     * 驴 + 驴 = 驴，驴 + 马 = 骡。
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.45f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
