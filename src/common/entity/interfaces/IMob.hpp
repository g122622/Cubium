#pragma once

namespace mc::entity {

/**
 * @brief 敌对生物标记接口
 *
 * 对齐 MC 1.16.5 `IMob` 的空标记语义，用于表达
 * “该实体属于敌对生物阵营”这一类型信息。
 */
class IMob {
public:
    virtual ~IMob() = default;
};

} // namespace mc::entity
