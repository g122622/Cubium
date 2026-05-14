#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 深度比较操作
 *
 * 定义深度测试的比较函数。
 * 与 Vulkan VkCompareOp 和 OpenGL glDepthFunc 对应。
 */
enum class CompareOp : u8 {
    Never,        // 始终失败
    Less,         // <
    Equal,        // ==
    LessEqual,    // <=
    Greater,      // >
    NotEqual,     // !=
    GreaterEqual, // >=
    Always        // 始终通过
};

/**
 * @brief 深度状态
 *
 * 定义深度测试和深度写入的配置。
 * 参考 MC 1.16.5 RenderStateDepthTest。
 */
struct DepthState {
    bool testEnabled = true;               // 是否启用深度测试
    bool writeEnabled = true;              // 是否写入深度缓冲
    CompareOp compareOp = CompareOp::Less; // 深度比较函数

    /**
     * @brief 创建禁用深度测试的状态
     */
    static DepthState disabled()
    {
        DepthState state;
        state.testEnabled = false;
        state.writeEnabled = false;
        return state;
    }

    /**
     * @brief 创建只读深度状态
     *
     * 用于透明物体，需要深度测试但不写入深度
     */
    static DepthState readOnly()
    {
        DepthState state;
        state.testEnabled = true;
        state.writeEnabled = false;
        state.compareOp = CompareOp::Less;
        return state;
    }

    /**
     * @brief 创建标准读写深度状态
     *
     * 用于不透明物体
     */
    static DepthState readWrite()
    {
        DepthState state;
        state.testEnabled = true;
        state.writeEnabled = true;
        state.compareOp = CompareOp::Less;
        return state;
    }

    /**
     * @brief 创建深度相等状态
     *
     * 用于 decals 或需要精确匹配的效果
     */
    static DepthState equal()
    {
        DepthState state;
        state.testEnabled = true;
        state.writeEnabled = false;
        state.compareOp = CompareOp::Equal;
        return state;
    }

    bool operator==(const DepthState& other) const
    {
        return testEnabled == other.testEnabled && writeEnabled == other.writeEnabled && compareOp == other.compareOp;
    }

    bool operator!=(const DepthState& other) const { return !(*this == other); }
};

} // namespace mc::client::renderer::api
