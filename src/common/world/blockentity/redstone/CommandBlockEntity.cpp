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
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/blocks/special/CommandBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/redstone/BedrockCloneFilterData.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include <algorithm>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 把项目命令方块模式映射成基岩 LPCommandMode 的取值
 *
 * 基岩：0=脉冲、1=循环、2=连锁；项目枚举为 Sequence=0 / Auto=1 / Redstone=2，取值不同，
 * 不能直接强转。读取侧有反向映射，两处必须成对维护。
 *
 * @param mode 项目命令方块模式
 * @return 基岩 LPCommandMode 取值
 */
i8 _modeToBedrockByte(blockentity::CommandBlockMode mode)
{
    switch (mode) {
        case blockentity::CommandBlockMode::Auto:
            return 1; // 循环 Repeating
        case blockentity::CommandBlockMode::Sequence:
            return 2; // 连锁 Chain
        case blockentity::CommandBlockMode::Redstone:
        default:
            return 0; // 脉冲 Normal
    }
}
} // namespace

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
    , m_auto(mode == CommandBlockMode::Auto)
{}

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

    // 基岩版特有字段：TickDelay / ExecuteOnFirstTick 决定循环方块的执行节奏，
    // 现已在调度路径生效（见 CommandBlock::tick 与 onUpdated 用 nextTickDelay()）。
    if (auto v = nh::tryGetInt(tag, "TickDelay")) {
        setTickDelay(static_cast<i32>(*v));
    }
    if (auto v = nh::tryGetBool(tag, "ExecuteOnFirstTick")) {
        setExecuteOnFirstTick(*v);
    }
    // 基岩版红石模式开关：LPRedstoneMode 1=需要红石、0=始终保持开启，与项目的 m_auto
    // （true=无需红石、自动执行）取值相反，因此取反后写入。该字段在 tick 路径生效
    // （见 CommandBlock::tick 中 isAuto() 决定是否响应红石脉冲）。
    if (auto v = nh::tryGetByte(tag, "LPRedstoneMode")) {
        setAuto(*v == 0);
    }
    // TODO: LPCondionalMode（条件模式开关）无法在此落地：conditional 存在方块状态的
    //   CONDITIONAL 属性里（见 isConditional），而 load/loadFromNBT 都没有世界参数，
    //   加载期改不了方块状态。要支持需让方块实体加载流程在写入世界后回调一次，按该字段
    //   更新 CONDITIONAL。在此之前该字段读进来会被丢弃。

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

    // 循环方块的执行节奏（基岩特有字段）。只在非默认值时写出，避免给 Java 存档塞冗余键。
    if (m_tickDelay != 0) {
        data["TickDelay"] = m_tickDelay;
    }
    if (!m_executeOnFirstTick) {
        data["ExecuteOnFirstTick"] = false;
    }
    // 模式与红石模式开关：与读取侧的 LPCommandMode / LPRedstoneMode 映射成对。
    // LPRedstoneMode 只在"需要红石"时写出（基岩默认 0 = 始终保持开启）。
    data["LPCommandMode"] = _modeToBedrockByte(m_mode);
    if (!m_auto) {
        data["LPRedstoneMode"] = 1;
    }
}

void CommandBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    // 与 save(nlohmann::json&) 保持同一字段集：两条存档路径写出的内容必须一致，
    // 否则从 NBT 读回的命令方块会丢掉命令、模式与所有开关状态，变成一个空命令方块。
    if (!m_command.empty()) {
        tag.put("Command", m_command);
    }
    if (m_successCount > 0) {
        tag.put("SuccessCount", m_successCount);
    }
    if (!m_customName.empty() && m_customName != "@") {
        tag.put("CustomName", m_customName);
    }
    if (m_trackOutput && !m_lastOutput.empty()) {
        tag.put("LastOutput", m_lastOutput);
    }
    // NBT 没有布尔类型，布尔统一以 Byte 的 0/1 表示（与基岩存档一致）。
    tag.put("TrackOutput", static_cast<i8>(m_trackOutput ? 1 : 0));
    tag.put("powered", static_cast<i8>(m_powered ? 1 : 0));
    tag.put("conditionMet", static_cast<i8>(m_conditionMet ? 1 : 0));
    tag.put("auto", static_cast<i8>(m_auto ? 1 : 0));
    if (m_updateLastExecution && m_lastExecution >= 0) {
        tag.put("LastExecution", m_lastExecution);
    }
    tag.put("UpdateLastExecution", static_cast<i8>(m_updateLastExecution ? 1 : 0));
    if (m_tickDelay != 0) {
        tag.put("TickDelay", m_tickDelay);
    }
    if (!m_executeOnFirstTick) {
        tag.put("ExecuteOnFirstTick", static_cast<i8>(0));
    }
    // 模式必须写出，否则读回来永远是方块类型默认的模式（脉冲/循环/连锁会退化）。
    tag.put("LPCommandMode", static_cast<i8>(_modeToBedrockByte(m_mode)));
    // 与读取侧取反逻辑对称：需要红石时写 1。
    if (!m_auto) {
        tag.put("LPRedstoneMode", static_cast<i8>(1));
    }
}

// 本类刻意不覆写 tick()/needsTick()：命令方块的全部执行逻辑由
// CommandBlock 的计划刻承担（与 vanilla CommandBlockEntity 一致——它同样没有每 tick 的
// tick 覆写）。早期版本覆写过 tick()，但唯一工作是 markConditionMet()，而
// CommandBlock::tick 每次计划刻都会刷新同一份缓存，因而只是纯粹的每 tick 冗余计算。
// 条件缓存的实际刷新点见 CommandBlock::execute / onNeighborChanged 等计划刻路径。
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
    cloned->m_tickDelay = m_tickDelay;
    cloned->m_executeOnFirstTick = m_executeOnFirstTick;
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

void CommandBlockEntity::clearCommand()
{
    m_command.clear();
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

bool CommandBlockEntity::markConditionMet(IWorld& world)
{
    // 对齐 vanilla CommandBlockEntity.markConditionMet：总是把 conditionMet 刷新为当前判定结果。
    m_conditionMet = true;
    if (!isConditional()) {
        return m_conditionMet;
    }
    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        m_conditionMet = false;
        return m_conditionMet;
    }
    // 背后 = FACING 的反方向；背后必须是命令方块（含连锁），其成功计数 > 0 时条件成立。
    // 命令方块（含连锁）的状态恒含 FACING 属性；世界在自身位置返回其它方块状态时视为无方向，
    // 直接判定条件不满足（避免对不含该属性的状态取值）。
    const auto facing = state->getOptional(BlockStateProperties::FACING());
    if (!facing.has_value()) {
        m_conditionMet = false;
        return m_conditionMet;
    }
    const BlockPos behindPos = m_pos.offset(Directions::opposite(*facing));
    const BlockState* behindState = world.getBlockState(behindPos);
    const bool behindIsCommandBlock =
        behindState != nullptr && (dynamic_cast<const blocks::CommandBlock*>(&behindState->getBlock()) != nullptr);
    if (!behindIsCommandBlock) {
        m_conditionMet = false;
        return m_conditionMet;
    }
    BlockEntity* behindEntity = world.getBlockEntity(behindPos);
    m_conditionMet = behindEntity != nullptr && behindEntity->getType() == BlockEntityType::CommandBlock &&
        static_cast<CommandBlockEntity*>(behindEntity)->getSuccessCount() > 0;
    return m_conditionMet;
}

bool CommandBlockEntity::isConditional() const
{
    // 对齐 vanilla CommandBlockEntity.isConditional：读取方块状态的 CONDITIONAL 属性。
    // 该查询依赖世界状态，故在实现中按需读取（无世界时视为非条件）。
    if (m_world == nullptr) {
        return false;
    }
    const BlockState* state = m_world->getBlockState(m_pos);
    if (state == nullptr) {
        return false;
    }
    // 仅命令方块（含连锁）拥有 CONDITIONAL 属性；其它方块状态视为非条件。
    return state->getOptional(BlockStateProperties::CONDITIONAL()).value_or(false);
}

