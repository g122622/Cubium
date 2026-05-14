#pragma once

#include "AbstractFishEntity.hpp"

namespace mc {

/**
 * @brief 群游鱼类实体中间层
 *
 * 对齐 1.16.5 的 AbstractGroupFishEntity，负责保存群首引用、
 * 群体大小和跟随范围。这里先补最小运行语义，后续再把
 * FollowSchoolLeaderGoal 和初始生成分组链路接上。
 */
class AbstractGroupFishEntity : public AbstractFishEntity {
public:
    /**
     * @brief 构造群游鱼类实体
     * @param type 实体类型
     * @param id 实体 ID
     */
    AbstractGroupFishEntity(LegacyEntityType type, EntityId id)
        : AbstractFishEntity(type, id)
    {}

    ~AbstractGroupFishEntity() override = default;

    AbstractGroupFishEntity(const AbstractGroupFishEntity&) = delete;
    AbstractGroupFishEntity& operator=(const AbstractGroupFishEntity&) = delete;
    AbstractGroupFishEntity(AbstractGroupFishEntity&&) = default;
    AbstractGroupFishEntity& operator=(AbstractGroupFishEntity&&) = default;

    [[nodiscard]] bool canSchool() const override { return true; }

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
        leader.increaseGroupSize();
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

        m_groupLeader->decreaseGroupSize();
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
     * MC 1.16.5: func_212810_a
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
     *
     * MC 1.16.5: moveToGroupLeader
     */
    void moveToGroupLeader();

    void tick() override
    {
        AbstractFishEntity::tick();
        if (m_groupLeader != nullptr && !m_groupLeader->isAlive()) {
            m_groupLeader = nullptr;
        }
    }

private:
    void increaseGroupSize() { ++m_groupSize; }

    void decreaseGroupSize()
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
