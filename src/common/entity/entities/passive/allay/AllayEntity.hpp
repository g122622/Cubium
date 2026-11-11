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

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/passive/basic/AnimalEntity.hpp"

#include <memory>

namespace mc {

class IWorld;

namespace entity {

/**
 * @brief 悦灵（minecraft:allay）
 *
 * 1.19 加入的被动飞行生物：被玩家给予物品后会四处寻找同类物品并搬运。
 * 尺寸 0.35 × 0.6、生命 20、飞行速度 0.1。
 *
 * TODO(entity-allay): 行为尚未实现——原版的「给予物品后搬运」逻辑依赖
 * Brain 系统（`AllayAi`）。Brain 基建（`common/entity/ai/brain/`）**已经存在**，
 * 村民已完整接入（memory / sensor / activity / task 四层）；
 * 悦灵缺的是它自己的 sensor 与搬运/返回任务，不是底层基建。
 * 因此这里只提供类型、尺寸、属性与基础移动能力，不与物品交互。
 */
class AllayEntity final : public AnimalEntity {
public:
    /**
     * @brief 构造悦灵
     * @param id 实体 ID
     * @param registry 实体注册表
     */
    AllayEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~AllayEntity() override = default;

    /**
     * @brief 工厂方法（供 EntityRegistry 注册使用）
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 繁殖 ==========

    // 原版悦灵不是可繁殖生物（没有任何繁殖/幼年逻辑），这里返回空表示「不产幼体」。
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    void registerAttributes() override;

    // ========== 声音 ==========

    /**
     * @brief 获取环境音
     *
     * 悦灵有两套环境音：手持物品时用 `entity.allay.ambient_with_item`，空手时用
     * `entity.allay.ambient_without_item`。基类的 `entity.<type>.ambient` 拼接在这里不成立。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音
     *
     * 原版用固定常量 `entity.allay.hurt`，不走基类的 `entity.<type>.hurt` 拼接。
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
};

} // namespace entity
} // namespace mc
