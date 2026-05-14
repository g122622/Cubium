#pragma once

#include "../buffer/IBuffer.hpp"
#include "../texture/ITexture.hpp"
#include "../texture/ITextureAtlas.hpp"
#include "RenderState.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::api {

/**
 * @brief 着色器阶段
 */
enum class ShaderStage : u8 { Vertex, Fragment, Geometry, TessControl, TessEvaluation, Compute };

/**
 * @brief 着色器模块描述
 */
struct ShaderModuleDesc {
    ShaderStage stage;
    std::string entryPoint = "main";
    std::vector<u8> bytecode; // SPIR-V 字节码
};

/**
 * @brief 管线描述
 */
struct PipelineDesc {
    std::string name;
    std::vector<ShaderModuleDesc> shaders;
    RenderState renderState;

    // 顶点输入描述 (简化版，实际可能需要更详细的描述)
    u32 vertexStride = sizeof(Vertex);

    // 描述符集布局描述
    struct DescriptorSetLayout {
        u32 set = 0;
        struct Binding {
            u32 binding = 0;
            // 可以扩展为更详细的描述
        };
        std::vector<Binding> bindings;
    };
    std::vector<DescriptorSetLayout> descriptorLayouts;
};

/**
 * @brief 渲染管线接口
 *
 * 平台无关的渲染管线抽象。
 * 封装了着色器、渲染状态和管线配置。
 */
class IPipeline {
public:
    virtual ~IPipeline() = default;

    /**
     * @brief 销毁管线
     */
    virtual void destroy() = 0;

    /**
     * @brief 获取管线名称
     */
    [[nodiscard]] virtual const std::string& name() const = 0;

    /**
     * @brief 获取渲染状态
     */
    [[nodiscard]] virtual const RenderState& renderState() const = 0;

    /**
     * @brief 检查管线是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief 绑定管线到命令缓冲区
     */
    virtual void bind(void* commandBuffer) = 0;
};

/**
 * @brief 管线布局接口
 *
 * 描述管线的描述符集布局和推送常量范围。
 */
class IPipelineLayout {
public:
    virtual ~IPipelineLayout() = default;

    /**
     * @brief 销毁布局
     */
    virtual void destroy() = 0;

    /**
     * @brief 获取描述符集数量
     */
    [[nodiscard]] virtual u32 descriptorSetCount() const = 0;

    /**
     * @brief 检查布局是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;
};

/**
 * @brief 描述符集接口
 *
 * 描述资源绑定 (uniform buffer, texture, sampler 等)。
 */
class IDescriptorSet {
public:
    virtual ~IDescriptorSet() = default;

    /**
     * @brief 绑定 Uniform 缓冲区
     * @param binding 绑定槽
     * @param buffer Uniform 缓冲区
     */
    virtual void bindUniformBuffer(u32 binding, IUniformBuffer* buffer) = 0;

    /**
     * @brief 绑定纹理
     * @param binding 绑定槽
     * @param texture 纹理
     */
    virtual void bindTexture(u32 binding, ITexture* texture) = 0;

    /**
     * @brief 绑定纹理图集
     * @param binding 绑定槽
     * @param atlas 纹理图集
     */
    virtual void bindTextureAtlas(u32 binding, ITextureAtlas* atlas) = 0;

    /**
     * @brief 更新描述符集
     */
    virtual void update() = 0;

    /**
     * @brief 绑定到命令缓冲区
     * @param commandBuffer 命令缓冲区
     * @param set 描述符集索引
     */
    virtual void bind(void* commandBuffer, u32 set) = 0;
};

} // namespace mc::client::renderer::api
