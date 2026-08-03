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

#include "../CriterionTrigger.hpp"
#include "../conditions/EntityPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// Forward declarations
class Entity;
class ServerPlayer;

namespace advancement {

// Forward declaration
class ChanneledLightningTriggerInstance;

/**
 * @brief 引雷附魔触发器
 *
 * 当玩家使用引雷附魔的三叉戟召唤闪电击中实体时触发。
 *
 * JSON 格式示例:
 * @code
 * {
 *   "conditions": {
 *     "victims": [
 *       { "type": "minecraft:zombie" }
 *     ]
 *   }
 * }
 * @endcode
 */
class ChanneledLightningTrigger : public AbstractCriterionTrigger<ChanneledLightningTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:channeled_lightning";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发进度
     * @param player 玩家
     * @param victims 被闪电击中的实体列表
     */
    void trigger(ServerPlayer& player, const std::vector<const Entity*>& victims);

    // trigger() 方法需要在 server 层通过包含 TriggerInstantiation.hpp 来实现
};

/**
 * @brief 引雷附魔触发器实例
 */
class ChanneledLightningTriggerInstance : public CriterionInstance<ChanneledLightningTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:channeled_lightning";

    ChanneledLightningTriggerInstance() = default;

    /**
     * @brief 构造带实体谓词的实例
     * @param victims 实体谓词列表
     */
    explicit ChanneledLightningTriggerInstance(std::vector<EntityPredicate> victims);

    /**
     * @brief 检查是否匹配
     * @param victims 被击中的实体列表
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const std::vector<const Entity*>& victims) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::vector<EntityPredicate>& getVictims() const noexcept { return m_victims; }

private:
    std::vector<EntityPredicate> m_victims;
};

} // namespace advancement
} // namespace mc
