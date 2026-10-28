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

import com.google.gson.Gson;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Proxy;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.SpawnPlacementTypes;
import net.minecraft.world.entity.SpawnPlacements;
import net.minecraft.world.level.LevelReader;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.border.WorldBorder;

/** 运行原版位置检查；世界只提供共同输入中的三层方块与边界，未支持的访问直接失败。 */
public final class SpawnPlacementOracle {
    private static final Gson GSON = new Gson();

    private SpawnPlacementOracle() {}

    public static void main(String[] args) throws Exception {
        var registries = DataPackRegistries.load();
        if (args.length == 2 && args[0].equals("list")) {
            var ids = BuiltInRegistries.ENTITY_TYPE.keySet().stream().map(Object::toString).sorted().toList();
            Files.write(Path.of(args[1]), ids, StandardCharsets.UTF_8);
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: SpawnPlacementOracle list <units> | run <input> <output>");
        }
        var blocks = registries.lookup.lookupOrThrow(Registries.BLOCK);
        StringBuilder output = new StringBuilder();
        for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject input = JsonParser.parseString(line).getAsJsonObject();
            String id = input.get("entity").getAsString();
            var type = BuiltInRegistries.ENTITY_TYPE.getOptional(Identifier.parse(id)).orElseThrow();
            var placement = SpawnPlacements.getPlacementType(type);
            String placementName = placement == SpawnPlacementTypes.ON_GROUND ? "on_ground"
                : placement == SpawnPlacementTypes.IN_WATER ? "in_water"
                : placement == SpawnPlacementTypes.IN_LAVA ? "in_lava" : "no_restrictions";
            JsonObject result = new JsonObject();
            result.addProperty("entity", id);
            result.addProperty("placement", placementName);
            result.addProperty("heightmap", SpawnPlacements.getHeightmapType(type).getSerializationKey());
            JsonArray cases = new JsonArray();
            for (var element : input.getAsJsonArray("cases")) {
                var scenario = element.getAsJsonObject();
                int y = scenario.get("y").getAsInt();
                BlockState below = BlockStateParser.parseForBlock(blocks, scenario.get("below").getAsString(), false).blockState();
                BlockState at = BlockStateParser.parseForBlock(blocks, scenario.get("at").getAsString(), false).blockState();
                BlockState above = BlockStateParser.parseForBlock(blocks, scenario.get("above").getAsString(), false).blockState();
                WorldBorder border = new WorldBorder();
                border.setSize(32);
                border.setCenter(scenario.get("insideBorder").getAsBoolean() ? 0 : 100, 0);
                LevelReader world = (LevelReader) Proxy.newProxyInstance(LevelReader.class.getClassLoader(),
                    new Class<?>[] { LevelReader.class }, (proxy, method, parameters) -> {
                        if (method.getName().equals("getBlockState")) {
                            BlockPos pos = (BlockPos) parameters[0];
                            return pos.getY() == y - 1 ? below : pos.getY() == y ? at : pos.getY() == y + 1 ? above
                                : net.minecraft.world.level.block.Blocks.AIR.defaultBlockState();
                        }
                        if (method.getName().equals("getFluidState")) {
                            return ((LevelReader) proxy).getBlockState((BlockPos) parameters[0]).getFluidState();
                        }
                        if (method.getName().equals("getWorldBorder")) {
                            return border;
                        }
                        if (method.isDefault()) {
                            return InvocationHandler.invokeDefault(proxy, method, parameters);
                        }
                        throw new UnsupportedOperationException(method.toString());
                    });
                JsonObject answer = new JsonObject();
                answer.addProperty("name", scenario.get("name").getAsString());
                answer.addProperty("positionOk", SpawnPlacements.isSpawnPositionOk(type, world, new BlockPos(0, y, 0)));
                cases.add(answer);
            }
            result.add("cases", cases);
            output.append(GSON.toJson(result)).append('\n');
        }
        Files.writeString(Path.of(args[2]), output, StandardCharsets.UTF_8);
    }
}
