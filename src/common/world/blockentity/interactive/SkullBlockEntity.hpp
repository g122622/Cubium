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
#include "common/resource/ResourceLocation.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/block/BlockPos.hpp"
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class BlockState;

namespace blockentity {

/**
 * @brief 头颅方块实体
 *
 * 存储头颅的所有者档案（玩家头颅的皮肤）、自定义名称，以及龙首/猪灵头的张嘴动画状态。
 * 龙首/猪灵头被红石激活时（POWERED=true）由动画 tick 驱动张嘴。
 *
 * 参考: net.minecraft.world.level.block.entity.SkullBlockEntity
 */
class SkullBlockEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit SkullBlockEntity(const BlockPos& pos);

    ~SkullBlockEntity() noexcept override = default;

    // ========== 所有者档案 ==========

    /**
     * @brief 获取所有者档案（玩家头颅的皮肤数据）
     * @return 档案指针，无则返回 nullptr
     */
    [[nodiscard]] const skin::GameProfile* getOwnerProfile() const noexcept
    {
        return m_owner.has_value() ? &m_owner.value() : nullptr;
    }

    /**
     * @brief 设置所有者档案
     */
    void setOwnerProfile(const skin::GameProfile& profile);

    // ========== 自定义名称 ==========

    [[nodiscard]] std::string getCustomName() const override { return m_customName; }

    void setCustomName(const std::string& name) override;

    // ========== 动画（龙首/猪灵头张嘴） ==========

    /**
     * @brief 每 tick 更新动画状态
     *
     * POWERED 为真时累加动画计数，否则停止动画。对齐 vanilla SkullBlockEntity.animation。
     */
    void tick(IWorld& world) override;

    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    /**
     * @brief 获取当前动画进度
     * @param partialTicks 部分 tick（渲染插值）
     * @return 动画计数（未动画时返回静态计数）
     */
    [[nodiscard]] f32 getAnimation(f32 partialTicks) const noexcept
    {
        return m_isAnimating ? static_cast<f32>(m_animationTickCount) + partialTicks
                             : static_cast<f32>(m_animationTickCount);
    }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    std::optional<skin::GameProfile> m_owner; ///< 所有者档案（玩家头颅皮肤）
    std::string m_customName;                 ///< 自定义名称
    i32 m_animationTickCount = 0;             ///< 动画计数
    bool m_isAnimating = false;               ///< 是否正在动画
};

} // namespace blockentity
} // namespace mc
