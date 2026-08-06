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

#include "CommandBlockEntity.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/core/Types.hpp"
#include "common/entity/serialization/NbtHelper.hpp" // nbt_helper::tryGetString/tryGetBool/tryGetByte/tryGetInt/tryGetLong
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockState.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ============================================================================
// 构造函数
// ============================================================================

CommandBlockEntity::CommandBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::CommandBlock, pos)
    , m_command("")
    , m_customName("@")
{}

CommandBlockEntity::CommandBlockEntity(const BlockPos& pos, CommandBlockMode mode)
    : BlockEntity(BlockEntityType::CommandBlock, pos)
    , m_command("")
    , m_customName("@")
    , m_mode(mode)
{
    // 循环命令方块默认为自动执行
    if (mode == CommandBlockMode::Auto) {
        m_auto = true;
    }
}

// ============================================================================
// BlockEntity 接口实现
// ============================================================================

bool CommandBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载命令
    if (data.contains("Command") && data["Command"].is_string()) {
        m_command = data["Command"].get<std::string>();
    }

    // 加载成功计数
    if (data.contains("SuccessCount") && data["SuccessCount"].is_number()) {
        m_successCount = std::clamp(data["SuccessCount"].get<i32>(), 0, 15);
    }

    // 加载自定义名称
    if (data.contains("CustomName") && data["CustomName"].is_string()) {
        m_customName = data["CustomName"].get<std::string>();
    }

    // 加载最后输出
    if (data.contains("LastOutput") && data["LastOutput"].is_string()) {
        m_lastOutput = data["LastOutput"].get<std::string>();
    }

    // 加载追踪输出
    if (data.contains("TrackOutput") && data["TrackOutput"].is_boolean()) {
        m_trackOutput = data["TrackOutput"].get<bool>();
    }

    // 加载供电状态
    if (data.contains("powered") && data["powered"].is_boolean()) {
        m_powered = data["powered"].get<bool>();
    }

    // 加载条件满足状态
    if (data.contains("conditionMet") && data["conditionMet"].is_boolean()) {
        m_conditionMet = data["conditionMet"].get<bool>();
    }

    // 加载自动执行
    if (data.contains("auto") && data["auto"].is_boolean()) {
        m_auto = data["auto"].get<bool>();
    }

    // 加载最后执行时间
    if (data.contains("LastExecution") && data["LastExecution"].is_number()) {
        m_lastExecution = data["LastExecution"].get<i64>();
    }

    // 加载更新最后执行标志
    if (data.contains("UpdateLastExecution") && data["UpdateLastExecution"].is_boolean()) {
        m_updateLastExecution = data["UpdateLastExecution"].get<bool>();
    }

    return true;
}

