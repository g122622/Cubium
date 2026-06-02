#pragma once

#include "common/core/Types.hpp"
#include <string>
#include <any>

namespace mc::mod::bedrock::addon {

/**
 * @brief 自定义组件参数
 *
 * 从JSON定义传入的参数数据，传递给组件事件回调。
 * 脚本注册自定义组件时，可以在行为包的block.json/item.json中
 * 定义参数，这些参数会通过CustomComponentParameters传递给回调。
 */
class CustomComponentParameters {
public:
    CustomComponentParameters() = default;
    explicit CustomComponentParameters(std::any params)
        : m_params(std::move(params))
    {}

    /**
     * @brief 获取参数数据
     *
     * @return 参数的std::any引用
     */
    [[nodiscard]] const std::any& params() const { return m_params; }

    /**
     * @brief 是否有参数
     */
    [[nodiscard]] bool hasParams() const { return m_params.has_value(); }

private:
    std::any m_params;
};

} // namespace mc::mod::bedrock::addon
