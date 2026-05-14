#pragma once

#include "EntityModel.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace mc::client::renderer::entity::model {

/**
 * @brief 分段模型基类
 *
 * 用于复杂实体（如末影龙）的分段渲染。
 * 分段渲染允许模型的各个部分独立动画和渲染。
 *
 * 参考 MC 1.16.5 SegmentedModel
 */
class SegmentedModel : public EntityModel {
public:
    using SegmentFunc = std::function<void(ModelRenderer&, f64)>;

    SegmentedModel() = default;
    ~SegmentedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    /**
     * @brief 设置分段渲染函数
     *
     * @param partName 部件名称
     * @param func 渲染函数
     */
    void setSegmentRenderer(const std::string& partName, SegmentFunc func);

    /**
     * @brief 渲染指定分段
     *
     * @param partName 部件名称
     * @param scale 缩放因子
     */
    void renderSegment(const std::string& partName, f64 scale);

    /**
     * @brief 检查分段是否存在
     */
    [[nodiscard]] bool hasSegment(const std::string& partName) const;

protected:
    std::unordered_map<std::string, SegmentFunc> m_segmentRenderers;
};

} // namespace mc::client::renderer::entity::model
