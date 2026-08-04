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
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/passive/water/WaterMobEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class LivingEntity;

/**
 * @brief 海豚实体
 *
 * 生活在海洋中的聪明哺乳动物。
 *
 * 特性：
 * - 游泳：快速游泳，可以跳出水面
 * - 寻找宝藏：喂食鱼后会引导玩家到宝藏（沉船或海底废墟）
 * - 与玩家同游：跟随游泳玩家并给予"海豚的恩惠"效果
 * - 玩物品：会拾取水中物品并扔出玩耍
 * - 掉落：生鳕鱼
 *
 * AI 目标（MC 1.16.5 优先级）：
 * - 0: BreatheAirGoal - 浮出水面呼吸
 * - 0: FindWaterGoal - 寻找水源
 * - 1: SwimToTreasureGoal - 游向宝藏
 * - 2: SwimWithPlayerGoal - 与玩家同游
 * - 4: RandomSwimmingGoal - 随机游泳
 * - 4: LookRandomlyGoal - 随机看向
 * - 5: LookAtGoal - 看向玩家
 * - 5: DolphinJumpGoal - 跳出水面
 * - 6: MeleeAttackGoal - 近战攻击
 * - 8: PlayWithItemsGoal - 玩物品
 * - 8: FollowBoatGoal - 跟随船
 * - 9: AvoidEntityGoal<GuardianEntity> - 避开守卫者
 *
 * 音效：
 * - ENTITY_DOLPHIN_AMBIENT: 陆地环境音
 * - ENTITY_DOLPHIN_AMBIENT_WATER: 水中环境音
 * - ENTITY_DOLPHIN_ATTACK: 攻击音效
 * - ENTITY_DOLPHIN_DEATH: 死亡音效
 * - ENTITY_DOLPHIN_EAT: 进食音效
 * - ENTITY_DOLPHIN_HURT: 受伤音效
 * - ENTITY_DOLPHIN_JUMP: 跳跃落水音效
 * - ENTITY_DOLPHIN_PLAY: 玩耍物品音效
 * - ENTITY_DOLPHIN_SPLASH: 溅水音效
 * - ENTITY_DOLPHIN_SWIM: 游泳音效
 */
