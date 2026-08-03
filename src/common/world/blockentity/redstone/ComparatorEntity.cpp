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

#include "ComparatorEntity.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <algorithm>
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

ComparatorEntity::ComparatorEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Comparator, pos)
    , m_outputSignal(0)
{}

bool ComparatorEntity::load(const nlohmann::json& data)
{
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载输出信号强度
    if (data.contains("OutputSignal") && data["OutputSignal"].is_number()) {
        m_outputSignal = data["OutputSignal"].get<i32>();
        // 验证范围
        m_outputSignal = std::clamp(m_outputSignal, 0, 15);
    }

    return true;
}

void ComparatorEntity::save(nlohmann::json& data) const
{
    BlockEntity::save(data);

    // 保存输出信号强度
    data["OutputSignal"] = m_outputSignal;
}

std::unique_ptr<BlockEntity> ComparatorEntity::clone() const
{
    auto entity = std::make_unique<ComparatorEntity>(m_pos);
    entity->m_outputSignal = m_outputSignal;
    return entity;
}

void ComparatorEntity::setOutputSignal(i32 signal)
{
    MC_ASSERT_RELEASE(signal >= 0 && signal <= 15);
    m_outputSignal = signal;
    setChanged();
}

} // namespace blockentity
} // namespace mc
