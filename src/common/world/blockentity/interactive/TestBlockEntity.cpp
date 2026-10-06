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

#include "TestBlockEntity.hpp"

#include "common/entity/serialization/NbtHelper.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <spdlog/spdlog.h>
#include <utility>

namespace mc {
namespace blockentity {

namespace {

/// 模式名（对齐 vanilla TestBlockMode.getSerializedName）
const char* _modeName(BlockStateProperties::TestBlockMode mode)
{
    switch (mode) {
        case BlockStateProperties::TestBlockMode::Start:
            return "start";
        case BlockStateProperties::TestBlockMode::Log:
            return "log";
        case BlockStateProperties::TestBlockMode::Fail:
            return "fail";
        case BlockStateProperties::TestBlockMode::Accept:
            return "accept";
        default:
            return "fail";
    }
}

} // namespace

TestBlockEntity::TestBlockEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::TestBlock, pos)
{}

BlockStateProperties::TestBlockMode TestBlockEntity::getMode() const
{
    // 权威值存放在方块状态 MODE 属性；方块实体未挂载世界时回退默认 Fail。
    const BlockState* state = getBlockState();
    if (state != nullptr && state->hasProperty(BlockStateProperties::TEST_BLOCK_MODE())) {
        return state->get(BlockStateProperties::TEST_BLOCK_MODE());
    }
    return BlockStateProperties::TestBlockMode::Fail;
}

void TestBlockEntity::setMode(BlockStateProperties::TestBlockMode mode)
{
    IWorld* world = getWorld();
    if (world == nullptr) {
        return;
    }
    const BlockState* state = world->getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }
    if (state->hasProperty(BlockStateProperties::TEST_BLOCK_MODE()) &&
        state->get(BlockStateProperties::TEST_BLOCK_MODE()) != mode) {
        const BlockState& next = state->with(BlockStateProperties::TEST_BLOCK_MODE(), mode);
        world->setBlockState(m_pos, &next);
    }
}

void TestBlockEntity::trigger()
{
    IWorld* world = getWorld();
    const BlockStateProperties::TestBlockMode mode = getMode();

    if (mode == BlockStateProperties::TestBlockMode::Start && world != nullptr) {
        // 对齐 vanilla：START 置为充能并通知邻居（不再标记 triggered）
        setPowered(true);
        if (const BlockState* state = world->getBlockState(m_pos)) {
            world->updateNeighbors(m_pos, state->getBlockMutable());
        }
        log();
        return;
    }

    if (mode == BlockStateProperties::TestBlockMode::Log) {
        log();
    }
    m_triggered = true;
}

void TestBlockEntity::reset()
{
    m_triggered = false;

    if (getMode() == BlockStateProperties::TestBlockMode::Start) {
        setPowered(false);
        IWorld* world = getWorld();
        if (world != nullptr) {
            if (const BlockState* state = world->getBlockState(m_pos)) {
                world->updateNeighbors(m_pos, state->getBlockMutable());
            }
        }
    }
}

void TestBlockEntity::log() const
{
    if (!m_message.empty()) {
        spdlog::info("Test {} (at {}): {}", _modeName(getMode()), m_pos.toString(), m_message);
    }
}

bool TestBlockEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    if (data.contains("message") && data["message"].is_string()) {
        m_message = data["message"].get<std::string>();
    }
    if (data.contains("powered") && data["powered"].is_boolean()) {
        m_powered = data["powered"].get<bool>();
    }
    return true;
}

void TestBlockEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    data["mode"] = _modeName(getMode());
    data["message"] = m_message;
    data["powered"] = m_powered;
}

bool TestBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!BlockEntity::loadFromNBT(tag)) {
        return false;
    }

    auto messageOpt = entity::serialization::nbt_helper::tryGetString(tag, "message");
    if (messageOpt.has_value()) {
        m_message = messageOpt.value();
    }
    auto poweredOpt = entity::serialization::nbt_helper::tryGetBool(tag, "powered");
    if (poweredOpt.has_value()) {
        m_powered = poweredOpt.value();
    }
    return true;
}

void TestBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    BlockEntity::saveToNBT(tag);

    tag.put("mode", std::string(_modeName(getMode())));
    tag.put("message", m_message);
    tag.put("powered", static_cast<i8>(m_powered ? 1 : 0));
}

std::unique_ptr<BlockEntity> TestBlockEntity::clone() const
{
    auto copy = std::make_unique<TestBlockEntity>(m_pos);
    copy->m_message = m_message;
    copy->m_powered = m_powered;
    copy->m_triggered = m_triggered;
    return copy;
}

} // namespace blockentity
} // namespace mc
