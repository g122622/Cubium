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

#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "util/assert/AssertMacros.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class BlockState;
class IWorld;
class Entity;
class CopperGolemEntity;

namespace blockentity {

/**
 * @brief 铜傀儡雕像方块实体
 *
 * 铜傀儡雕像的方块实体，主要用于：
 * - 存储 CUSTOM_NAME 组件（雕像的自定义名称，由铜傀儡转化时保留）
 * - 提供 removeStatue() 接口（用于斧头敲击 Unaffected 等级雕像时生成铜傀儡）
 *
 * MC Java 1.21.11 行为参考：
 * - 铜傀儡雕像放置时创建方块实体
 * - 铜傀儡变雕像时（createStatue）将铜傀儡的自定义名称保存到方块实体的 CUSTOM_NAME 组件
 * - 玩家用斧头敲击 Unaffected 等级雕像时（removeStatue）生成铜傀儡并转移 CUSTOM_NAME
 * - 物品拾取（getCloneItemStack）通过方块实体将 CUSTOM_NAME 和 POSE 状态写入物品
 *
 * 参考: net.minecraft.world.level.block.entity.CopperGolemStatueBlockEntity (MC 1.21.11)
 */
class CopperGolemStatueBlockEntity : public BlockEntity {
public:
    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit CopperGolemStatueBlockEntity(const BlockPos& pos)
        : BlockEntity(BlockEntityType::CopperGolemStatue, pos)
    {}

    ~CopperGolemStatueBlockEntity() noexcept override = default;

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称
     * @return 自定义名称，如果没有返回空字符串
     *
     * 对应 MC Java: components().get(DataComponents.CUSTOM_NAME)
     */
    [[nodiscard]] std::string getCustomName() const override { return m_customName; }

    /**
     * @brief 设置自定义名称
     * @param name 名称
     *
     * 对应 MC Java: setComponents(...set(DataComponents.CUSTOM_NAME, name))
     */
    void setCustomName(const std::string& name) override
    {
        m_customName = name;
        setChanged();
    }

    // ========== 铜傀儡生成 ==========

    /**
     * @brief 移除雕像并生成铜傀儡
     *
     * 对应 MC Java: CopperGolemStatueBlockEntity.removeStatue(BlockState)
     *
     * 在 MC Java 中，当玩家用斧头敲击 Unaffected 等级的铜傀儡雕像时：
     * 1. 创建 CopperGolem 实体（EntityType.COPPER_GOLEM.create(...)）
     * 2. 转移 CUSTOM_NAME 到新生成的铜傀儡
     * 3. 设置铜傀儡的位置（pos.getCenter()）和朝向（与雕像的 FACING 一致）
     * 4. 播放生成音效（coppergolem.playSpawnSound()）
     * 5. 由调用方负责将实体加入世界并移除方块
     *
     * @param state 当前方块状态（用于获取 FACING）
     * @return 生成的铜傀儡实体（所有权转移给调用方），失败返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Entity> removeStatue(const BlockState& state);

    // ========== 序列化 ==========

    /**
     * @brief 从 JSON 加载数据
     * @param data JSON 数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override
    {
        if (!BlockEntity::load(data)) {
            return false;
        }

        // 加载自定义名称（JSON 格式使用 custom_name 键）
        if (data.contains("custom_name") && data["custom_name"].is_string()) {
            m_customName = data["custom_name"].get<std::string>();
        }

        return true;
    }

    /**
     * @brief 保存数据到 JSON
     * @param data 输出 JSON 数据
     */
    void save(nlohmann::json& data) const override
    {
        BlockEntity::save(data);

        if (!m_customName.empty()) {
            data["custom_name"] = m_customName;
        }
    }

    /**
     * @brief 从 NBT 加载数据（用于结构模板）
     * @param tag NBT 复合标签
     * @return 是否成功
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override
    {
        if (!BlockEntity::loadFromNBT(tag)) {
            return false;
        }

        // 加载自定义名称（NBT 格式使用 CustomName 键）
        auto nameOpt = entity::serialization::nbt_helper::tryGetString(tag, "CustomName");
        if (nameOpt.has_value()) {
            m_customName = nameOpt.value();
        }

        return true;
    }

    /**
     * @brief 保存数据到 NBT（用于结构模板）
     * @param tag 输出 NBT 复合标签
     */
    void saveToNBT(nbt::CompoundTag& tag) const override
    {
        BlockEntity::saveToNBT(tag);

        if (!m_customName.empty()) {
            tag.put("CustomName", m_customName);
        }
    }

    // ========== 克隆 ==========

    /**
     * @brief 克隆方块实体
     * @return 副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override
    {
        auto copy = std::make_unique<CopperGolemStatueBlockEntity>(m_pos);
        copy->m_customName = m_customName;
        return copy;
    }

private:
    /// 自定义名称（对应 MC Java 的 DataComponents.CUSTOM_NAME）
    std::string m_customName;
};

} // namespace blockentity
} // namespace mc
