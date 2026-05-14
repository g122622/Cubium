#pragma once

#include "../LayerContext.hpp"

namespace mc {
namespace layer {

/**
 * @brief 缩放层变换器
 *
 * 将区域放大 2 倍。有两种模式：普通和模糊。
 * 参考 MC ZoomLayer
 *
 * 采样模式：
 * - 偶数坐标 (x, z)：直接返回父级值
 * - 边缘坐标：从相邻值中选择
 * - 角落坐标：使用 pickZoomed 算法
 */
class ZoomLayer : public ITransformer1 {
public:
    /**
     * @brief 缩放模式
     */
    enum class Mode {
        Normal, // 普通缩放：使用众数算法
        Fuzzy   // 模糊缩放：随机选择
    };

    explicit ZoomLayer(Mode mode = Mode::Normal);

    using ITransformer1::apply;
    [[nodiscard]] i32 apply(IAreaContext& ctx, const IArea& area, i32 x, i32 z) override;

    [[nodiscard]] i32 getOffsetX(i32 x) const override { return x >> 1; }
    [[nodiscard]] i32 getOffsetZ(i32 z) const override { return z >> 1; }

    [[nodiscard]] std::unique_ptr<IAreaFactory> apply(
        IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input) override;

private:
    Mode m_mode;

    /**
     * @brief 选择缩放后的值（众数算法）
     */
    [[nodiscard]] i32 pickZoomed(IAreaContext& ctx, i32 a, i32 b, i32 c, i32 d);
};

} // namespace layer
} // namespace mc
