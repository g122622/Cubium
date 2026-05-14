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
#include "../../../../resource/ResourceLocation.hpp"
#include "../basic/AnimalEntity.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class Player;
class ItemStack;
class DamageSource;

/**
 * @brief 狐狸实体
 *
 * 具有特殊行为的被动动物。
 *
 * 特性：
 * - 信任机制：可喂食建立信任，但不可驯服
 * - 叼物品：会叼起地上的物品
 * - 狩猎：会攻击鸡、兔子等小动物
 * - 跳跃攻击：跳起来攻击
 * - 睡觉：白天睡觉，晚上活动
 * - 躲避玩家：野生狐狸会躲避玩家
 * - 多种皮肤：红色、白色（雪地变种）
 * - 幼体：小狐狸
 * - 信任玩家：幼狐信任喂养者
 *
 * 参考 MC 1.16.5 FoxEntity
 */
class FoxEntity : public AnimalEntity {
public:
    /**
     * @brief 狐狸皮肤类型
     */
    enum class FoxType : u8 {
        Red = 0, // 红色狐狸
        Snow = 1 // 白色狐狸（雪地变种）
    };

    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    FoxEntity(LegacyEntityType type, EntityId id);
    ~FoxEntity() override = default;

    // 禁止拷贝
    FoxEntity(const FoxEntity&) = delete;
    FoxEntity& operator=(const FoxEntity&) = delete;

    // 允许移动
    FoxEntity(FoxEntity&&) = default;
    FoxEntity& operator=(FoxEntity&&) = default;

    /**
     * @brief 创建狐狸实体
     * @param world 世界实例
     * @return 新的狐狸实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 皮肤类型 ==========

    /**
     * @brief 获取皮肤类型
     */
    [[nodiscard]] FoxType getFoxType() const { return m_foxType; }

    /**
     * @brief 设置皮肤类型
     */
    void setFoxType(FoxType type) { m_foxType = type; }

    // ========== 信任系统 ==========

    /**
     * @brief 是否信任某个玩家
     * @param playerId 玩家ID
     * @return 如果信任返回true
     */
    [[nodiscard]] bool trusts(u64 playerId) const;

    /**
     * @brief 添加信任
     * @param playerId 玩家ID
     */
    void addTrustedPlayer(u64 playerId);

    /**
     * @brief 移除信任
     * @param playerId 玩家ID
     */
    void removeTrustedPlayer(u64 playerId);

    /**
     * @brief 获取第一个信任的玩家
     * @return 信任的玩家ID
     */
    [[nodiscard]] std::optional<u64> getFirstTrustedPlayer() const;

    /**
     * @brief 获取所有信任的玩家列表
     * @return 信任玩家ID列表的常量引用
     */
    [[nodiscard]] const std::vector<u64>& getTrustedPlayers() const { return m_trustedPlayers; }

    // ========== 睡眠状态 ==========

    /**
     * @brief 是否正在睡觉
     */
    [[nodiscard]] bool isSleeping() const { return m_sleeping; }

    /**
     * @brief 设置睡眠状态
     */
    void setSleeping(bool sleeping);

    // ========== 叼物品 ==========

    /**
     * @brief 是否叼着物品
     */
    [[nodiscard]] bool isHoldingItem() const;

    /**
     * @brief 获取叼着的物品
     */
    [[nodiscard]] const ItemStack* getHeldItem() const { return m_heldItem.get(); }

    /**
     * @brief 设置叼着的物品
     */
    void setHeldItem(std::unique_ptr<ItemStack> item);

    /**
     * @brief 丢弃叼着的物品
     */
    void dropHeldItem();

    // ========== 行为 ==========

    /**
     * @brief 是否可以扑击
     */
    [[nodiscard]] bool canPounce() const { return m_canPounce; }

    /**
     * @brief 设置是否可以扑击
     */
    void setCanPounce(bool canPounce) { m_canPounce = canPounce; }

    /**
     * @brief 是否正在扑击
     */
    [[nodiscard]] bool isPouncing() const { return m_pouncing; }

    /**
     * @brief 设置扑击状态
     */
    void setPouncing(bool pouncing) { m_pouncing = pouncing; }

    // ========== 繁殖 ==========

    /**
     * @brief 检查物品是否可用于繁殖
     */
    [[nodiscard]] bool isBreedingItem(const ItemStack& itemStack) const override;

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return isChild() ? 0.2f : 0.4f; }

    // ========== 音效 ==========

    /**
     * @brief 获取环境音效
     * 白狐使用不同的叫声
     */
    [[nodiscard]] std::optional<ResourceLocation> getAmbientSound() const override;

    /**
     * @brief 获取受伤音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;

    /**
     * @brief 获取死亡音效
     */
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;

    /**
     * @brief 播放睡觉音效
     */
    void playSleepSound();

    /**
     * @brief 播放嗅探音效
     */
    void playSniffSound();

    /**
     * @brief 播放咬音效
     */
    void playBiteSound();

    /**
     * @brief 播放进食音效
     */
    void playEatSound();

    /**
     * @brief 播放吐出物品音效
     */
    void playSpitSound();

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

    // ========== 刻更新 ==========
    void tick() override;

private:
    // 皮肤类型
    FoxType m_foxType = FoxType::Red;

    // 信任的玩家（最多2个）
    std::vector<u64> m_trustedPlayers;

    // 睡眠状态
    bool m_sleeping = false;
    i32 m_sleepTimer = 0;

    // 叼着的物品
    std::unique_ptr<ItemStack> m_heldItem;

    // 扑击状态
    bool m_canPounce = false;
    bool m_pouncing = false;
    f32 m_pounceTargetX = 0.0f;
    f32 m_pounceTargetZ = 0.0f;

    // 常量
    static constexpr size_t MAX_TRUSTED_PLAYERS = 2;
};

} // namespace mc
