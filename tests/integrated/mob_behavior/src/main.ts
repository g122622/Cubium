// mob_behavior 行为包入口：注册生物行为类 GameTest。
// 测试按主角生物的 Cubium 实体分类（src/common/entity/entities）拆分到 src/tests/ 子目录，
// 未来为每个生物加行为测试时，放入对应分类目录即可。

import { registerZombieTests } from "./tests/monster/undead/ZombieTests.js";
import { registerSkeletonTests } from "./tests/monster/undead/SkeletonTests.js";
import { registerIronGolemTests } from "./tests/passive/golem/IronGolemTests.js";
import { registerZoglinTests } from "./tests/monster/nether/ZoglinTests.js";
import { registerMagmaCubeTests } from "./tests/monster/nether/MagmaCubeTests.js";
import { registerZombifiedPiglinTests } from "./tests/monster/nether/ZombifiedPiglinTests.js";
import { registerEndermanTests } from "./tests/monster/end/EndermanTests.js";
import { registerWitherSkeletonTests } from "./tests/monster/undead/WitherSkeletonTests.js";
import { registerHuskTests } from "./tests/monster/undead/HuskTests.js";
import { registerPhantomTests } from "./tests/monster/basic/PhantomTests.js";
import { registerCreeperTests } from "./tests/monster/basic/CreeperTests.js";
import { registerSlimeTests } from "./tests/monster/basic/SlimeTests.js";
import { registerSpiderTests } from "./tests/monster/arthropod/SpiderTests.js";
import { registerPigTests } from "./tests/passive/basic/PigTests.js";
import { registerCowTests } from "./tests/passive/basic/CowTests.js";
import { registerSheepTests } from "./tests/passive/basic/SheepTests.js";
import { registerChickenTests } from "./tests/passive/basic/ChickenTests.js";

registerZombieTests();
registerSkeletonTests();
registerIronGolemTests();
registerZoglinTests();
registerMagmaCubeTests();
registerZombifiedPiglinTests();
registerEndermanTests();
registerWitherSkeletonTests();
registerHuskTests();
registerPhantomTests();
registerCreeperTests();
registerSlimeTests();
registerSpiderTests();
registerPigTests();
registerCowTests();
registerSheepTests();
registerChickenTests();
