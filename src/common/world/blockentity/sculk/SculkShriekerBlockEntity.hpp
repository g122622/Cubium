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

#include "common/world/gameevent/VibrationSystem.hpp"
#include "world/blockentity/BlockEntity.hpp"

namespace mc {

class IWorld;

namespace blockentity {

/**
 * @brief 幽匿尖啸体方块实体
 *
 * 接收振动信号并递增警告等级。当警告等级达到阈值（4级）时，
 * 可召唤监守者（Warden）。存储振动系统数据和警告等级。
 * VibrationSystem 的集成（User/Listener/Ticker）在服务端目录中实现，
 * 避免方块实体对 ServerWorld 的编译依赖。
 *
 * 警告等级和振动系统数据通过 NBT/JSON 序列化保存到存档，
 * 与 MC 原版 SculkShriekerBlockEntity 的序列化格式兼容。
 *
 * 参考: net.minecraft.block.entity.SculkShriekerBlockEntity
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
     * @brief 递增警告等级
     *
     * 每次振动到达时调用，警告等级最大为 MAX_WARNING_LEVEL。
     * 达到阈值时会触发监守者召唤逻辑。
     *
     * @return 递增后的警告等级
     */
    i32 incrementWarningLevel();

    /**
     * @brief 检查是否可以召唤监守者
     *
     * 当警告等级达到阈值（4级）且满足其他条件时返回 true。
     *
     * @return 是否可以召唤监守者
     */
    [[nodiscard]] bool canSummonWarden() const { return m_warningLevel >= MAX_WARNING_LEVEL; }

    /// 最大警告等级
    static constexpr i32 MAX_WARNING_LEVEL = 4;

private:
    /// 振动系统数据
    gameevent::VibrationSystem::Data m_vibrationData;

    /// 警告等级 (0-4)，对齐 MC 原版 SculkShriekerBlockEntity.warningLevel
    i32 m_warningLevel = 0;
};

} // namespace blockentity
} // namespace mc