void CommandBlockEntity::setAuto(bool autoExec)
{
    const bool wasAuto = m_auto;
    m_auto = autoExec;
    // 对齐 vanilla CommandBlockEntity.setAutomatic：开启自动执行且此前未开启、当前未被供电、
    // 且模式不为连锁时，调度 1 tick 后的方块刻（循环命令方块开启后立即执行首轮）。
    // 条件状态由方块刻内 CommandBlock::tick 的 markConditionMet 刷新，此处不提前刷新。
    if (!wasAuto && autoExec && !m_powered && m_world != nullptr && m_mode != CommandBlockMode::Sequence) {
        const BlockState* state = m_world->getBlockState(m_pos);
        if (state != nullptr) {
            m_world->tickManager().scheduleBlockTick(m_pos, state->getBlock(), 1, world::tick::TickPriority::High);
        }
    }
}

void CommandBlockEntity::reapplyAutomatic()
{
    // 对齐 vanilla CommandBlock.setPlacedBy：放置/创建时以方块类型的自动执行标志覆盖实体设置。
    // 对齐 vanilla CommandBlock.newBlockEntity：创建实体时直接设定（不走 setAutomatic 的调度分支）。
    m_auto = m_mode == CommandBlockMode::Auto;
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

    // 命令源朝向：基岩版命令方块执行 `^` 局部坐标命令时，forward 固定朝南(+Z)、left 固定朝东(+X)，
    // 即 rotation=(pitch=0, yaw=0)，与方块的 FACING 无关（区别于 Java 版 CommandBlockEntity
    // 用 direction.toYRot()）。GameTest 行为包的 clone 命令据此设计：所有命令方块 `^` forward
    // 均为 +Z。TODO: 若未来支持 Java 结构命令方块，需按结构来源区分 facing-based / 固定朝南两种语义。
    Vector2f rotation(0.0f, 0.0f);

    // 基岩版 clone filtered 语法适配：基岩语法为
    //   clone <b> <e> <d> filtered <mode> <block> [<data>]
    // （mode 紧跟 filtered，block/data 在后），项目命令树采用 Java 语法
    //   clone <b> <e> <d> filtered <block> [<props>] <mode>
    // 此处将基岩语法的 filtered 子句重排为 Java 语法，并把基岩 data 值转化为 Java 方块状态属性
    // 附加到方块名（如 stairs 的 data=3 → [facing=north,half=bottom]），使 filtered clone 能按
    // 朝向等属性精确过滤（vanilla Java clone filtered 仅按 filter 显式属性匹配）。
    // 仅处理 "filtered <mode> <block> [<data>]" 模式（mode ∈ normal|force|move）。
    // TODO: 基岩命令体系完整支持后，此适配可移除。data→props 映射目前仅覆盖 stairs，
    //   其余方块的位布局缺口与原因见 bedrockCloneFilterDataToJavaProps 上方的说明。
    std::string cmdToRun = m_command;
    {
        const std::string marker = " filtered ";
        auto pos = cmdToRun.find(marker);
        if (pos != std::string::npos) {
            std::string head = cmdToRun.substr(0, pos + marker.size() - 1); // 含 " filtered"
            std::string tail = cmdToRun.substr(pos + marker.size());
            // tail 形如 "<mode> <block> <data>" 或 "<mode> <block>"
            std::istringstream ss(tail);
            std::string modeTok, blockTok, dataTok;
            ss >> modeTok >> blockTok >> dataTok;
            if (modeTok == "normal" || modeTok == "force" || modeTok == "move") {
                // 基岩 data → Java 方块状态属性（仅覆盖 stairs：data = weirdo_direction + upside_down*4）
                std::string javaProps = bedrockCloneFilterDataToJavaProps(blockTok, dataTok);
                std::string javaTail = blockTok + javaProps + " " + modeTok;
                cmdToRun = head + " " + javaTail;
            }
        }
    }

    // 执行命令（命令方块的权限级别为 2）
    i32 result = world.executeCommand(cmdToRun, position, 2, rotation);

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