class DolphinEntity : public WaterMobEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    DolphinEntity(EntityInstanceId id);
    ~DolphinEntity() override = default;

    // 禁止拷贝
    DolphinEntity(const DolphinEntity&) = delete;
    DolphinEntity& operator=(const DolphinEntity&) = delete;

    // 允许移动
    DolphinEntity(DolphinEntity&&) = delete;
    DolphinEntity& operator=(DolphinEntity&&) = delete;

    /**
     * @brief 创建海豚实体
     * @param world 世界实例
     * @return 新的海豚实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /// 本类继承链标识（parent = WaterMobEntity::classInfo()）。见 Entity::classInfo()。
    // vanilla 1.21.11 Dolphin 经 AgeableWaterCreature→AgeableMob，字段 BABY@16(继承)/
    // GOT_FISH@17(Boolean)/MOISTNESS_LEVEL@18(Int)。项目 WaterMobEntity 不经 AgeableEntity
    // （无 BABY），故在 DolphinEntity 层补 BABY 占位 + GOT_FISH/MOISTNESS 对齐 vanilla id。
    static const entity::EntityClassInfo& classInfo();

    // ========== 游泳行为 ==========

    /**
     * @brief 是否正在跳跃
     */
    [[nodiscard]] bool isJumping() const noexcept { return m_jumping; }

    /**
     * @brief 设置跳跃状态
     */
    void setJumping(bool jumping) noexcept override { m_jumping = jumping; }

    /**
     * @brief 是否可以跳出水
     */
    [[nodiscard]] bool canJumpOutOfWater() const;

    // ========== 宝藏寻找 ==========

    /**
     * @brief 获取宝藏位置
     */
    [[nodiscard]] const BlockPos& getTreasurePos() const noexcept { return m_treasurePos; }

    /**
     * @brief 设置宝藏位置
     */
    void setTreasurePos(const BlockPos& pos);

    /**
     * @brief 是否有宝藏目标
     */
    [[nodiscard]] bool hasTreasureTarget() const noexcept { return m_hasTreasure; }

    /**
     * @brief 清除宝藏目标
     */
    void clearTreasureTarget();

    /**
     * @brief 是否正在引导玩家
     */
    [[nodiscard]] bool isGuidingPlayer() const noexcept { return m_guidingPlayer; }

    /**
     * @brief 设置引导玩家状态
     */
    void setGuidingPlayer(bool guiding, u64 playerId = 0);

    /**
     * @brief 获取被引导的玩家ID
     */
    [[nodiscard]] u64 getGuidedPlayerId() const noexcept { return m_guidedPlayerId; }

    // ========== 鱼标记 ==========

    /**
     * @brief 是否得到了鱼
     *
     * 当玩家喂食鱼后设置为 true，海豚会引导玩家到宝藏。
     */
    [[nodiscard]] bool hasGotFish() const noexcept { return m_gotFish; }

    /**
     * @brief 设置得到鱼标记
     *
     * 同步写入 DATA_GOT_FISH 元数据（id17），客户端据此渲染海豚寻宝引导状态。
     */
    void setGotFish(bool gotFish)
    {
        m_gotFish = gotFish;
        m_dataManager.set(DATA_GOT_FISH_PARAM, gotFish);
    }

    // ========== 食物 ==========

    /**
     * @brief 检查物品是否是食物
     * 海豚吃鱼（鳕鱼、鲑鱼、河豚、热带鱼）
     */
    [[nodiscard]] bool isFoodItem(const ItemStack& itemStack) const;

    // ========== 导航辅助 ==========

    /**
     * @brief 检查是否接近导航目标
     */
    [[nodiscard]] bool closeToTarget() const;

    /**
     * @brief 检查是否有路径
     */
    [[nodiscard]] bool hasPath() const;

    /**
     * @brief 清除导航路径
     */
    void clearNavigationPath();

    /**
     * @brief 尝试移动到实体
     * @param entity 目标实体
     * @param speed 移动速度
     * @return 是否成功开始移动
     */
    bool tryMoveToEntity(const Entity& entity, f64 speed);

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const noexcept override { return 0.3f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const noexcept override { return 0.9f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const noexcept override { return 0.6f; }

    /**
     * @brief 获取最大空气供应量
     * 海豚有 4800 tick (4分钟) 的空气储备
     */
    [[nodiscard]] i32 maxAir() const noexcept override { return MAX_AIR; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 在水中和陆地播放不同的音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 播放攻击音效
     */
    void playAttackSound(LivingEntity& target) override;

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 同步数据注册 ==========
    // 派生类构造函数须显式调用 registerData()（C++ 基类构造期虚函数不派发，参考 SquidEntity）。
    void registerData() override;

    // ========== 水状态回调 ==========
    void onLeaveWater() override;

private:
    // 跳跃状态
    bool m_jumping = false;

    // 宝藏寻找
    BlockPos m_treasurePos;
    bool m_hasTreasure = false;
    bool m_guidingPlayer = false;
    u64 m_guidedPlayerId = 0;
    i32 m_guideTimer = 0;

    // 鱼标记
    bool m_gotFish = false;

    // 游泳计时器
    i32 m_swimTimer = 0;

    // ========== 同步数据参数（vanilla 1.21.11 Dolphin 字段，见 registerData） ==========
    // id16 占位对齐 vanilla AgeableMob.DATA_BABY（海豚幼体语义暂未实现，占位恒 false）。
    static entity::DataParameter<bool> DATA_BABY_PLACEHOLDER_PARAM; // id16
    // id17 GOT_FISH（玩家喂鱼后置 true，海豚引导玩家寻宝）。由 setGotFish 同步。
    static entity::DataParameter<bool> DATA_GOT_FISH_PARAM; // id17
    // id18 MOISTNESS_LEVEL（湿润度，vanilla 起始 2400）。TODO: 离水时递减的业务联动暂未实现。
    static entity::DataParameter<i32> DATA_MOISTNESS_LEVEL_PARAM; // id18

    // 常量
    static constexpr i32 GUIDE_DURATION = 1200;         // 60秒引导时间 (ticks)
    static constexpr f32 SWIM_SPEED = 0.6f;             // 游泳速度
    static constexpr f32 JUMP_VELOCITY = 0.7f;          // 跳跃速度
    static constexpr i32 MAX_AIR = 4800;                // 4分钟空气储备 (ticks)
    static constexpr i32 SWIM_JUMP_INTERVAL = 200;      // 游泳跳跃检查间隔 (ticks)
    static constexpr i32 JUMP_CHANCE_DENOMINATOR = 100; // 跳跃概率分母
    static constexpr f64 CLOSE_DISTANCE = 12.0;         // 接近目标的距离阈值（方块）
};

} // namespace mc
