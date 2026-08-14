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

/**
 * @brief 骡实体
 *
 * 骡和驴共用箱子马类中间层，但保留自身"不育"的后代语义。
 */
class MuleEntity : public AbstractChestedHorseEntity {
public:
    /**
     * @brief 构造骡实体
     * @param id 实体 ID
     */
    MuleEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~MuleEntity() override = default;

    MuleEntity(const MuleEntity&) = delete;
    MuleEntity& operator=(const MuleEntity&) = delete;
    MuleEntity(MuleEntity&&) = delete;
    MuleEntity& operator=(MuleEntity&&) = delete;

    /**
     * @brief 创建骡实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    /**
     * @brief 骡不能繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override
    {
        (void)itemStack;
        return false;
    }

    /**
     * @brief 骡不能生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override
    {
        (void)partner;
        return nullptr;
    }

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.5f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

} // namespace mc
