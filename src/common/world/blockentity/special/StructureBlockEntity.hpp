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
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc {

namespace blockentity {

/**
 * @brief 结构方块实体
 *
 * 保存结构方块的编辑状态：结构名、坐标偏移、尺寸、镜像/旋转、模式、是否忽略实体、
 * 数据模式下的元数据、完整度与随机种子。客户端打开界面时由服务端下发本实体的更新 NBT。
 *
 * 参考：net.minecraft.world.level.block.entity.StructureBlockEntity
 *
 * @note 未实现（TODO）：保存（SAVE）/加载（LOAD）/扫描（SCAN）三种操作真正读写结构模板。
 *       本项目结构模板体系（TemplateManager，位于 server 模块）与 common 层方块实体之间尚无
 *       访问通道，故 saveStructure/loadStructureInfo/detectSize 目前只维护实体状态并返回结果。
 */
class StructureBlockEntity : public BlockEntity {
public:
    /// 每个轴向上的最大尺寸与偏移（对齐 vanilla MAX_SIZE_PER_AXIS / MAX_OFFSET_PER_AXIS）
    static constexpr i32 MAX_SIZE_PER_AXIS = 48;
    /// 每个轴向上的最大偏移（对齐 vanilla MAX_OFFSET_PER_AXIS）
    static constexpr i32 MAX_OFFSET_PER_AXIS = 48;

    explicit StructureBlockEntity(const BlockPos& pos);

    ~StructureBlockEntity() override = default;

    // ========== BlockEntity 接口 ==========

    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /// 结构方块与拼图方块一样属于管理员方块：仅 OP 可通过物品 NBT 改写
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

    // ========== 结构名 ==========

    /// 结构名（空字符串表示未设置，对齐 vanilla getStructureName）
    [[nodiscard]] const std::string& getStructureName() const noexcept { return m_structureName; }
    [[nodiscard]] bool hasStructureName() const noexcept { return !m_structureName.empty(); }
    void setStructureName(const std::string& name);

    // ========== 作者 / 元数据 ==========

    [[nodiscard]] const std::string& getAuthor() const noexcept { return m_author; }
    void setAuthor(const std::string& author) { m_author = author; }

    [[nodiscard]] const std::string& getMetaData() const noexcept { return m_metaData; }
    void setMetaData(const std::string& metaData) { m_metaData = metaData; }

    // ========== 位置 / 尺寸 ==========

    [[nodiscard]] const BlockPos& getStructurePos() const noexcept { return m_structurePos; }
    void setStructurePos(const BlockPos& pos);

    [[nodiscard]] const BlockPos& getStructureSize() const noexcept { return m_structureSize; }
    void setStructureSize(const BlockPos& size);

    // ========== 镜像 / 旋转 ==========

    [[nodiscard]] Mirror getMirror() const noexcept { return m_mirror; }
    void setMirror(Mirror mirror) { m_mirror = mirror; }

    [[nodiscard]] Rotation getRotation() const noexcept { return m_rotation; }
    void setRotation(Rotation rotation) { m_rotation = rotation; }

    // ========== 开关 ==========

    [[nodiscard]] bool isIgnoreEntities() const noexcept { return m_ignoreEntities; }
    void setIgnoreEntities(bool ignoreEntities) { m_ignoreEntities = ignoreEntities; }

    [[nodiscard]] bool isStrict() const noexcept { return m_strict; }
    void setStrict(bool strict) { m_strict = strict; }

    [[nodiscard]] bool isShowAir() const noexcept { return m_showAir; }
    void setShowAir(bool showAir) { m_showAir = showAir; }

    [[nodiscard]] bool isShowBoundingBox() const noexcept { return m_showBoundingBox; }
    void setShowBoundingBox(bool showBoundingBox) { m_showBoundingBox = showBoundingBox; }

    [[nodiscard]] f32 getIntegrity() const noexcept { return m_integrity; }
    void setIntegrity(f32 integrity);

    [[nodiscard]] i64 getSeed() const noexcept { return m_seed; }
    void setSeed(i64 seed) { m_seed = seed; }

private:
    /// 当前模式（读取方块状态的 STRUCTURE_MODE 属性，方块状态变化后自动同步）
    [[nodiscard]] BlockStateProperties::StructureMode _mode() const;

    std::string m_structureName;
    std::string m_author;
    std::string m_metaData;
    BlockPos m_structurePos{0, 1, 0};
    BlockPos m_structureSize{0, 0, 0};
    Mirror m_mirror = Mirror::None;
    Rotation m_rotation = Rotation::None;
    bool m_ignoreEntities = true;
    bool m_strict = false;
    bool m_showAir = false;
    bool m_showBoundingBox = true;
    f32 m_integrity = 1.0f;
    i64 m_seed = 0;
};

} // namespace blockentity
} // namespace mc