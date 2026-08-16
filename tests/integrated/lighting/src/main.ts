// lighting 行为包入口：注册光照引擎类 GameTest。
//
// 测试组织：src/tests/core/ 下按光照引擎职责拆分（发光/传播/天空光/重算/综合亮度），每个文件导出
// register*Tests 函数。BlockLightEmissionTests 在模块加载时即注册（无 export），故以副作用 import 引入。
//
// 测试策略（用户要求"拓展 script api 直读光照值 + 间接行为观测"）：
//   - 直接观测：Cubium 扩展 Block.blockLight/skyLight/brightness/canSeeSky（MinecraftModuleFactory.cpp
//     新增绑定），确定性直读光照数值，覆盖发光等级/曼哈顿衰减/max 语义/天空光遮挡/方块变更重算/
//     综合亮度 max 计算。这是光照引擎核心，确定性、可重复。
//   - 间接观测：亡灵日光燃烧（天空光驱动）已由 mob_behavior 包五方对照覆盖（zombie/skeleton/stray/
//     bogged 燃 + wither_skeleton 免疫），本包不重复。雪/冰融化依赖 randomTick（概率事件，跨两端
//     flaky），其光照前提（blockLight>=12）已被衰减测试确定性覆盖，故不单独写 flaky 融化测试。
//
// 不写「天空光散射」测试（water/lava/ice/leaves 散射）的设计决策：
//   wiki「散射」（tech_亮度.txt#散射，JE 专有）：天空光向下穿过散射方块（Leaves/Ice/Water/Lava/含水）
//   减1，即散射方块自身 skyLight=14、下方=13。Cubium 走 StarLight 算法实现散射的方式是
//   "opacity>0 的格子垂直列 break + 水平 flood-fill 衰减 max(1,opacity)"，与 vanilla 散射语义是不同
//   抽象层次，导致系统性偏差：water(opacity=0)不衰减→自身15/下方15（vanilla 应14/13）；ice(opacity=2)
//   →自身13/下方14（vanilla 应14/13，方向相反）；仅 lava(opacity=1)巧合接近 14/13。此外 water/lava
//   是流体，放置后流动会污染探针点（lava 流到探针格使其 skyLight 在 13/14 间非确定波动）。综上，
//   散射行为既与 vanilla 不一致（按准则不为偏差写测试），又对流体系非确定，故不写散射测试。
//   TODO: 待 Cubium 实现真正的 vanilla 散射语义（散射方块穿过统一减1，与 opacity 解耦）后补充。
//
// 跨服务端：blockLight/skyLight/brightness/canSeeSky 是 Cubium 专有，基岩 BDS 的 Block 无此属性（读得
// undefined→-1）。故本包测试在基岩端归类为 one-sided（仅 Cubium 跑），不参与基岩对比的双向判定。

// 必须最先 import（副作用执行）：GameTest RegistrationBuilder 跨服务端兼容垫片。
// Cubium 在官方 RegistrationBuilder 之上扩展了 skyAccess 链式方法（基岩 BDS 无此方法，调用抛 TypeError
// 致整个行为包加载失败）。垫片在基岩侧用 prototype 注入把 skyAccess 降级为 no-op。详见 gametest-shim.ts。
import "./gametest-shim.js";

// BlockLightEmissionTests 在模块加载时即注册（registerEmissionTest 顶层调用），无 export，副作用 import。
import "./tests/core/BlockLightEmissionTests.js";
// ExtraEmissionTests 同设计：registerExtraEmissionTest 顶层调用注册紫晶簇系列发光方块，无 export，副作用 import。
import "./tests/core/ExtraEmissionTests.js";

import { registerBlockLightPropagationTests } from "./tests/core/BlockLightPropagationTests.js";
import { registerSkyLightTests } from "./tests/core/SkyLightTests.js";
import { registerBlockChangeRelightTests } from "./tests/core/BlockChangeRelightTests.js";
import { registerBrightnessTests } from "./tests/core/BrightnessTests.js";
import { registerOpacityBlockLightTests } from "./tests/core/OpacityBlockLightTests.js";
import { registerShapeOcclusionSkyLightTests } from "./tests/core/ShapeOcclusionSkyLightTests.js";
import { registerSkyLightColumnDepthTests } from "./tests/core/SkyLightColumnDepthTests.js";
import { registerDynamicEmissionTests } from "./tests/core/DynamicEmissionTests.js";

registerBlockLightPropagationTests();
registerSkyLightTests();
registerBlockChangeRelightTests();
registerBrightnessTests();
registerOpacityBlockLightTests();
registerShapeOcclusionSkyLightTests();
registerSkyLightColumnDepthTests();
registerDynamicEmissionTests();
