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

#include "common/world/gen/jigsaw/JigsawPattern.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

/**
 * @brief 试炼密室模板池注册
 *
 * 注册试炼密室所需的自定义处理器和池别名绑定。
 * 试炼密室的大部分模板池从数据包 JSON 文件中加载
 * （路径：data/minecraft/worldgen/template_pool/trial_chambers/），
 * 此类负责注册需要编程式创建的组件。
 *
 * 数据包中已存在的模板池：
 * - trial_chambers/chamber/end          - 起始池
 * - trial_chambers/corridor             - 柱廊
 * - trial_chambers/hallway              - 过道
 * - trial_chambers/intersection         - 交叉口
 * - trial_chambers/chamber/assembly     - 组装决斗室
 * - trial_chambers/chamber/eruption     - 喷发决斗室
 * - trial_chambers/chamber/pedestal     - 基座决斗室
 * - trial_chambers/chamber/slanted      - 倾斜决斗室
 * - trial_chambers/spawner/...           - 刷怪笼
 * - trial_chambers/reward/...            - 宝库奖励
 * - trial_chambers/corridor/slices      - 柱廊片段
 * - trial_chambers/atrium               - 中庭
 * - trial_chambers/decor                - 装饰
 * - trial_chambers/dispensers/...        - 发射器机关
 */
class TrialChambersPools {
public:
    /**
     * @brief 注册试炼密室模板池和处理器
     * @param registry 模板池注册表
     *
     * 在 Pools::initialize() 中调用。
     * 主要注册铜灯降级处理器，因为模板池本身从数据包加载。
     */
    static void registerAll(jigsaw::JigsawPatternRegistry& registry);
};

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
