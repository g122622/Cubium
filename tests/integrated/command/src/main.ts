// command 行为包入口：注册命令类 GameTest。

import { registerCommandTests } from "./CommandTests.js";
import { registerWorldCommandTests } from "./tests/world/WorldCommandTests.js";
import { registerCloneCommandTests } from "./tests/world/CloneCommandTests.js";
import { registerEntityCommandTests } from "./tests/entity/EntityCommandTests.js";
import { registerTeleportCommandTests } from "./tests/entity/TeleportCommandTests.js";
import { registerGameModeTests } from "./tests/player/GameModeTests.js";
import { registerEffectTests } from "./tests/player/EffectTests.js";
import { registerEnchantTests } from "./tests/player/EnchantTests.js";
import { registerExperienceTests } from "./tests/player/ExperienceTests.js";

registerCommandTests();
registerWorldCommandTests();
registerCloneCommandTests();
registerEntityCommandTests();
registerTeleportCommandTests();
registerGameModeTests();
registerEffectTests();
registerEnchantTests();
registerExperienceTests();
