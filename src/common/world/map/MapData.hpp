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

#include "MapBanner.hpp"
#include "MapDecoration.hpp"
#include "MapFrame.hpp"
#include "core/Types.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include "world/dimension/MapDimensionId.hpp"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mc {
class IWorld;
}

namespace mc::world::map {

/**
 * @brief 地图数据
 *
 * 存储单个地图的全部状态数据，包括地图像素颜色、装饰物、旗帜标记等。
 * 参考: net.minecraft.world.storage.MapData
 */
class MapData {
public:
    /** 地图像素尺寸（128x128） */
    static constexpr i32 MAP_SIZE = 128;

    /** 最大缩放级别 */
    static constexpr i32 MAX_SCALE = 4;

    /** 像素数据总大小 */
    static constexpr i32 COLOR_ARRAY_SIZE = MAP_SIZE * MAP_SIZE;

    /**
     * @brief 地图玩家追踪信息
     *
     * 每个持有地图的玩家都有一个MapInfo实例，用于追踪需要同步的脏区域。
     */
    class MapInfo {
    public:
        explicit MapInfo(i32 playerId);

        /** 标记指定像素为脏 */
        void markDirty(i32 x, i32 y);

        /** 重置脏区域 */
        void resetDirty();

        /** 增加更新步进（用于分批更新地形） */
        void incrementStep() { ++m_step; }

        [[nodiscard]] i32 playerId() const { return m_playerId; }
        [[nodiscard]] i32 step() const { return m_step; }
        [[nodiscard]] bool isDirty() const { return m_isDirty; }
        [[nodiscard]] i32 minX() const { return m_minX; }
        [[nodiscard]] i32 minY() const { return m_minY; }
        [[nodiscard]] i32 maxX() const { return m_maxX; }
        [[nodiscard]] i32 maxY() const { return m_maxY; }

    private:
        i32 m_playerId;
        bool m_isDirty = true;
        i32 m_minX = 0;
        i32 m_minY = 0;
        i32 m_maxX = MAP_SIZE - 1;
        i32 m_maxY = MAP_SIZE - 1;
        i32 m_step = 0;
    };

    /**
     * @brief 默认构造，创建空白地图数据
     */
    MapData() = default;

    /**
     * @brief 带地图ID构造
     */
    explicit MapData(i32 mapId);

    /**
     * @brief 初始化地图中心坐标和缩放
     */
    void initialize(
        i32 xCenter, i32 zCenter, i32 scale, bool trackingPosition, bool unlimitedTracking, MapDimensionId dimension);

    /**
     * @brief 根据世界坐标计算地图中心
     *
     * 地图中心会对齐到缩放级别的网格，使得不同缩放级别的地图覆盖区域一致。
     */
    static void calculateMapCenter(f64 x, f64 z, i32 scale, i32& outCenterX, i32& outCenterZ);

    /**
     * @brief 更新装饰物位置
     *
     * @param type 装饰类型
     * @param world 世界指针（可为nullptr，下界旋转需要）
     * @param decorationName 装饰唯一标识名
     * @param worldX 世界X坐标
     * @param worldZ 世界Z坐标
     * @param rotation 旋转角度（度）
     * @param displayName 可选的显示名称
     */
    void updateDecoration(DecorationType type,
        const IWorld* world,
        const std::string& decorationName,
        f64 worldX,
        f64 worldZ,
        f64 rotation,
        const text::ITextComponent* displayName = nullptr);

    /**
     * @brief 尝试在指定位置添加旗帜标记
     *
     * 检查指定位置是否存在旗帜方块实体，如果存在则在地图上添加/切换旗帜标记。
     * 如果旗帜已存在则移除，如果不存在则添加。同时检查装饰物数量上限。
     * 参考: net.minecraft.world.level.saveddata.maps.MapItemSavedData.toggleBanner
     *
     * @param world 世界引用
     * @param pos 旗帜方块位置
     * @return true 如果成功添加或移除了旗帜标记
     */
    bool tryAddBanner(IWorld& world, const BlockPos& pos);

    /**
     * @brief 移除过期的旗帜标记
     *
     * 检查指定区域内的旗帜标记是否仍然有效（旗帜方块仍然存在且颜色匹配），
     * 移除不再匹配的旗帜标记和对应的装饰物。
     * 参考: net.minecraft.world.level.saveddata.maps.MapItemSavedData.checkBanners
     *
     * @param world 世界引用
     * @param x 区块X坐标
     * @param z 区块Z坐标
     */
    void removeStaleBanners(IWorld& world, i32 x, i32 z);

    /**
     * @brief 添加展示框标记
     */
    void addFrame(const MapFrame& frame);

    /**
     * @brief 移除展示框标记
     */
    void removeFrame(const std::string& frameId);

