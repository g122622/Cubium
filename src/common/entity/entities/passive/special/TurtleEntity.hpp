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
#include "../../../../world/block/BlockPos.hpp"
#include "../../../core/DataParameter.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../basic/AnimalEntity.hpp"
#include "common/core/Result.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <memory>
#include <optional>

namespace mc {

// 前向声明
namespace sound {
enum class SoundCategory : u8;
}

namespace blocks {
class TurtleEggBlock;
}

/**
 * @brief 海龟实体
 *
 * 生活在海洋和沙滩的大型被动生物。
 *
 * 特性：
 * - 出生地记忆：海龟会记住出生位置并返回产卵
 * - 产卵：在沙滩上产卵，孵化出小海龟
 * - 游泳：擅长游泳，陆地上缓慢
 * - 婴儿：小海龟受到攻击会害怕
 * - 天敌：僵尸、劫掠者等会攻击海龟蛋
 */
class TurtleEntity : public AnimalEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    TurtleEntity(EntityInstanceId id);
    ~TurtleEntity() override = default;

    // 禁止拷贝
    TurtleEntity(const TurtleEntity&) = delete;
    TurtleEntity& operator=(const TurtleEntity&) = delete;

    // 允许移动
    TurtleEntity(TurtleEntity&&) = delete;
    TurtleEntity& operator=(TurtleEntity&&) = delete;

    /**
     * @brief 创建海龟实体
     * @param world 世界实例
     * @return 新的海龟实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 出生地系统 ==========

    /**
     * @brief 设置出生位置
     * @param pos 出生位置
     *
     * 小海龟孵化后会记住这个位置
     */
    void setHomePos(const BlockPos& pos);

    /**
     * @brief 获取出生位置
     */
    [[nodiscard]] const BlockPos& getHomePos() const { return m_homePos; }

    /**
     * @brief 检查是否有出生位置
     */
    [[nodiscard]] bool hasHomePos() const { return m_hasHomePos; }

    // ========== 产卵状态 ==========

    /**
     * @brief 是否正在产卵
     */
    [[nodiscard]] bool isLayingEgg() const;

    /**
     * @brief 设置产卵状态
     */
    void setLayingEgg(bool laying);

    /**
     * @brief 是否有蛋
     */
    [[nodiscard]] bool hasEgg() const;

    /**
     * @brief 设置是否有蛋
     */
    void setHasEgg(bool hasEgg);

    /**
     * @brief 开始产卵动画
     * 设置产卵状态并重置计时器
     */
    void startLayEgg()
    {
        setLayingEgg(true);
        m_layEggTimer = LAY_EGG_DURATION;
    }

    // ========== 行进状态 ==========

    /**
     * @brief 是否正在前往出生地
     */
    [[nodiscard]] bool isGoingHome() const { return m_goingHome; }

    /**
     * @brief 设置前往出生地状态
     */
    void setGoingHome(bool going) { m_goingHome = going; }

    /**
     * @brief 是否正在旅行（去海里）
     */
    [[nodiscard]] bool isTravelling() const { return m_travelling; }

    /**
     * @brief 设置旅行状态
     */
    void setTravelling(bool travelling) { m_travelling = travelling; }

    // ========== 水陆状态 ==========

    /**
     * @brief 是否在水中
     */
    [[nodiscard]] bool isInWater() const override;

    /**
     * @brief 是否在陆地上
     */
    [[nodiscard]] bool isOnLand() const { return !isInWater(); }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     * 海龟使用海草繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 检查是否可以繁殖
     *
     * 海龟只有在没有蛋的情况下才能繁殖
     */
    [[nodiscard]] bool canBreed() const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.4f; }

    /**
     * @brief 获取环境音效
     *
     * 仅在“不在水中 + 在地面 + 非幼体”三条件同时满足时播放陆地环境音，
     * 否则不播放（水中游泳或幼体均静默），与原版 Turtle.getAmbientSound 一致。
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    // ========== 寻路权重 ==========

    /**
     * @brief 海龟寻路权重
     *
     * 对齐 MC Turtle.getWalkTargetValue：
     * - 非回家状态 + 水中：返回 10.0f（偏好水域）
     * - 沙滩上（脚下是沙子）：返回 10.0f（偏好沙滩产卵）
     * - 其他位置：返回 brightness - 0.5f（基于亮度）
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 处理移动物理
     *
     * 海龟在水中和陆地有不同的移动速度：
     * - 水中：正常速度 0.25，并给予轻微上升动力
     * - 陆地：速度减半，最低 0.06
     * - 幼体在水中：速度再降低
     */
    void travel(const Vector3& travelVec) override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 数据同步 ==========
    void registerData() override;

    // ========== NBT 序列化 ==========
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

private:
    /**
     * @brief 产卵
     *
     * 在当前位置下方放置海龟蛋方块。
     * 需要满足条件：下方是沙子类方块，当前位置为空气。
     */
    void _layEgg();

    /**
     * @brief 检查指定位置下方是否为沙子
     *
     * 对齐 MC TurtleEggBlock.onSand：检查给定位置下方一格是否为沙子类方块。
     *
     * @param world 世界引用
     * @param pos 待检测位置
     * @return true 如果下方是沙子
     */
    [[nodiscard]] static bool _isOnSand(const IWorld& world, const BlockPos& pos);

    // ========== 数据同步参数 ==========
    static entity::DataParameter<bool> DATA_HAS_EGG_PARAM;
    static entity::DataParameter<bool> DATA_LAYING_EGG_PARAM;

protected:
    /// 本类继承链标识（parent = AnimalEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

private:
    // 出生位置
    BlockPos m_homePos;
    bool m_hasHomePos = false;

    // 行进状态
    bool m_goingHome = false;
    bool m_travelling = false;

    // 产卵计时器
    i32 m_layEggTimer = 0;
    static constexpr i32 LAY_EGG_DURATION = 200; // 10秒
};

} // namespace mc
