// block_behavior 行为包入口：注册方块行为类 GameTest。
// 测试按主角方块的 Cubium 实现分类（src/common/world/block/blocks 的目录结构）拆分到 src/tests/ 子目录，
// 镜像 C++ blocks/ 功能分类（liquid/special/agricultural/building/vegetation 等）。
// 未来为每个方块加行为测试时，放入对应分类目录即可。

// 必须最先 import（副作用执行）：GameTest RegistrationBuilder 跨服务端兼容垫片。
// Cubium 在官方 RegistrationBuilder 之上扩展了 skyAccess 链式方法（基岩 BDS 无此方法，调用抛 TypeError
// 致整个行为包加载失败）。垫片在基岩侧用 prototype 注入把 skyAccess 降级为 no-op，Cubium 侧保留原实现。
// 详见 gametest-shim.ts。
import "./gametest-shim.js";

import { registerLiquidTests } from "./tests/liquid/LiquidTests.js";
import { registerSpongeTests } from "./tests/special/SpongeTests.js";
import { registerFarmlandTests } from "./tests/agricultural/FarmlandTests.js";
import { registerConcretePowderTests } from "./tests/building/ConcretePowderTests.js";
import { registerFallingBlockTests } from "./tests/falling/FallingBlockTests.js";
import { registerCoralTests } from "./tests/coral/CoralTests.js";

registerLiquidTests();
registerSpongeTests();
registerFarmlandTests();
registerConcretePowderTests();
registerFallingBlockTests();
registerCoralTests();
