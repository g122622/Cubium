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
