#pragma once

#include "WorldCarver.hpp"
#include "CaveCarver.hpp"
#include "CanyonCarver.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc::world::gen::carver {

/**
 * @brief 水下洞穴雕刻器
 *
 * 参考 MC 1.16.5 UnderwaterCaveWorldCarver，继承自 CaveWorldCarver。
 * 与普通洞穴的区别：
 * - 可雕刻方块包含水下相关方块（水、熔岩、黑曜石、空气等）
 * - 不检测区域是否在水下（始终可以在水下生成）
 * - Y==10 处有特殊逻辑：25% 岩浆块，75% 黑曜石
 * - Y<10 填充熔岩，Y>10 填充水
 */
class UnderwaterCaveCarver : public CaveCarver {
public:
    UnderwaterCaveCarver();

    ~UnderwaterCaveCarver() override = default;

protected:
    /**
     * @brief 检查椭球位置是否有效
     * 水下洞穴始终返回 false（不跳过任何位置）
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;

    /**
     * @brief 检查方块是否可雕刻
     * 水下洞穴包含更多可雕刻方块
     */
    [[nodiscard]] static bool isUnderwaterCarvable(const BlockState& state);
};

/**
 * @brief 水下峡谷雕刻器
 *
 * 参考 MC 1.16.5 UnderwaterCanyonWorldCarver，继承自 CanyonWorldCarver。
 * 与普通峡谷的区别类似水下洞穴。
 */
class UnderwaterCanyonCarver : public CanyonCarver {
public:
    UnderwaterCanyonCarver();

    ~UnderwaterCanyonCarver() override = default;

protected:
    /**
     * @brief 检查椭球位置是否有效
     * 水下峡谷使用与普通峡谷相同的厚度检测
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;
};

/**
 * @brief 创建水下洞穴雕刻器
 */
std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver();

/**
 * @brief 创建水下峡谷雕刻器
 */
std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver();

} // namespace mc::world::gen::carver

// 向后兼容：在 mc 命名空间中提供类型别名
namespace mc {
using UnderwaterCaveCarver = world::gen::carver::UnderwaterCaveCarver;
using UnderwaterCanyonCarver = world::gen::carver::UnderwaterCanyonCarver;
}