bool CommandBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    namespace nh = mc::entity::serialization::nbt_helper;

    // 命令字符串（基岩版与 Java 版字段名一致：Command）
    if (auto v = nh::tryGetString(tag, "Command")) {
        m_command = *v;
    }

    // 成功计数（0-15，比较器输出信号强度）
    if (auto v = nh::tryGetInt(tag, "SuccessCount")) {
        m_successCount = std::clamp(*v, 0, 15);
    }

    // 自定义名称
    if (auto v = nh::tryGetString(tag, "CustomName")) {
        m_customName = *v;
    }

    // 最后输出（基岩版存本地化键如 "commands.clone.success"，此处原样保留不解析）
    if (auto v = nh::tryGetString(tag, "LastOutput")) {
        m_lastOutput = *v;
    }

    // 追踪输出
    if (auto v = nh::tryGetBool(tag, "TrackOutput")) {
        m_trackOutput = *v;
    }

    // 供电状态
    if (auto v = nh::tryGetBool(tag, "powered")) {
        m_powered = *v;
    }

    // 条件满足状态
    if (auto v = nh::tryGetBool(tag, "conditionMet")) {
        m_conditionMet = *v;
    }

    // 自动执行（循环模式始终 true）
    if (auto v = nh::tryGetBool(tag, "auto")) {
        m_auto = *v;
    }

    // 最后执行时间
    if (auto v = nh::tryGetLong(tag, "LastExecution")) {
        m_lastExecution = *v;
    }

    // 基岩版特有：LPCommandMode（0=脉冲 Normal / 1=循环 Repeating / 2=连锁 Chain）
    // 项目 CommandBlockMode 枚举值不同（Sequence=0/Auto=1/Redstone=2），需映射。
    if (auto v = nh::tryGetByte(tag, "LPCommandMode")) {
        switch (*v) {
            case 0:
                m_mode = CommandBlockMode::Redstone;
                break; // Normal → 脉冲（红石触发）
            case 1:
                m_mode = CommandBlockMode::Auto;
                break; // Repeating → 循环（自动执行）
            case 2:
                m_mode = CommandBlockMode::Sequence;
                break; // Chain → 连锁
            default:
                break; // 未知值保持当前 mode
        }
    }

    // 基岩版特有字段 ExecuteOnFirstTick / TickDelay / LPCondionalMode / LPRedstoneMode 项目无对应字段，
    // 暂忽略（TODO: 循环方块首 tick 执行 / 延迟执行接线后补）。

    return true;
}

void CommandBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存命令
    if (!m_command.empty()) {
        data["Command"] = m_command;
    }

    // 保存成功计数
    if (m_successCount > 0) {
        data["SuccessCount"] = m_successCount;
    }

    // 保存自定义名称
    if (!m_customName.empty() && m_customName != "@") {
        data["CustomName"] = m_customName;
    }

    // 保存最后输出
    if (m_trackOutput && !m_lastOutput.empty()) {
        data["LastOutput"] = m_lastOutput;
    }

    // 保存追踪输出
    data["TrackOutput"] = m_trackOutput;

    // 保存供电状态
    data["powered"] = m_powered;

    // 保存条件满足状态
    data["conditionMet"] = m_conditionMet;

    // 保存自动执行
    data["auto"] = m_auto;

    // 保存最后执行时间
    if (m_updateLastExecution && m_lastExecution >= 0) {
        data["LastExecution"] = m_lastExecution;
    }

    // 保存更新最后执行标志
    data["UpdateLastExecution"] = m_updateLastExecution;
}

void CommandBlockEntity::tick(IWorld& world)
{
    // 循环命令方块在 tick 中执行
    if (m_mode != CommandBlockMode::Auto) {
        return;
    }

    // 检查是否应该执行
    // 循环命令方块需要被供电或设置为自动执行
    if (!m_powered && !m_auto) {
        return;
    }

    // 检查条件
    if (!m_conditionMet) {
        m_successCount = 0;
        return;
    }

    // 执行命令
    if (!m_command.empty()) {
        trigger(world);
    }
}

bool CommandBlockEntity::needsTick() const noexcept
{
    // 循环命令方块需要每 tick 更新
    return m_mode == CommandBlockMode::Auto;
}

std::unique_ptr<BlockEntity> CommandBlockEntity::clone() const
{
    auto cloned = std::make_unique<CommandBlockEntity>(m_pos, m_mode);
    cloned->m_command = m_command;
    cloned->m_successCount = m_successCount;
    cloned->m_lastOutput = m_lastOutput;
    cloned->m_customName = m_customName;
    cloned->m_auto = m_auto;
    cloned->m_powered = m_powered;
    cloned->m_conditionMet = m_conditionMet;
    cloned->m_trackOutput = m_trackOutput;
    cloned->m_updateLastExecution = m_updateLastExecution;
    cloned->m_lastExecution = m_lastExecution;
    return cloned;
}

// ============================================================================
// ICommandSource 接口实现
// ============================================================================

void CommandBlockEntity::sendMessage(const std::string& message, const std::optional<Uuid>& senderUuid)
{
    MC_UNUSED(senderUuid);
    // 命令方块的输出存储在 lastOutput 中
    if (m_trackOutput) {
        m_lastOutput = message;
        setChanged();
    }
}

