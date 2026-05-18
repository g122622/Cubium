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
#include "../../passive/water/WaterMobEntity.hpp"
#include "ZombieEntity.hpp"
#include <memory>

namespace mc {

/**
 * @brief 溺尸实体
 *
 * 在水中生成的僵尸变种。
 *
 * 特性：
 * - 水中生成：在海洋和河流中生成
 * - 水中生活：可以在水中呼吸
 * - 三叉戟：有概率手持三叉戟
 * - 溺水：玩家溺水后可能转化为溺尸
 *
 * 参考 MC 1.16.5 DrownedEntity
 */
class DrownedEntity : public ZombieEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    DrownedEntity(EntityId id);

    ~DrownedEntity() override = default;

    // 禁止拷贝
    DrownedEntity(const DrownedEntity&) = delete;
    DrownedEntity& operator=(const DrownedEntity&) = delete;

    // 允许移动
    DrownedEntity(DrownedEntity&&) = default;
    DrownedEntity& operator=(DrownedEntity&&) = default;

    /**
     * @brief 创建溺尸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 水中生活 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否可以游泳
     */
    [[nodiscard]] bool canSwim() const { return true; }

    // ========== 装备 ==========

    /**
     * @brief 是否手持三叉戟
     */
    [[nodiscard]] bool hasTrident() const { return m_hasTrident; }

    /**
     * @brief 设置手持三叉戟
     */
    void setHasTrident(bool trident) { m_hasTrident = trident; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 溺尸不在阳光下燃烧（如果在水中）
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerAttributes() override;

private:
    bool m_hasTrident = false;
};

} // namespace mc
