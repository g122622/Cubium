// command 行为包入口：注册命令类 GameTest。

import { registerCommandTests } from "./CommandTests.js";
import { registerWorldCommandTests } from "./tests/world/WorldCommandTests.js";
import { registerCloneCommandTests } from "./tests/world/CloneCommandTests.js";
import { registerTimeTests } from "./tests/world/TimeTests.js";
import { registerGameRuleTests } from "./tests/world/GameRuleTests.js";
import { registerWeatherTests } from "./tests/world/WeatherTests.js";
import { registerEntityCommandTests } from "./tests/entity/EntityCommandTests.js";
import { registerTeleportCommandTests } from "./tests/entity/TeleportCommandTests.js";
import { registerGameModeTests } from "./tests/player/GameModeTests.js";
import { registerEffectTests } from "./tests/player/EffectTests.js";
import { registerEnchantTests } from "./tests/player/EnchantTests.js";
import { registerExperienceTests } from "./tests/player/ExperienceTests.js";
import { registerGiveTests } from "./tests/player/GiveTests.js";
import { registerTagTests } from "./tests/player/TagTests.js";
import { registerDifficultyTests } from "./tests/world/DifficultyTests.js";

registerCommandTests();
registerWorldCommandTests();
registerCloneCommandTests();
registerTimeTests();
registerGameRuleTests();
registerWeatherTests();
registerEntityCommandTests();
registerTeleportCommandTests();
registerGameModeTests();
registerEffectTests();
registerEnchantTests();
registerExperienceTests();
registerGiveTests();
registerTagTests();
registerDifficultyTests();