    /**
     * @brief 直接添加旗帜标记
     *
     * 用于NBT反序列化或测试场景，跳过tryAddBanner的世界交互检查。
     * 同时在地图上添加对应的装饰物。
     *
     * @param banner 旗帜标记数据
     */
    void addBanner(const MapBanner& banner);

    /**
     * @brief 移除指定名称的装饰物
     */
    void removeDecoration(const std::string& decorationName);

    /**
     * @brief 应用客户端装饰（对齐 Java MapItemSavedData.addClientSideDecorations）
     *
     * 清空当前装饰，按 "icon-"+index 加入网络下发的装饰列表。仅客户端使用——
     * 服务端的 tracking 装饰由 updateDecoration 维护，不由此方法覆盖。
     */
    void applyClientDecorations(std::vector<MapDecoration> decorations);

    /**
     * @brief 应用色块补丁（对齐 Java MapPatch.applyToMap）
     *
     * 将 width×height 的 colors 子区域写入 (startX, startY) 起的像素，行优先索引
     * colors[i + j*width] ↔ (startX+i, startY+j)。越界坐标被裁剪。
     */
    void applyColorPatch(i32 startX, i32 startY, i32 width, i32 height, const std::vector<u8>& colors);

    /**
     * @brief 从另一地图数据复制内容（用于锁定）
     */
    void lockFrom(const MapData& source);

    /**
     * @brief 从另一地图数据复制旗帜和装饰
     */
    void copyFrom(const MapData& source);

    /**
     * @brief 标记指定像素为脏
     */
    void markDirty(i32 x, i32 y);

    /**
     * @brief 设置指定位置的颜色
     */
    void setColor(i32 x, i32 y, u8 color);

    /**
     * @brief 获取指定位置的颜色
     */
    [[nodiscard]] u8 getColor(i32 x, i32 y) const;

    // NBT序列化
    void toNbt(nbt::tags::compound_tag& tag) const;
    static MapData fromNbt(const nbt::tags::compound_tag& tag, i32 mapId);

    // 访问器
    [[nodiscard]] i32 mapId() const { return m_mapId; }
    void setMapId(i32 id) { m_mapId = id; }
    [[nodiscard]] i32 xCenter() const { return m_xCenter; }
    [[nodiscard]] i32 zCenter() const { return m_zCenter; }
    [[nodiscard]] i32 scale() const { return m_scale; }
    [[nodiscard]] MapDimensionId dimension() const { return m_dimension; }
    [[nodiscard]] bool trackingPosition() const { return m_trackingPosition; }
    [[nodiscard]] bool unlimitedTracking() const { return m_unlimitedTracking; }
    [[nodiscard]] bool locked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }
    [[nodiscard]] const std::array<u8, COLOR_ARRAY_SIZE>& colors() const { return m_colors; }
    [[nodiscard]] const std::map<std::string, MapDecoration>& decorations() const { return m_decorations; }
    [[nodiscard]] const std::map<std::string, MapBanner>& banners() const { return m_banners; }
    [[nodiscard]] const std::map<std::string, MapFrame>& frames() const { return m_frames; }
    [[nodiscard]] std::vector<MapInfo>& playerInfos() { return m_playerInfos; }

    /**
     * @brief 获取地图名称（用于存储和日志）
     */
    [[nodiscard]] static std::string getMapName(i32 mapId);

    /**
     * @brief 标记数据为脏（需要保存）
     */
    void setDirty() { m_dirty = true; }
    [[nodiscard]] bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    /**
     * @brief 计算装饰物旋转值
     *
     * 在下界中使用基于游戏时间的伪随机旋转，模拟指南针失灵效果。
     * 在其他维度中使用实际朝向角度。
     * 参考: net.minecraft.world.level.saveddata.maps.MapItemSavedData.calculateRotation
     *
     * @param world 世界指针（可为nullptr，下界旋转需要）
     * @param rotation 实际旋转角度（度）
     * @return 旋转字节值 (0-15)
     */
    [[nodiscard]] u8 calculateRotation(const IWorld* world, f64 rotation) const;

    /**
     * @brief 检查追踪的装饰物数量是否超过上限
     *
     * 参考: net.minecraft.world.level.saveddata.maps.MapItemSavedData.isTrackedCountOverLimit
     */
    [[nodiscard]] bool isTrackedCountOverLimit(i32 limit) const;

    i32 m_mapId = 0;
    i32 m_xCenter = 0;
    i32 m_zCenter = 0;
    i32 m_scale = 0;
    MapDimensionId m_dimension = MapDimensionId::Overworld;
    bool m_trackingPosition = true;
    bool m_unlimitedTracking = false;
    bool m_locked = false;
    bool m_dirty = false;
    std::array<u8, COLOR_ARRAY_SIZE> m_colors{};
    std::map<std::string, MapDecoration> m_decorations;
    std::map<std::string, MapBanner> m_banners;
    std::map<std::string, MapFrame> m_frames;
    std::vector<MapInfo> m_playerInfos;
};

} // namespace mc::world::map
