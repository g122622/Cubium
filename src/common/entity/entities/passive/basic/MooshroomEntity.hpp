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
#include "../../../interfaces/IShearable.hpp"
#include "CowEntity.hpp"
#include <memory>

namespace mc {

// Forward declarations
class Player;
class ItemStack;

/**
 * @brief 哞菇实体
 *
 * 长着蘑菇的牛，只生成在蘑菇岛生物群系。
 *
 * 特性：
 * - 两种皮肤：红色哞菇、棕色哞菇
 * - 蘑菇繁殖：被雷击后红色哞菇变为棕色
 * - 剪毛：使用剪刀获得蘑菇并变成普通牛
 * - 碗交互：使用空碗获得蘑菇汤
 * - 繁殖：与普通牛相同
 * - 棕色哞菇：喂食花朵后可产出迷之炖菜
 */
class MooshroomEntity : public CowEntity, public entity::IShearable {
public:
    /**
     * @brief 哞菇类型
     */
    enum class MooshroomType : u8 {
        Red = 0,  // 红色哞菇
        Brown = 1 // 棕色哞菇
    };

    /**
     * @brief 构造函数
     * @param id 实体ID
     */
    MooshroomEntity(EntityId id);
    ~MooshroomEntity() override = default;

    // 禁止拷贝
    MooshroomEntity(const MooshroomEntity&) = delete;
    MooshroomEntity& operator=(const MooshroomEntity&) = delete;

    // 允许移动
    MooshroomEntity(MooshroomEntity&&) = delete;
    MooshroomEntity& operator=(MooshroomEntity&&) = delete;

    /**
     * @brief 创建哞菇实体
     * @param world 世界实例
     * @return 新的哞菇实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 类型 ==========

    /**
     * @brief 获取哞菇类型
     */
    [[nodiscard]] MooshroomType getMooshroomType() const { return m_mooshroomType; }

    /**
     * @brief 设置哞菇类型
     */
    void setMooshroomType(MooshroomType type) { m_mooshroomType = type; }

    /**
     * @brief 是否是红色哞菇
     */
    [[nodiscard]] bool isRed() const { return m_mooshroomType == MooshroomType::Red; }

    /**
     * @brief 是否是棕色哞菇
     */
    [[nodiscard]] bool isBrown() const { return m_mooshroomType == MooshroomType::Brown; }

    // ========== IShearable接口实现 ==========

    /**
     * @brief 检查是否可以被剪毛 (IShearable接口实现)
     * @return 如果有蘑菇返回true
     */
    [[nodiscard]] bool isShearable() const override { return true; }

    /**
     * @brief 剪毛 (IShearable接口实现)
     * @param player 执行剪毛的玩家
     * @return 获得的蘑菇物品
     */
    std::vector<ItemStack> shear(Player* player = nullptr) override;

    // ========== 其他交互 ==========

    /**
     * @brief 检查是否可以用空碗获取蘑菇汤
     * @param itemStack 物品
     * @return 如果是空碗返回true
     */
    [[nodiscard]] bool canBeStewed(const ItemStack& itemStack) const;

    /**
     * @brief 获取蘑菇汤
     * @return 蘑菇汤物品
     */
    ItemStack getStew();

    // ========== 繁殖 ==========

    /**
     * @brief 生成幼体
     */
    std::unique_ptr<AnimalEntity> spawnBaby(AnimalEntity& partner) override;

    // ========== 雷击 ==========

    /**
     * @brief 被雷击时触发
     * 红色哞菇变为棕色哞菇，棕色哞菇变为红色哞菇
     */
    void onStruckByLightning() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

private:
    // 哞菇类型
    MooshroomType m_mooshroomType = MooshroomType::Red;

    // 效果花（棕色哞菇用）
    // 注：迷之炖菜效果系统待效果系统实现后添加
    // EffectInstance m_effect;
};

} // namespace mc
