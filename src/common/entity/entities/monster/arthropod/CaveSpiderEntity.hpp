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
#include "SpiderEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
class LivingEntity;

/**
 * @brief 洞穴蜘蛛实体
 *
 * 在废弃矿井中生成的小型蜘蛛变种。
 *
 * 特性：
 * - 中毒：攻击会造成中毒效果
 * - 小型：比普通蜘蛛更小
 * - 爬墙：可以爬墙
 * - 黑暗中敌对：继承蜘蛛的光照敏感攻击特性
 */
class CaveSpiderEntity : public SpiderEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    CaveSpiderEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~CaveSpiderEntity() override = default;

    // 禁止拷贝
    CaveSpiderEntity(const CaveSpiderEntity&) = delete;
    CaveSpiderEntity& operator=(const CaveSpiderEntity&) = delete;

    // 允许移动
    CaveSpiderEntity(CaveSpiderEntity&&) = delete;
    CaveSpiderEntity& operator=(CaveSpiderEntity&&) = delete;

    /**
     * @brief 创建洞穴蜘蛛实体
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    // ========== 攻击 ==========

    /**
     * @brief 作为生物攻击实体
     *
     * 洞穴蜘蛛攻击会造成中毒效果：
     * - 简单难度：无中毒
     * - 普通难度：7秒中毒I
     * - 困难难度：15秒中毒I
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    // ========== 中毒攻击 ==========

    /**
     * @brief 攻击是否会中毒
     */
    [[nodiscard]] bool canPoison() const { return true; }

    /**
     * @brief 获取中毒持续时间（秒）
     * @note 返回的是默认值，实际持续时间取决于游戏难度
     */
    [[nodiscard]] i32 getPoisonDuration() const { return m_poisonDuration; }

    /**
     * @brief 设置中毒持续时间
     */
    void setPoisonDuration(i32 duration) { m_poisonDuration = duration; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 0.45f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.7f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 0.5f; }

protected:
    void registerAttributes() override;

    /**
     * @brief 获取环境音效
     *
     * 洞穴蜘蛛复用普通蜘蛛的环境音，对齐原版 CaveSpider（继承 Spider.getAmbientSound）。
     * sounds.json 中无 entity.cave_spider.*（洞穴蜘蛛共享 spider.* 音效），
     * 故不能走默认 makeSoundEventId("ambient")（会拼接出 cave_spider.ambient）。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

private:
    i32 m_poisonDuration = 7; // 默认7秒中毒（普通难度）
};

} // namespace mc
