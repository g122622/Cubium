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
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.PathNavigationRegion;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.pathfinder.Node;
import net.minecraft.world.level.pathfinder.PathFinder;
import net.minecraft.world.level.pathfinder.PathType;
import net.minecraft.world.level.pathfinder.PathfindingContext;
import net.minecraft.world.level.pathfinder.WalkNodeEvaluator;

/**
 * A* 寻路 oracle：跑原版 {@code PathFinder}，输出整条路径与沿途的邻居枚举。
 *
 * <p>输出分两段，对应「搜索过程」与「搜索结果」：
 *
 * <ul>
 *   <li>{@code neighbors}：起点节点在四个水平方向与四个对角方向上的邻居，含每个邻居的
 *       类型、代价与是否被接受。邻居枚举是 A* 的全部输入，它对了路径才可能对；
 *   <li>{@code nodes}：{@code findPath} 返回的节点序列（坐标 + 类型 + 代价）。比对不对
 *       节点代价的浮点（末位会因求值顺序不同而差 1 ulp），只比坐标与类型。
 * </ul>
 *
 * <p>同时输出 {@code canReach} 与 {@code distanceToTarget}：它们是上层 AI 判断
 * 「到没到」的唯一依据，比整条路径更早暴露语义偏差。
 */
public final class PathFinderOracle {
    private static final Gson GSON = new Gson();

    private PathFinderOracle() {}

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), List.of(), StandardCharsets.UTF_8);
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: PathFinderOracle list <units> | run <input> <output>");
        }

        PathWorld world = PathWorld.get();
        ServerLevel level = world.level;
        var blocks = level.registryAccess().lookupOrThrow(Registries.BLOCK);
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

        // 1. 布置场景：先清出一块空白场地，再摆方块。
        world.clear(originX, originZ, Math.max(sizeX, sizeZ) + 4, originY - 4, sizeY + 8);
        for (var element : input.getAsJsonArray("cells")) {
            JsonObject cell = element.getAsJsonObject();
            BlockPos pos = new BlockPos(
                originX + cell.get("dx").getAsInt(),
                originY + cell.get("dy").getAsInt(),
                originZ + cell.get("dz").getAsInt());
            BlockState state = WalkPathTypeOracle.parseBlockChecked(blocks, cell.get("block").getAsString());
            world.setBlockDirect(pos.getX(), pos.getY(), pos.getZ(), state);
        world.invalidatePathTypeCache();
        }

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

        JsonObject flags = input.getAsJsonObject("flags");

        // 3. 区域与寻路器。能力开关必须设在 evaluator 上（PathFinder 只负责搜索）。
        int range = Math.max(sizeX, sizeZ) + 4;
        PathNavigationRegion region = new PathNavigationRegion(
            level,
            new BlockPos(originX - 4, originY - 4, originZ - 4),
            new BlockPos(originX + range, originY + sizeY + 4, originZ + range));
        WalkNodeEvaluator evaluator = new WalkNodeEvaluator();
        evaluator.setCanPassDoors(flags.get("canPassDoors").getAsBoolean());
        evaluator.setCanOpenDoors(flags.get("canOpenDoors").getAsBoolean());
        evaluator.setCanFloat(flags.get("canFloat").getAsBoolean());
        evaluator.setCanWalkOverFences(flags.get("canWalkOverFences").getAsBoolean());

        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        // 4. 邻居枚举：先开会话，取起点，再把起点展开一次。
        PathFinder finder = new PathFinder(evaluator, input.get("maxVisitedNodes").getAsInt());
        evaluator.prepare(region, mob);
        PathfindingContext context = new PathfindingContext(region, mob);
        Node start = evaluator.getStart();
        if (start == null) {
            result.addProperty("start", (String) null);
            result.add("neighbors", new JsonArray());
            evaluator.done();
            return result;
        }
        JsonObject startObject = new JsonObject();
        startObject.addProperty("x", start.x);
        startObject.addProperty("y", start.y);
        startObject.addProperty("z", start.z);
        startObject.addProperty("type", WalkPathTypeOracle.pathTypeName(start.type));
        startObject.addProperty("malus", start.costMalus);
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

        // 5. 真正跑一次 A*：目标集合按输入给的目标逐个加入。
        Set<BlockPos> targets = new LinkedHashSet<>();
        List<BlockPos> orderedTargets = new ArrayList<>();
        for (var element : input.getAsJsonArray("targets")) {
            JsonObject target = element.getAsJsonObject();
            BlockPos pos = new BlockPos(
                originX + target.get("x").getAsInt(),
                originY + target.get("y").getAsInt(),
                originZ + target.get("z").getAsInt());
            targets.add(pos);
            orderedTargets.add(pos);
        }
        WalkNodeEvaluator searchEvaluator = new WalkNodeEvaluator();
        searchEvaluator.setCanPassDoors(flags.get("canPassDoors").getAsBoolean());
        searchEvaluator.setCanOpenDoors(flags.get("canOpenDoors").getAsBoolean());
        searchEvaluator.setCanFloat(flags.get("canFloat").getAsBoolean());
        searchEvaluator.setCanWalkOverFences(flags.get("canWalkOverFences").getAsBoolean());
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
