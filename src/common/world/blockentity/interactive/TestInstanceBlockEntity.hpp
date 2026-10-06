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
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include <string>
#include <vector>

namespace mc {
namespace blockentity {

/**
 * @brief 测试实例方块实体
 *
 * 存储测试实例（GameTest 结构）的元数据：测试标识、结构尺寸、旋转、是否忽略实体、
 * 运行状态与错误信息，以及结构边界内的错误标记列表。
 *
 * 注：vanilla 中该实体还负责放置/导出结构模板、运行测试、绘制信标光束与边界盒
 * （依赖 StructureTemplate 与 gametest 注册表）。本项目对应的结构放置与测试运行
 * 由 GameTestCommand / MinecraftGameTestInstance 承担，此实体聚焦于数据承载与
 * 状态同步，结构操作相关接口留 TODO。
 *
 * 参考: net.minecraft.world.level.block.entity.TestInstanceBlockEntity
 */
class TestInstanceBlockEntity : public BlockEntity {
public:
    /**
     * @brief 测试实例运行状态
     *
     * 对齐 vanilla TestInstanceBlockEntity.Status。
     */
    enum class Status : u8 {
        Cleared = 0, ///< 已清除
        Running = 1, ///< 运行中
        Finished = 2 ///< 已完成
    };

    /**
     * @brief 错误标记：结构内某位置的错误文本
     */
    struct ErrorMarker {
        BlockPos pos;
        std::string text;
    };

    explicit TestInstanceBlockEntity(const BlockPos& pos);

    ~TestInstanceBlockEntity() override = default;

    // ========== 结构位置 ==========

    /// 结构原点相对方块位置的偏移（对齐 vanilla STRUCTURE_OFFSET = (0,1,1)）
    static constexpr i32 STRUCTURE_OFFSET_X = 0;
    static constexpr i32 STRUCTURE_OFFSET_Y = 1;
    static constexpr i32 STRUCTURE_OFFSET_Z = 1;

    /**
     * @brief 结构原点位置（方块位置 + 偏移）
     */
    [[nodiscard]] BlockPos getStructurePos() const;

    // ========== 数据访问 ==========

    [[nodiscard]] const std::string& getTestId() const noexcept { return m_testId; }
    void setTestId(const std::string& testId) { m_testId = testId; }

    [[nodiscard]] i32 getSizeX() const noexcept { return m_sizeX; }
    [[nodiscard]] i32 getSizeY() const noexcept { return m_sizeY; }
    [[nodiscard]] i32 getSizeZ() const noexcept { return m_sizeZ; }
    void setSize(i32 x, i32 y, i32 z)
    {
        m_sizeX = x;
        m_sizeY = y;
        m_sizeZ = z;
    }

    [[nodiscard]] Rotation getRotation() const noexcept { return m_rotation; }
    void setRotation(Rotation rotation) { m_rotation = rotation; }

    [[nodiscard]] bool ignoreEntities() const noexcept { return m_ignoreEntities; }
    void setIgnoreEntities(bool ignore) { m_ignoreEntities = ignore; }

    [[nodiscard]] Status getStatus() const noexcept { return m_status; }
    void setStatus(Status status)
    {
        m_status = status;
        setChanged();
    }

    [[nodiscard]] const std::string& getErrorMessage() const noexcept { return m_errorMessage; }
    void setErrorMessage(const std::string& message)
    {
        m_errorMessage = message;
        m_status = Status::Finished;
        setChanged();
    }

    // ========== 状态便捷方法（对齐 vanilla） ==========

    void setSuccess();
    void setRunning();

    // ========== 错误标记 ==========

    [[nodiscard]] const std::vector<ErrorMarker>& getErrorMarkers() const noexcept { return m_errorMarkers; }
    void markError(const BlockPos& pos, const std::string& text);
    void clearErrorMarkers();

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /// 测试实例方块为管理员方块，NBT 仅允许 OP 修改
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    // ========== 未实现的结构操作（留 TODO） ==========

    // TODO: 实现 placeStructure（放置结构模板）、resetTest/saveTest/exportTest（保存/导出测试）、
    //   runTest（运行测试）、encaseStructure/removeBarriers（结构边界处理）、getBeamSections（信标光束）。
    //   这些依赖 StructureTemplate 与 gametest 实例注册表，需在结构系统接入后补全，
    //   对齐 vanilla TestInstanceBlockEntity 的同名方法。

private:
    std::string m_testId;                    ///< 测试标识（空表示无测试，对齐 Data.test Optional）
    i32 m_sizeX = 0;                         ///< 结构尺寸 X
    i32 m_sizeY = 0;                         ///< 结构尺寸 Y
    i32 m_sizeZ = 0;                         ///< 结构尺寸 Z
    Rotation m_rotation = Rotation::None;    ///< 结构旋转
    bool m_ignoreEntities = false;           ///< 是否忽略实体
    Status m_status = Status::Cleared;       ///< 运行状态
    std::string m_errorMessage;              ///< 错误信息
    std::vector<ErrorMarker> m_errorMarkers; ///< 错误标记
};

} // namespace blockentity
} // namespace mc
