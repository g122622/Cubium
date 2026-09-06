// teleport 行为包入口：注册跨维度传送类 GameTest。
// 测试按传送机制分类（下界传送门/末地传送门/脚本传送/Dimension.id）拆分到 src/tests/cross_dimension/。
// 验证 Cubium 跨维度传送链路（EndPortalBlock→changeDimension→EntityManager 迁移）与脚本绑定
// （world.getDimension/Dimension.id/Entity.teleport）对齐 vanilla 行为。

// 必须最先 import（副作用执行）：GameTest RegistrationBuilder 跨服务端兼容垫片。
// Cubium 在官方 RegistrationBuilder 之上扩展了 skyAccess 链式方法（基岩 BDS 无此方法，调用抛 TypeError
// 致整个行为包加载失败）。垫片在基岩侧用 prototype 注入把 skyAccess 降级为 no-op，Cubium 侧保留原实现。
// 详见 gametest-shim.ts。
import "./gametest-shim.js";

import { registerNetherPortalTests } from "./tests/cross_dimension/NetherPortalTests.js";
import { registerEndPortalTests } from "./tests/cross_dimension/EndPortalTests.js";
import { registerScriptTeleportTests } from "./tests/cross_dimension/ScriptTeleportTests.js";
import { registerDimensionIdTests } from "./tests/cross_dimension/DimensionIdTests.js";
import { registerExecuteInTeleportTests } from "./tests/cross_dimension/ExecuteInTeleportTests.js";
import { registerNetherPortalFormationTests } from "./tests/cross_dimension/NetherPortalFormationTests.js";
import { registerNetherPortalCoordinateScaleTests } from "./tests/cross_dimension/NetherPortalCoordinateScaleTests.js";
import { registerEndPortalPlatformTests } from "./tests/cross_dimension/EndPortalPlatformTests.js";
import { registerPortalCooldownTests } from "./tests/cross_dimension/PortalCooldownTests.js";
import { registerEntityCrossDimensionTests } from "./tests/cross_dimension/EntityCrossDimensionTests.js";

registerNetherPortalTests();
registerEndPortalTests();
registerScriptTeleportTests();
registerDimensionIdTests();
registerExecuteInTeleportTests();
registerNetherPortalFormationTests();
registerNetherPortalCoordinateScaleTests();
registerEndPortalPlatformTests();
registerPortalCooldownTests();
registerEntityCrossDimensionTests();
