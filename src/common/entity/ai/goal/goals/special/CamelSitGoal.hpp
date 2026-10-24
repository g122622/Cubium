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

#include "common/entity/ai/goal/Goal.hpp"

namespace mc {

class CamelEntity;

namespace entity::ai::goal {

/**
 * @brief 骆驼随机坐下/站起（对齐 vanilla CamelAi.RandomSitting）
 *
 * 启动条件（全部满足）：不在水中、距上次姿态变化 >= 20 秒、未被拴绳、着地、
 * 无控制乘客、且目标姿态处不会窒息。
 * 执行：坐着则站起，否则（不在恐慌中）坐下。
 */
class CamelSitGoal : public Goal {
public:
    explicit CamelSitGoal(CamelEntity* camel);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "CamelSitGoal";
    }

private:
    CamelEntity* m_camel;
};

} // namespace entity::ai::goal
} // namespace mc