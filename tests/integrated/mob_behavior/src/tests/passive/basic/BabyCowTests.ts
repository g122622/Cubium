// 幼年动物（FollowParentGoal + spawn_baby 事件）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// 小牛必须靠近同类成年母牛。使用实体句柄，避免把游走的母牛当成小牛。
function babyCowFollowsMother(test: Test): void {
    const mother = test.spawn("minecraft:cow", { x: 6, y: 2, z: 4 });
    (mother.getComponent("minecraft:movement") as any).setCurrentValue(0);
    const baby = test.spawn("cow<minecraft:spawn_baby>", { x: 2, y: 2, z: 4 });
    const distance = () => Math.hypot(baby.location.x - mother.location.x, baby.location.z - mother.location.z);
    test.assert(distance() >= 3.5, "baby should start outside the follow stopping distance");
    let followedFirstPosition = false;
    let secondStartDistance = 0;
    pollUntilSucceed(test, () => {
        if (distance() >= 3.5) return false;
        if (!followedFirstPosition) {
            followedFirstPosition = true;
            mother.teleport(test.worldLocation({ x: 7, y: 2, z: 6 }));
            secondStartDistance = distance();
            test.assert(secondStartDistance > 3.5, "moving the mother should require a second follow response");
            return false;
        }
        return distance() < secondStartDistance - 1;
    }, {
        startTick: 4, interval: 1, maxTick: 100,
        onTimeout: () => test.assert(false,
            `baby did not follow both mother positions: phase=${followedFirstPosition}, distance=${distance().toFixed(2)}, ` +
            `baby=${JSON.stringify(baby.location)}, ` +
            `mother=${JSON.stringify(mother.location)}`),
    });
}

export function registerBabyCowTests(): void {
    GameTest.register("MobBehaviorTests", "baby_cow_follows_mother", babyCowFollowsMother)
        .structureName("gametests:grass_pen")
        .maxTicks(120);
}
