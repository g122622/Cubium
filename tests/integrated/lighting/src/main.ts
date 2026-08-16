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
// 跨服务端：blockLight/skyLight/brightness/canSeeSky 是 Cubium 专有，基岩 BDS 的 Block 无此属性（读得
// undefined→-1）。故本包测试在基岩端归类为 one-sided（仅 Cubium 跑），不参与基岩对比的双向判定。

// 必须最先 import（副作用执行）：GameTest RegistrationBuilder 跨服务端兼容垫片。
// Cubium 在官方 RegistrationBuilder 之上扩展了 skyAccess 链式方法（基岩 BDS 无此方法，调用抛 TypeError
// 致整个行为包加载失败）。垫片在基岩侧用 prototype 注入把 skyAccess 降级为 no-op。详见 gametest-shim.ts。
import "./gametest-shim.js";

// BlockLightEmissionTests 在模块加载时即注册（registerEmissionTest 顶层调用），无 export，副作用 import。
import "./tests/core/BlockLightEmissionTests.js";

import { registerBlockLightPropagationTests } from "./tests/core/BlockLightPropagationTests.js";
import { registerSkyLightTests } from "./tests/core/SkyLightTests.js";
import { registerBlockChangeRelightTests } from "./tests/core/BlockChangeRelightTests.js";
import { registerBrightnessTests } from "./tests/core/BrightnessTests.js";

registerBlockLightPropagationTests();
registerSkyLightTests();
registerBlockChangeRelightTests();
registerBrightnessTests();
