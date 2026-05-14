#include "TransformerTraits.hpp"
#include "../LayerContext.hpp"
#include <memory>

namespace mc {
namespace layer {

// ============================================================================
// IC0Transformer 工厂方法实现
// ============================================================================

std::unique_ptr<IAreaFactory> IC0Transformer::apply(IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input)
{
    // 使用 shared_from_this 获取 shared_ptr，然后 dynamic_pointer_cast 转换
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<TransformFactory>(this, sharedContext, std::move(input));
}

// ============================================================================
// IC1Transformer 工厂方法实现
// ============================================================================

std::unique_ptr<IAreaFactory> IC1Transformer::apply(IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<TransformFactory>(this, sharedContext, std::move(input));
}

// ============================================================================
// ICastleTransformer 工厂方法实现
// ============================================================================

std::unique_ptr<IAreaFactory> ICastleTransformer::apply(
    IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<TransformFactory>(this, sharedContext, std::move(input));
}

// ============================================================================
// IBishopTransformer 工厂方法实现
// ============================================================================

std::unique_ptr<IAreaFactory> IBishopTransformer::apply(
    IExtendedAreaContext& context, std::unique_ptr<IAreaFactory> input)
{
    auto sharedContext = std::dynamic_pointer_cast<LayerContext>(context.shared_from_this());
    return std::make_unique<TransformFactory>(this, sharedContext, std::move(input));
}

} // namespace layer
} // namespace mc
