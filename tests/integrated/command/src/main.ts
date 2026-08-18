// command 行为包入口：注册命令类 GameTest。

import { registerCommandTests } from "./CommandTests.js";
import { registerWorldCommandTests } from "./tests/world/WorldCommandTests.js";
import { registerCloneCommandTests } from "./tests/world/CloneCommandTests.js";
import { registerEntityCommandTests } from "./tests/entity/EntityCommandTests.js";
import { registerTeleportCommandTests } from "./tests/entity/TeleportCommandTests.js";

registerCommandTests();
registerWorldCommandTests();
registerCloneCommandTests();
registerEntityCommandTests();
registerTeleportCommandTests();