void CommandBlockEntity::sendError(const std::string& message)
{
    // 命令方块的错误输出存储在 lastOutput 中
    if (m_trackOutput) {
        m_lastOutput = message;
        setChanged();
    }
}

bool CommandBlockEntity::shouldReceiveFeedback() const
{
    return m_trackOutput;
}

bool CommandBlockEntity::shouldReceiveErrors() const
{
    return true; // 命令方块始终接收错误信息
}

bool CommandBlockEntity::allowLogging() const
{
    return true; // 允许日志记录
}

// ============================================================================
// 命令管理
// ============================================================================

void CommandBlockEntity::setCommand(const std::string& command)
{
    m_command = command;
    m_successCount = 0;
    setChanged();
}

void CommandBlockEntity::setSuccessCount(i32 count)
{
    m_successCount = std::clamp(count, 0, 15);
    setChanged();
}

void CommandBlockEntity::setLastOutput(const std::string& output)
{
    m_lastOutput = output;
    if (m_trackOutput) {
        setChanged();
    }
}

void CommandBlockEntity::setCustomName(const std::string& name)
{
    m_customName = name;
    setChanged();
}

void CommandBlockEntity::setPowered(bool powered)
{
    if (m_powered != powered) {
        m_powered = powered;
        setChanged();
    }
}

bool CommandBlockEntity::checkCondition(IWorld& world, Direction facing, bool isConditional)
{
    m_conditionMet = true;

    if (!isConditional) {
        return true;
    }

    // 获取背后的方块位置（FACING 的反方向）
    BlockPos behindPos = m_pos.offset(Directions::opposite(facing));

    // 获取背后的方块实体
    BlockEntity* behindEntity = world.getBlockEntity(behindPos);
    if (behindEntity == nullptr || behindEntity->getType() != BlockEntityType::CommandBlock) {
        m_conditionMet = false;
        return false;
    }

    // 条件满足：背后命令方块的成功计数 > 0
    auto* behindCommandBlock = static_cast<CommandBlockEntity*>(behindEntity);
    m_conditionMet = behindCommandBlock->getSuccessCount() > 0;

    return m_conditionMet;
}

// ============================================================================
// 命令执行
// ============================================================================

bool CommandBlockEntity::trigger(IWorld& world)
{
    // 检查是否在客户端
    if (world.isClientSide()) {
        return false;
    }

    // 获取当前游戏时间
    i64 currentTick = world.currentTick();

    // 防止同一 tick 重复执行
    if (currentTick == m_lastExecution) {
        return false;
    }

    // 彩蛋：输入 "Searge" 返回 "#itzlipofutzli"
    if (m_command == "Searge") {
        m_lastOutput = "#itzlipofutzli";
        m_successCount = 1;
        setChanged();
        return true;
    }

    // 空命令不执行
    if (m_command.empty()) {
        m_successCount = 0;
        return false;
    }

    // 重置成功计数
    m_successCount = 0;

    // 计算命令执行位置（方块中心）
    Vector3d position(
        static_cast<f64>(m_pos.x) + 0.5, static_cast<f64>(m_pos.y) + 0.5, static_cast<f64>(m_pos.z) + 0.5);

    // 执行命令（命令方块的权限级别为 2）
    i32 result = world.executeCommand(m_command, position, 2);

    // 更新成功计数
    if (result > 0) {
        m_successCount = std::min(result, 15);
    }

    // 更新最后执行时间
    if (m_updateLastExecution) {
        m_lastExecution = currentTick;
    } else {
        m_lastExecution = -1;
    }

    // 更新最后输出
    if (m_successCount > 0) {
        m_lastOutput = "Command executed successfully";
    } else {
        m_lastOutput = "Command failed";
    }

    setChanged();
    return true;
}

} // namespace blockentity
} // namespace mc
