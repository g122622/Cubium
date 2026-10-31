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

package dev.horizonlab.diff;

import net.minecraft.core.registries.Registries;
import net.minecraft.server.WorldLoader;
import net.minecraft.world.level.GameType;
import net.minecraft.world.level.gamerules.GameRules;
import net.minecraft.world.level.LevelSettings;
import net.minecraft.world.level.WorldDataConfiguration;
import net.minecraft.world.level.levelgen.WorldOptions;
import net.minecraft.world.level.levelgen.presets.WorldPresets;
import net.minecraft.world.level.storage.PrimaryLevelData;
import net.minecraft.world.level.storage.WorldData;

/**
 * 寻路 oracle 的世界数据：**超平坦、无结构**，方块由场景脚本自行摆放。
 *
 * <p>用 flat 预设而不是 normal：normal 会生成真实地形，场景摆方块时要先把一整片地形清掉，
 * 既慢又容易漏；flat 预设下每个区块本来就是空的（除了一层基岩），场景只需往上加方块。
 */
final class PathServerData {
    private PathServerData() {}

    /** 超平坦预设的世界数据（与原版「超平坦」新建世界同一套流程）。 */
    static WorldLoader.DataLoadOutput<WorldData> newWorldData(WorldLoader.DataLoadContext context) {
        WorldDataConfiguration configuration = context.dataConfiguration();
        LevelSettings settings = new LevelSettings(
            "diff-path", GameType.CREATIVE, false, net.minecraft.world.Difficulty.NORMAL, false,
            new GameRules(configuration.enabledFeatures()), configuration);
        var dimensions = WorldPresets.createFlatWorldDimensions(context.datapackWorldgen());
        var complete = dimensions.bake(context.datapackDimensions().lookupOrThrow(Registries.LEVEL_STEM));
        var lifecycle = complete.lifecycle().add(context.datapackWorldgen().allRegistriesLifecycle());
        var worldData = new PrimaryLevelData(
            settings, WorldOptions.testWorldWithRandomSeed(), complete.specialWorldProperty(), lifecycle);
        return new WorldLoader.DataLoadOutput<>(worldData, complete.dimensionsRegistryAccess());
    }
}