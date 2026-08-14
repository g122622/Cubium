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

#include "AbstractFishEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include <vector>

namespace mc {

/**
 * @brief 群游鱼类实体中间层
 *
 * 负责保存群首引用、群体大小和跟随范围。
 * 群游 AI 由 FollowSchoolLeaderGoal 实现。
 *
 * AI 目标（继承 AbstractFishEntity 并添加）:
 * - 优先级 5: FollowSchoolLeaderGoal - 跟随群首
 */
class AbstractGroupFishEntity : public AbstractFishEntity {
public:
    /**
     * @brief 构造群游鱼类实体
     * @param id 实体 ID
     * @param registry 实体注册表（ECS）
     */
    AbstractGroupFishEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
        : AbstractFishEntity(id, registry)
    {}

    ~AbstractGroupFishEntity() override = default;

    AbstractGroupFishEntity(const AbstractGroupFishEntity&) = delete;
    AbstractGroupFishEntity& operator=(const AbstractGroupFishEntity&) = delete;
    AbstractGroupFishEntity(AbstractGroupFishEntity&&) = delete;
    AbstractGroupFishEntity& operator=(AbstractGroupFishEntity&&) = delete;

    /// 本类继承链标识（parent = AbstractFishEntity::classInfo()）。见 Entity::classInfo()。
    // 透传层无自身同步字段，classInfo 仅作父链遍历节点。
    static const entity::EntityClassInfo& classInfo();

    [[nodiscard]] bool canSchool() const override { return true; }

    /**
     * @brief 是否可以随机游泳
     *
     * 群游鱼类只有在没有群首时才能自主游泳。
     * 这样有群首的鱼会跟随群首而不是随机游动。
     *
     * @return 如果没有群首返回 true
     */
    [[nodiscard]] bool canRandomSwim() const override { return !hasGroupLeader(); }

    /**
     * @brief 获取群首
     */
    [[nodiscard]] AbstractGroupFishEntity* getGroupLeader() { return hasGroupLeader() ? m_groupLeader : nullptr; }

    /**
     * @brief 获取群首
     */
    [[nodiscard]] const AbstractGroupFishEntity* getGroupLeader() const
    {
        return hasGroupLeader() ? m_groupLeader : nullptr;
    }

    /**
     * @brief 当前是否已经跟随群首
     */
    [[nodiscard]] bool hasGroupLeader() const { return m_groupLeader != nullptr && m_groupLeader->isAlive(); }

    /**
     * @brief 当前是否是群首
     */
    [[nodiscard]] bool isGroupLeader() const { return m_groupLeader == nullptr && m_groupSize > 1; }

    /**
     * @brief 当前记录的群体大小
     */
    [[nodiscard]] i32 getGroupSize() const { return m_groupSize; }

    /**
     * @brief 获取最大群体大小
     */
    [[nodiscard]] virtual i32 getMaxGroupSize() const { return m_maxGroupSize; }

    /**
     * @brief 设置最大群体大小
     */
    void setMaxGroupSize(i32 maxGroupSize) { m_maxGroupSize = maxGroupSize; }

    /**
     * @brief 获取跟随群首的最大距离
     */
    [[nodiscard]] f32 getSchoolingRange() const { return m_schoolingRange; }

    /**
     * @brief 设置跟随群首的最大距离
     */
    void setSchoolingRange(f32 range) { m_schoolingRange = range; }

    /**
     * @brief 判断当前群体是否还能继续增长
     */
    [[nodiscard]] bool canGroupGrow() const
    {
        const AbstractGroupFishEntity* school = hasGroupLeader() ? m_groupLeader : this;
        return school->isGroupLeader() && school->m_groupSize < school->getMaxGroupSize();
    }

    /**
     * @brief 加入指定群首
     * @return 返回群首，便于后续拼接调用
     */
    AbstractGroupFishEntity& joinGroup(AbstractGroupFishEntity& leader)
    {
        if (&leader == this) {
            return leader;
        }

        leaveGroup();
        m_groupLeader = &leader;
        leader._increaseGroupSize();
        return leader;
    }

    /**
     * @brief 离开当前群体
     */
    void leaveGroup()
    {
        if (m_groupLeader == nullptr) {
            return;
        }

        m_groupLeader->_decreaseGroupSize();
        m_groupLeader = nullptr;
    }

    /**
     * @brief 判断自己是否处于群首跟随范围内
     */
    [[nodiscard]] bool inRangeOfGroupLeader() const
    {
        if (!hasGroupLeader()) {
            return false;
        }

        const f32 range = m_schoolingRange;
        return distanceSqTo(*m_groupLeader) <= range * range;
    }

    /**
     * @brief 招募无首领鱼加入群体
     *
     * 遍历无首领鱼列表，将它们加入当前群体（直到群体满员）
     *
     * @param followers 无首领鱼列表（自己不应该在列表中）
     */
    void recruitFollowers(const std::vector<AbstractGroupFishEntity*>& followers)
    {
        for (AbstractGroupFishEntity* follower : followers) {
            if (canGroupGrow()) {
                follower->joinGroup(*this);
            }
        }
    }

    /**
     * @brief 导航到群首位置
     */
    void moveToGroupLeader();

    void tick() override
    {
        AbstractFishEntity::tick();
        if (m_groupLeader != nullptr && !m_groupLeader->isAlive()) {
            m_groupLeader = nullptr;
        }
    }

protected:
    void registerGoals() override;

private:
    void _increaseGroupSize() { ++m_groupSize; }

    void _decreaseGroupSize()
    {
        if (m_groupSize > 1) {
            --m_groupSize;
        }
    }

    AbstractGroupFishEntity* m_groupLeader = nullptr;
    i32 m_groupSize = 1;
    f32 m_schoolingRange = 11.0f;
    i32 m_maxGroupSize = 8;
};

} // namespace mc
