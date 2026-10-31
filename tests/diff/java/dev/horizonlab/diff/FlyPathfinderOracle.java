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
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import net.minecraft.core.BlockPos;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.PathNavigationRegion;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.pathfinder.FlyNodeEvaluator;
import net.minecraft.world.level.pathfinder.Node;
import net.minecraft.world.level.pathfinder.PathFinder;
import net.minecraft.world.level.pathfinder.PathfindingContext;

/**
 * 飞行寻路 oracle：跑原版 {@code FlyNodeEvaluator} + {@code PathFinder}。
 *
 * <p>与 {@link PathFinderOracle} 的地形搭建完全相同，只把地面求值器换成飞行求值器，
 * 于是两侧比对的是「同一块地形上飞行实体的 A* 结果」。飞行寻路比地面多两个维度：
 * 邻居有 26 个方向，且起点的枚举方式（原版按实体包围盒逐格扫描）与地面不同，
 * 因此单独成一个单元。
 */
public final class FlyPathfinderOracle {
    private static final Gson GSON = new Gson();

    private FlyPathfinderOracle() {}

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), List.of(), StandardCharsets.UTF_8);
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: FlyPathfinderOracle list <units> | run <input> <output>");
        }

        PathWorld world = PathWorld.get();
        ServerLevel level = world.level;
        var blocks = level.registryAccess().lookupOrThrow(net.minecraft.core.registries.Registries.BLOCK);
        StringBuilder output = new StringBuilder();
        for (String line : Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject input = JsonParser.parseString(line).getAsJsonObject();
            output.append(GSON.toJson(evaluate(world, level, blocks, input))).append('\n');
        }
        Files.writeString(Path.of(args[2]), output, StandardCharsets.UTF_8);
    }

    private static JsonObject evaluate(
        PathWorld world,
        ServerLevel level,
        net.minecraft.core.HolderLookup<net.minecraft.world.level.block.Block> blocks,
        JsonObject input) {
        String unit = input.get("unit").getAsString();
        JsonObject origin = input.getAsJsonObject("origin");
        int originX = origin.get("x").getAsInt();
        int originY = origin.get("y").getAsInt();
        int originZ = origin.get("z").getAsInt();
        int sizeX = input.get("sizeX").getAsInt();
        int sizeY = input.get("sizeY").getAsInt();
        int sizeZ = input.get("sizeZ").getAsInt();

        // 1. 场地方块与地面寻路完全相同：先清空，再摆本场景方块。
        world.clear(originX, originZ, Math.max(sizeX, sizeZ) + 4, originY - 4, sizeY + 8);
        for (var element : input.getAsJsonArray("cells")) {
            JsonObject cell = element.getAsJsonObject();
            BlockPos pos = new BlockPos(
                originX + cell.get("dx").getAsInt(),
                originY + cell.get("dy").getAsInt(),
                originZ + cell.get("dz").getAsInt());
            BlockState state = WalkPathTypeOracle.parseBlockChecked(blocks, cell.get("block").getAsString());
            world.setBlockDirect(pos.getX(), pos.getY(), pos.getZ(), state);
        }
        world.invalidatePathTypeCache();

        // 2. 实体。
        JsonObject mobInput = input.getAsJsonObject("mob");
        PathProbeMob mob = new PathProbeMob(
            level,
            mobInput.get("width").getAsFloat(),
            mobInput.get("height").getAsFloat(),
            mobInput.get("stepHeight").getAsFloat(),
            mobInput.get("onGround").getAsBoolean(),
            mobInput.get("inWater").getAsBoolean(),
            mobInput.has("maxFallDistance") ? mobInput.get("maxFallDistance").getAsInt() : -1);
        JsonObject mobPos = mobInput.getAsJsonObject("position");
        mob.placeAt(
            originX + mobPos.get("x").getAsDouble(),
            originY + mobPos.get("y").getAsDouble(),
            originZ + mobPos.get("z").getAsDouble(), 0.0F);
        for (var element : mobInput.getAsJsonArray("malus")) {
            JsonObject entry = element.getAsJsonObject();
            mob.setMalus(WalkPathTypeOracle.parsePathTypeName(entry.get("type").getAsString()), entry.get("value").getAsFloat());
        }

        int range = Math.max(sizeX, sizeZ) + 4;
        PathNavigationRegion region = new PathNavigationRegion(
            level,
            new BlockPos(originX - 4, originY - 4, originZ - 4),
            new BlockPos(originX + range, originY + sizeY + 4, originZ + range));

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        // 3. 起点与邻居：飞行求值器的 26 邻居枚举是它最容易偏的地方。
        FlyNodeEvaluator evaluator = new FlyNodeEvaluator();
        evaluator.prepare(region, mob);
        PathfindingContext context = new PathfindingContext(region, mob);
        Node start = evaluator.getStart();
        if (start == null) {
            result.addProperty("start", (String) null);
            result.add("neighbors", new JsonArray());
            evaluator.done();
            result.addProperty("path", (String) null);
            return result;
        }
        JsonObject startObject = new JsonObject();
        startObject.addProperty("x", start.x);
        startObject.addProperty("y", start.y);
        startObject.addProperty("z", start.z);
        startObject.addProperty("type", WalkPathTypeOracle.pathTypeName(start.type));
        result.add("start", startObject);

        Node[] buffer = new Node[64];
        int neighborCount = evaluator.getNeighbors(buffer, start);
        JsonArray neighbors = new JsonArray();
        for (int i = 0; i < neighborCount; i++) {
            Node node = buffer[i];
            JsonObject entry = new JsonObject();
            entry.addProperty("x", node.x);
            entry.addProperty("y", node.y);
            entry.addProperty("z", node.z);
            entry.addProperty("type", WalkPathTypeOracle.pathTypeName(node.type));
            entry.addProperty("malus", node.costMalus);
            neighbors.add(entry);
        }
        result.add("neighbors", neighbors);
        evaluator.done();

        // 4. 正式搜索。
        Set<BlockPos> targets = new LinkedHashSet<>();
        List<BlockPos> ordered = new ArrayList<>();
        for (var element : input.getAsJsonArray("targets")) {
            JsonObject target = element.getAsJsonObject();
            BlockPos pos = new BlockPos(
                originX + target.get("x").getAsInt(),
                originY + target.get("y").getAsInt(),
                originZ + target.get("z").getAsInt());
            targets.add(pos);
            ordered.add(pos);
        }
        FlyNodeEvaluator searchEvaluator = new FlyNodeEvaluator();
        PathFinder pathFinder = new PathFinder(searchEvaluator, input.get("maxVisitedNodes").getAsInt());
        net.minecraft.world.level.pathfinder.Path path = pathFinder.findPath(
            region,
            mob,
            targets,
            input.get("maxPathLength").getAsFloat(),
            input.get("reachRange").getAsInt(),
            input.get("maxVisitedNodesMultiplier").getAsFloat());
        if (path == null) {
            result.addProperty("path", (String) null);
            return result;
        }
        JsonArray nodes = new JsonArray();
        for (int i = 0; i < path.getNodeCount(); i++) {
            Node node = path.getNode(i);
            JsonObject entry = new JsonObject();
            entry.addProperty("x", node.x);
            entry.addProperty("y", node.y);
            entry.addProperty("z", node.z);
            entry.addProperty("type", WalkPathTypeOracle.pathTypeName(node.type));
            nodes.add(entry);
        }
        result.add("path", nodes);
        result.addProperty("canReach", path.canReach());
        result.addProperty("endX", path.getEndNode() == null ? 0 : path.getEndNode().x);
        result.addProperty("endY", path.getEndNode() == null ? 0 : path.getEndNode().y);
        result.addProperty("endZ", path.getEndNode() == null ? 0 : path.getEndNode().z);
        return result;
    }
}