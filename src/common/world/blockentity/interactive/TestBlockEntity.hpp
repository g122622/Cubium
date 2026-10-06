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
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"

namespace mc {
namespace blockentity {

/**
 * @brief 测试方块实体
 *
 * 存储测试方块（TestBlock）的运行状态：模式缓存（权威值在方块状态 MODE 属性）、
 * 消息文本、是否被红石充能、是否已被触发。
 *
 * 参考: net.minecraft.world.level.block.entity.TestBlockEntity
 */
class TestBlockEntity : public BlockEntity {
public:
    explicit TestBlockEntity(const BlockPos& pos);

    ~TestBlockEntity() override = default;

    // ========== 模式 ==========

    /**
     * @brief 获取当前模式
     *
     * 权威值存放在方块状态 MODE 属性中，此处直接从方块状态读取，
     * 避免构造期（createBlockEntity 先于方块实体挂载世界）读取不到状态。
     */
    [[nodiscard]] BlockStateProperties::TestBlockMode getMode() const;

    /**
     * @brief 设置模式并同步到方块状态
     */
    void setMode(BlockStateProperties::TestBlockMode mode);

    // ========== 充能与触发 ==========

    [[nodiscard]] bool isPowered() const noexcept { return m_powered; }
    void setPowered(bool powered) { m_powered = powered; }

    /**
     * @brief 触发测试方块
     *
     * 对齐 vanilla TestBlockEntity#trigger：
     * - START 模式：置为已充能、通知邻居、记录日志；
     * - LOG 模式：记录日志并标记已触发；
     * - 其他模式：标记已触发。
     */
    void trigger();

    /**
     * @brief 重置测试方块
     *
     * 对齐 vanilla TestBlockEntity#reset：清除触发标记，START 模式额外清除充能并通知邻居。
     */
    void reset();

    [[nodiscard]] bool hasTriggered() const noexcept { return m_triggered; }

    // ========== 消息 ==========

    [[nodiscard]] const std::string& getMessage() const noexcept { return m_message; }
    void setMessage(const std::string& message) { m_message = message; }

    /**
     * @brief 输出日志（对齐 vanilla TestBlockEntity#log）
     */
    void log() const;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /// 测试方块为管理员方块，NBT 仅允许 OP 修改
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

private:
    std::string m_message;    ///< 日志消息
    bool m_powered = false;   ///< 是否被红石充能
    bool m_triggered = false; ///< 是否已被触发
};

} // namespace blockentity
} // namespace mc
