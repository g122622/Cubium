/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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
#include "common/world/block/BlockPos.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;

namespace blockentity {

/**
 * @brief 幽匿尖啸体方块实体
 *
 * 接收振动信号并递增警告等级。当警告等级达到阈值（4级）时，
 * 可召唤监守者（Warden）。存储振动系统数据和警告等级。
 *
 * VibrationSystem 的集成采用"附件"模式：
 * - 本类持有 VibrationSystem::Data（序列化/反序列化）
 * - SculkVibrationSystem（服务端）持有 VibrationSystem::User 和 Listener
 * - SculkVibrationManager 在 ServerWorld 中管理附件的生命周期
 *
 * 警告等级和振动系统数据通过 NBT/JSON 序列化保存到存档。
 */
class SculkShriekerBlockEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit SculkShriekerBlockEntity(const BlockPos& pos);

    ~SculkShriekerBlockEntity() override = default;

    // ========== 振动数据访问 ==========

    /**
     * @brief 获取振动系统数据
     */
    [[nodiscard]] gameevent::VibrationSystem::Data& getVibrationData() { return m_vibrationData; }
    [[nodiscard]] const gameevent::VibrationSystem::Data& getVibrationData() const { return m_vibrationData; }

    /**
     * @brief 设置振动系统数据（从存档加载时使用）
     */
    void setVibrationData(gameevent::VibrationSystem::Data data) { m_vibrationData = std::move(data); }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== 方块实体接口 ==========

    /**
     * @brief 获取警告等级
     *
     * 警告等级从 0 递增到最大值，每次接收到振动信号时递增。
     * 当达到阈值时（4级），可以召唤监守者。
     *
     * @return 警告等级 (0-4)
     */
    [[nodiscard]] i32 getWarningLevel() const noexcept { return m_warningLevel; }

    /**
     * @brief 设置警告等级
     * @param level 警告等级
     */
    void setWarningLevel(i32 level) { m_warningLevel = level; }

    /**
     * @brief 检查是否可以召唤监守者
     *
     * 当警告等级达到阈值（4级）且满足其他条件时返回 true。
     *
     * @return 是否可以召唤监守者
     */
    [[nodiscard]] bool canSummonWarden() const { return m_warningLevel >= MAX_WARNING_LEVEL; }

    /**
     * @brief 检查尖啸是否刚结束（用于触发 tryRespond）
     *
     * 当 SHRIEKING 状态到期或方块被移除时设置为 true，
     * 服务端 tick 中检测到此标志后执行响应逻辑。
     */
    [[nodiscard]] bool isShriekingFinished() const { return m_shriekingFinished; }

    /**
     * @brief 设置尖啸结束标志
     * @param finished 是否结束
     */
    void setShriekingFinished(bool finished) { m_shriekingFinished = finished; }

    /// 最大警告等级
    static constexpr i32 MAX_WARNING_LEVEL = 4;

private:
    /// 振动系统数据
    gameevent::VibrationSystem::Data m_vibrationData;

    /// 警告等级 (0-4)
    i32 m_warningLevel = 0;

    /// 尖啸结束标志（SHRIEKING 状态到期或方块被移除时设置，服务端检测后触发 tryRespond）
    bool m_shriekingFinished = false;
};

} // namespace blockentity
} // namespace mc
