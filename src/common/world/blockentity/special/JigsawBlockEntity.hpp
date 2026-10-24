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
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/gen/jigsaw/JigsawOrientation.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc {

namespace world::gen::jigsaw {
/// 拼图连接类型
///
/// 与 server 模块 world/gen/jigsaw/JigsawTypes.hpp 中的同名枚举取值一致；因 common 层不能依赖
/// server 层，此处独立定义。两者仅在各自模块内使用，不做跨模块数值互转。
enum class JigsawJointType : u8 {
    Rollable, ///< 可旋转连接
    Aligned   ///< 对齐连接
};
} // namespace world::gen::jigsaw

namespace blockentity {

/**
 * @brief 拼图方块实体
 *
 * 保存拼图方块在结构生成中的连接信息：连接点名（name）、目标名（target）、目标模板池（pool）、
 * 连接类型（joint）、最终态方块（final_state）以及放置/选择优先级。
 * 客户端打开编辑界面时由服务端下发本实体的更新 NBT。
 *
 * 参考：net.minecraft.world.level.block.entity.JigsawBlockEntity
 *
 * @note 未实现（TODO）：`generate`（按目标池拼装结构）依赖 server 模块的 JigsawAssembler，
 *       本项目 common 层方块实体没有访问通道，留待接入。
 */
class JigsawBlockEntity : public BlockEntity {
public:
    /// 未设置连接名/目标名时的默认标识（对齐 vanilla EMPTY_ID）
    static constexpr const char* EMPTY_ID = "minecraft:empty";
    /// final_state 默认值（对齐 vanilla DEFAULT_FINAL_STATE）
    static constexpr const char* DEFAULT_FINAL_STATE = "minecraft:air";

    explicit JigsawBlockEntity(const BlockPos& pos);

    ~JigsawBlockEntity() override = default;

    // ========== BlockEntity 接口 ==========

    bool loadFromNBT(const nbt::CompoundTag& tag) override;
    void saveToNBT(nbt::CompoundTag& tag) const override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /// 拼图方块属于管理员方块：仅 OP 可通过物品 NBT 改写
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override
    {
        return true;
    }

    // ========== 连接信息 ==========

    [[nodiscard]] const std::string& getName() const noexcept
    {
        return m_name;
    }

    void setName(const std::string& name)
    {
        m_name = name;
    }

    [[nodiscard]] const std::string& getTarget() const noexcept
    {
        return m_target;
    }

    void setTarget(const std::string& target)
    {
        m_target = target;
    }

    [[nodiscard]] const std::string& getPool() const noexcept
    {
        return m_pool;
    }

    void setPool(const std::string& pool)
    {
        m_pool = pool;
    }

    [[nodiscard]] const std::string& getFinalState() const noexcept
    {
        return m_finalState;
    }

    void setFinalState(const std::string& finalState)
    {
        m_finalState = finalState;
    }

    [[nodiscard]] world::gen::jigsaw::JigsawJointType getJoint() const noexcept
    {
        return m_joint;
    }

    void setJoint(world::gen::jigsaw::JigsawJointType joint)
    {
        m_joint = joint;
    }

    [[nodiscard]] i32 getPlacementPriority() const noexcept
    {
        return m_placementPriority;
    }

    void setPlacementPriority(i32 priority)
    {
        m_placementPriority = priority;
    }

    [[nodiscard]] i32 getSelectionPriority() const noexcept
    {
        return m_selectionPriority;
    }

    void setSelectionPriority(i32 priority)
    {
        m_selectionPriority = priority;
    }

    /// 连接点朝向（前方方向），供默认 joint 类型推断与结构拼装使用
    [[nodiscard]] Direction frontFacing() const;

    /**
     * @brief 依据前方朝向推断默认连接类型
     *
     * 对齐 vanilla StructureTemplate.getDefaultJointType：前方为水平方向时为 ALIGNED，
     * 竖直方向时为 ROLLABLE。
     */
    [[nodiscard]] world::gen::jigsaw::JigsawJointType defaultJointType() const;

private:
    std::string m_name = EMPTY_ID;
    std::string m_target = EMPTY_ID;
    std::string m_pool = "minecraft:empty";
    std::string m_finalState = DEFAULT_FINAL_STATE;
    world::gen::jigsaw::JigsawJointType m_joint = world::gen::jigsaw::JigsawJointType::Rollable;
    i32 m_placementPriority = 0;
    i32 m_selectionPriority = 0;
};

} // namespace blockentity
} // namespace mc