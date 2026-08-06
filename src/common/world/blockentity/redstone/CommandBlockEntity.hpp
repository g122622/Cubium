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

#include "../BlockEntity.hpp"
#include "command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "util/math/Vector3.hpp"
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;

namespace blockentity {

/**
 * @brief 命令方块实体模式
 *
 * 定义三种命令方块的行为模式。
 */
enum class CommandBlockMode : u8 {
    /**
     * @brief 序列模式（连锁命令方块）
     *
     * 被前一个命令方块触发后执行。
     */
    Sequence,

    /**
     * @brief 自动模式（循环命令方块）
     *
     * 每游戏刻自动执行。
     */
    Auto,

    /**
     * @brief 红石模式（脉冲命令方块）
     *
     * 红石信号上升沿触发执行。
     */
    Redstone
};

/**
 * @brief 命令方块实体
 *
 * 存储命令方块的命令、执行状态和输出信息。
 * 支持三种模式：脉冲、循环、连锁。
 */
class CommandBlockEntity : public BlockEntity, public command::ICommandSource {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit CommandBlockEntity(const BlockPos& pos);

    /**
     * @brief 构造函数（指定模式）
     * @param pos 方块位置
     * @param mode 命令方块模式
     */
    CommandBlockEntity(const BlockPos& pos, CommandBlockMode mode);

    // ========== BlockEntity 接口 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 从 NBT 加载命令方块数据（基岩版 .mcstructure / Java 版 .nbt 通用）
     *
     * 读取基岩版 block_entity_data 字段：Command / SuccessCount / powered / auto / conditionMet /
     * LPCommandMode（基岩版特有，0=脉冲/1=循环/2=连锁 → 项目 Redstone/Auto/Sequence）/ CustomName /
     * LastOutput / TrackOutput / LastExecution。基岩版与 Java 版字段名（Command/powered/auto 等）
     * 大小写一致，故同一实现兼容两者；LPCommandMode 仅基岩版有，缺失时保持当前 mode。
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override;

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override;
    std::unique_ptr<BlockEntity> clone() const override;

    // ========== ICommandSource 接口 ==========

    void sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid = std::nullopt) override;
    void sendError(const std::string& message) override;
    [[nodiscard]] bool shouldReceiveFeedback() const override;
    [[nodiscard]] bool shouldReceiveErrors() const override;
    [[nodiscard]] bool allowLogging() const override;

    // ========== 命令管理 ==========

    /**
     * @brief 检查是否只有 OP 可以设置 NBT
     *
     * 命令方块的 NBT 数据只能由 OP 级玩家修改，
     * 防止非授权玩家通过物品NBT注入恶意命令。
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    /**
     * @brief 获取存储的命令
     * @return 命令字符串
     */
    [[nodiscard]] const std::string& getCommand() const { return m_command; }

    /**
     * @brief 设置命令
     * @param command 命令字符串
     */
    void setCommand(const std::string& command);

    /**
     * @brief 获取成功计数
     *
     * 成功计数用于比较器输出信号强度。
     *
     * @return 成功计数（0-15）
     */
    [[nodiscard]] i32 getSuccessCount() const { return m_successCount; }

    /**
     * @brief 设置成功计数
     * @param count 成功计数
     */
    void setSuccessCount(i32 count);

    /**
     * @brief 获取最后输出
     * @return 最后输出字符串
     */
    [[nodiscard]] const std::string& getLastOutput() const { return m_lastOutput; }

    /**
     * @brief 设置最后输出
     * @param output 输出字符串
     */
    void setLastOutput(const std::string& output);

    // ========== 自定义名称 ==========

    [[nodiscard]] std::string getCustomName() const override { return m_customName; }
    void setCustomName(const std::string& name) override;

    // ========== 执行控制 ==========

    /**
     * @brief 获取执行模式
     * @return 命令方块模式
     */
    [[nodiscard]] CommandBlockMode getMode() const { return m_mode; }

    /**
     * @brief 设置执行模式
     * @param mode 命令方块模式
     */
    void setMode(CommandBlockMode mode) { m_mode = mode; }

    /**
     * @brief 检查是否自动执行
     *
     * 循环命令方块始终为 true。
     * 连锁命令方块根据设置决定。
     *
     * @return 是否自动执行
     */
    [[nodiscard]] bool isAuto() const { return m_auto; }

    /**
     * @brief 设置自动执行
     * @param autoExec 是否自动执行
     */
    void setAuto(bool autoExec) { m_auto = autoExec; }

    /**
     * @brief 检查是否被红石供电
     * @return 是否被供电
     */
    [[nodiscard]] bool isPowered() const { return m_powered; }

    /**
     * @brief 设置供电状态
     * @param powered 是否被供电
     */
    void setPowered(bool powered);

    /**
     * @brief 检查条件是否满足
     *
     * 条件模式下，检查背后命令方块的成功计数。
     *
     * @return 条件是否满足
     */
    [[nodiscard]] bool isConditionMet() const { return m_conditionMet; }

    /**
     * @brief 检查并设置条件状态
     *
     * 如果方块设置为条件执行，检查背后的命令方块。
     *
     * @param world 世界引用
     * @param facing 方块朝向
     * @param isConditional 是否为条件执行
     * @return 条件是否满足
     */
    bool checkCondition(IWorld& world, Direction facing, bool isConditional);

    // ========== 命令执行 ==========

    /**
     * @brief 触发命令执行
     *
     * 执行存储的命令，更新成功计数和最后输出。
     * 防止同一 tick 内重复执行。
     *
     * @param world 世界引用
     * @return 是否成功执行
     */
    bool trigger(IWorld& world);

    /**
     * @brief 设置追踪输出标志
     * @param track 是否追踪输出
     */
    void setTrackOutput(bool track) { m_trackOutput = track; }

    /**
     * @brief 检查是否追踪输出
     * @return 是否追踪输出
     */
    [[nodiscard]] bool shouldTrackOutput() const { return m_trackOutput; }

private:
    std::string m_command;                                ///< 存储的命令
    i32 m_successCount = 0;                               ///< 成功计数（用于比较器输出）
    std::string m_lastOutput;                             ///< 最后的输出
    std::string m_customName;                             ///< 自定义名称（"@"" 表示默认）
    CommandBlockMode m_mode = CommandBlockMode::Redstone; ///< 执行模式
    bool m_auto = false;                                  ///< 是否自动执行（循环模式）
    bool m_powered = false;                               ///< 是否被红石供电
    bool m_conditionMet = true;                           ///< 条件是否满足
    bool m_trackOutput = true;                            ///< 是否追踪输出
    bool m_updateLastExecution = true;                    ///< 是否更新最后执行时间
    i64 m_lastExecution = -1;                             ///< 最后执行的游戏时间（防止同一 tick 重复执行）
};

} // namespace blockentity
} // namespace mc
