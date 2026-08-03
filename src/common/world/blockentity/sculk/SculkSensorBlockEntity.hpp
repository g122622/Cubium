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
 * @brief 幽匿感测体方块实体
 *
 * 检测振动并输出红石信号的方块实体。存储振动系统数据和最后振动频率。
 *
 * VibrationSystem 的集成采用"附件"模式：
 * - 本类持有 VibrationSystem::Data（序列化/反序列化）
 * - SculkVibrationSystem（服务端）持有 VibrationSystem::User 和 Listener
 * - SculkVibrationManager 在 ServerWorld 中管理附件的生命周期
 *
 * 振动系统数据通过 NBT/JSON 序列化保存到存档，键名为 "listener"，
 * 与 MC 原版 SculkSensorBlockEntity 的序列化格式兼容。
 *
 * 参考: net.minecraft.block.entity.SculkSensorBlockEntity
 */
class SculkSensorBlockEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit SculkSensorBlockEntity(const BlockPos& pos);

    ~SculkSensorBlockEntity() override = default;

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
     * @brief 获取最后接收的振动频率
     *
     * 频率 0 表示没有振动，1-15 对应不同类型的游戏事件。
     * 用于红石信号输出和共鸣事件触发。
     *
     * @return 振动频率 (0-15)
     */
    [[nodiscard]] i32 getLastVibrationFrequency() const noexcept { return m_lastVibrationFrequency; }

    /**
     * @brief 设置最后接收的振动频率
     * @param frequency 振动频率 (0-15)
     */
    void setLastVibrationFrequency(i32 frequency) { m_lastVibrationFrequency = frequency; }

private:
    /// 振动系统数据
    gameevent::VibrationSystem::Data m_vibrationData;

    /// 最后接收的振动频率（0-15），用于红石信号输出
    i32 m_lastVibrationFrequency = 0;
};

} // namespace blockentity
} // namespace mc
