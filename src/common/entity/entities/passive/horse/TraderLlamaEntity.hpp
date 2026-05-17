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

#include "LlamaEntity.hpp"

#include <memory>

namespace mc {

/**
 * @brief 商队羊驼实体
 *
 * 对齐 1.16.5 `TraderLlamaEntity` 的最小支撑层。
 * 当前先补消失倒计时与类型层次，后续再接拴绳、流浪商人仇恨联动
 * 和生成逻辑。
 */
class TraderLlamaEntity : public LlamaEntity {
public:
    /**
     * @brief 构造商队羊驼
     * @param type 实体类型
     * @param id 实体 ID
     */
    TraderLlamaEntity(LegacyEntityType type, EntityId id)
        : LlamaEntity(type, id)
    {}

    ~TraderLlamaEntity() override = default;

    TraderLlamaEntity(const TraderLlamaEntity&) = delete;
    TraderLlamaEntity& operator=(const TraderLlamaEntity&) = delete;
    TraderLlamaEntity(TraderLlamaEntity&&) = default;
    TraderLlamaEntity& operator=(TraderLlamaEntity&&) = default;

    /**
     * @brief 创建商队羊驼
     */
    static std::unique_ptr<Entity> create(IWorld* /*world*/)
    {
        return std::make_unique<TraderLlamaEntity>(LegacyEntityType::TraderLlama, 0);
    }

    /**
     * @brief 当前是否为商队羊驼
     */
    [[nodiscard]] bool isTraderLlama() const { return true; }

    /**
     * @brief 获取消失倒计时
     */
    [[nodiscard]] i32 getDespawnDelay() const { return m_despawnDelay; }

    /**
     * @brief 设置消失倒计时
     */
    void setDespawnDelay(i32 despawnDelay) { m_despawnDelay = despawnDelay; }

    /**
     * @brief 以流浪商人倒计时同步自身倒计时
     */
    void syncDespawnDelayFromTrader(i32 traderDespawnDelay) { m_despawnDelay = traderDespawnDelay - 1; }

    /**
     * @brief 当前是否允许自然消失
     *
     * 先补最小语义：被驯服或被骑乘时不消失。
     * 拴绳和 trader 关联逻辑后续补齐。
     */
    [[nodiscard]] bool canDespawn(double distanceToClosestPlayer) const override
    {
        (void)distanceToClosestPlayer;
        return !isTame() && !isBeingRidden();
    }

    void tick() override
    {
        LlamaEntity::tick();

        if (isTame() || isBeingRidden()) {
            return;
        }

        --m_despawnDelay;
        if (m_despawnDelay <= 0) {
            remove();
        }
    }

private:
    i32 m_despawnDelay = 47999;
};

} // namespace mc
