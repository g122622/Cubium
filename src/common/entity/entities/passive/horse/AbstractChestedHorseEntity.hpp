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

#include "AbstractHorseEntity.hpp"

namespace mc {

/**
 * @brief 可装备箱子的马类中间层
 *
 * 对齐 1.16.5 `AbstractChestedHorseEntity` 的层次职责，
 * 负责承载驴、骡和羊驼共享的箱子状态与基础库存规模计算。
 */
class AbstractChestedHorseEntity : public AbstractHorseEntity {
public:
    /**
     * @brief 构造可携带箱子的马类实体
     * @param id 实体 ID
     */
    AbstractChestedHorseEntity(EntityId id)
        : AbstractHorseEntity(id)
    {}

    ~AbstractChestedHorseEntity() override = default;

    AbstractChestedHorseEntity(const AbstractChestedHorseEntity&) = delete;
    AbstractChestedHorseEntity& operator=(const AbstractChestedHorseEntity&) = delete;
    AbstractChestedHorseEntity(AbstractChestedHorseEntity&&) = default;
    AbstractChestedHorseEntity& operator=(AbstractChestedHorseEntity&&) = default;

    /**
     * @brief 当前是否装备了箱子
     */
    [[nodiscard]] bool hasChest() const { return m_hasChest; }

    /**
     * @brief 设置箱子状态
     */
    void setChest(bool chest) { m_hasChest = chest; }

    /**
     * @brief 返回箱子库存列数
     *
     * vanilla 驴和骡固定为 5 列，羊驼会覆写成 strength。
     */
    [[nodiscard]] virtual i32 getInventoryColumns() const { return 5; }

    /**
     * @brief 计算库存大小
     *
     * 对齐 vanilla：未装备箱子时回落到 `AbstractHorseEntity` 的基础槽位，
     * 装备箱子后为 2 + 3 * 列数。
     */
    [[nodiscard]] i32 getInventorySize() const override
    {
        if (!m_hasChest) {
            return AbstractHorseEntity::getInventorySize();
        }

        return 2 + 3 * getInventoryColumns();
    }

private:
    bool m_hasChest = false;
};

} // namespace mc
