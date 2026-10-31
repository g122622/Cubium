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
import java.util.List;
import net.minecraft.commands.arguments.blocks.BlockStateParser;
import net.minecraft.core.BlockPos;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.PathNavigationRegion;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.pathfinder.PathType;
import net.minecraft.world.level.pathfinder.PathfindingContext;
import net.minecraft.world.level.pathfinder.WalkNodeEvaluator;

/**
 * 逐格路径类型 oracle：对场景网格的每一格，输出原版 {@code WalkNodeEvaluator} 判定的路径类型。
 *
 * <p>三层判定分别对应输入里的三个开关，覆盖范围从「只看这一格」到「看整个实体包围盒」：
 *
 * <ul>
 *   <li>{@code fromState}：{@code getPathTypeFromState}，只看该格方块与流体；
 *   <li>{@code static}：{@code getPathTypeStatic}，叠加「脚下是什么」与 3×3×3 危险邻域；
 *   <li>{@code ofMob}：{@code getPathTypeOfMob}，再叠加实体尺寸、门、铁轨、栅栏与窄实体规则。
 * </ul>
 *
 * <p>输入里的方块用原版的 {@code BlockStateParser} 解析（与命令 {@code /setblock} 同一套语法），
 * 因此两侧描述场景的字符串完全一致：Cubium 探针用同一份字符串走它自己的解析器。
 */
public final class WalkPathTypeOracle {
    private static final Gson GSON = new Gson();

    private WalkPathTypeOracle() {}

    public static void main(String[] args) throws Exception {
        if (args.length == 2 && args[0].equals("list")) {
            // 单元是场景名：由网格定义文件决定，两侧共用同一份。
            Files.write(Path.of(args[1]), List.of(), StandardCharsets.UTF_8);
            return;
        }
        if (args.length != 3 || !args[0].equals("run")) {
            throw new IllegalArgumentException("usage: WalkPathTypeOracle list <units> | run <input> <output>");
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

        // 1. 先把场地清空，再摆本场景的方块：清空范围完整覆盖所有可能被查询到的格子，
        // 否则上一个场景的残留方块会让本次判定读到不属于本场景的地形。
        // 清除范围必须完整覆盖「本场景可能踩到的所有格子」，否则上一个场景的方块会残留：
        // 场景网格含 dy = -1 的地面层，邻域查询还要看 y+1 层，因此上下各留足余量。
        world.clear(originX, originZ, Math.max(sizeX, sizeZ) + 4, originY - 4, sizeY + 8);
        JsonArray cells = input.getAsJsonArray("cells");
        for (var element : cells) {
            JsonObject cell = element.getAsJsonObject();
            BlockPos pos = new BlockPos(
                originX + cell.get("dx").getAsInt(),
                originY + cell.get("dy").getAsInt(),
                originZ + cell.get("dz").getAsInt());
            BlockState state = parseBlockChecked(blocks, cell.get("block").getAsString());
            world.setBlockDirect(pos.getX(), pos.getY(), pos.getZ(), state);
        world.invalidatePathTypeCache();
        }

        // 2. 实体属性。
        JsonObject mobInput = input.getAsJsonObject("mob");
        float width = mobInput.get("width").getAsFloat();
        float height = mobInput.get("height").getAsFloat();
        float stepHeight = mobInput.get("stepHeight").getAsFloat();
        boolean onGround = mobInput.get("onGround").getAsBoolean();
        boolean inWater = mobInput.get("inWater").getAsBoolean();
        int maxFallDistance = mobInput.has("maxFallDistance") ? mobInput.get("maxFallDistance").getAsInt() : -1;

        PathProbeMob mob = new PathProbeMob(level, width, height, stepHeight, onGround, inWater, maxFallDistance);
        JsonObject mobPos = mobInput.getAsJsonObject("position");
        mob.placeAt(
            originX + mobPos.get("x").getAsDouble(),
            originY + mobPos.get("y").getAsDouble(),
            originZ + mobPos.get("z").getAsDouble(), 0.0F);
        for (var element : mobInput.getAsJsonArray("malus")) {
            JsonObject entry = element.getAsJsonObject();
            mob.setMalus(parsePathTypeName(entry.get("type").getAsString()), entry.get("value").getAsFloat());
        }

        // 3. 求值器能力开关与会话。
        WalkNodeEvaluator evaluator = new WalkNodeEvaluator();
        JsonObject flags = input.getAsJsonObject("flags");
        evaluator.setCanPassDoors(flags.get("canPassDoors").getAsBoolean());
        evaluator.setCanOpenDoors(flags.get("canOpenDoors").getAsBoolean());
        evaluator.setCanFloat(flags.get("canFloat").getAsBoolean());
        evaluator.setCanWalkOverFences(flags.get("canWalkOverFences").getAsBoolean());

        BlockPos min = new BlockPos(originX - 2, originY - 2, originZ - 2);
        BlockPos max = new BlockPos(
            originX + Math.max(sizeX, sizeZ) + 2, originY + sizeY + 2, originZ + Math.max(sizeX, sizeZ) + 2);
        PathNavigationRegion region = new PathNavigationRegion(level, min, max);
        evaluator.prepare(region, mob);
        if (System.getenv("MC_DIFF_DEBUG_TAG") != null) { for (var c : input.getAsJsonArray("cells")) { var bp = new BlockPos(originX + c.getAsJsonObject().get("dx").getAsInt(), originY + c.getAsJsonObject().get("dy").getAsInt(), originZ + c.getAsJsonObject().get("dz").getAsInt()); var st = level.getBlockState(bp); if (!st.isAir()) { System.err.println("CELL " + bp + " " + net.minecraft.core.registries.BuiltInRegistries.BLOCK.getKey(st.getBlock()) + " top=" + st.getCollisionShape(level, bp).max(net.minecraft.core.Direction.Axis.Y) + " empty=" + st.getCollisionShape(level, bp).isEmpty() + " pathfindable=" + st.isPathfindable(net.minecraft.world.level.pathfinder.PathComputationType.LAND)); } } }
        PathfindingContext context = new PathfindingContext(region, mob);

        // 4. 逐格判定。
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);
        JsonArray typesFromState = new JsonArray();
        JsonArray typesStatic = new JsonArray();
        JsonArray typesOfMob = new JsonArray();
        List<String> errors = new ArrayList<>();
        for (int dy = 0; dy < sizeY; dy++) {
            for (int dz = 0; dz < sizeZ; dz++) {
                for (int dx = 0; dx < sizeX; dx++) {
                    int x = originX + dx;
                    int y = originY + dy;
                    int z = originZ + dz;
                    typesFromState.add(pathTypeName(WalkNodeEvaluatorBridge.fromState(region, x, y, z)));
                    typesStatic.add(pathTypeName(WalkNodeEvaluator.getPathTypeStatic(context, new BlockPos.MutableBlockPos(x, y, z))));
                    typesOfMob.add(pathTypeName(evaluator.getPathTypeOfMob(context, x, y, z, mob)));
                }
            }
        }
        evaluator.done();

        result.add("fromState", typesFromState);
        result.add("static", typesStatic);
        result.add("ofMob", typesOfMob);
        result.addProperty("fallDistance", mob.getMaxFallDistance());
        result.addProperty("entityWidth", net.minecraft.util.Mth.floor(width + 1.0F));
        result.addProperty("entityHeight", net.minecraft.util.Mth.floor(height + 1.0F));
        if (!errors.isEmpty()) {
            JsonArray errorArray = new JsonArray();
            errors.forEach(errorArray::add);
            result.add("errors", errorArray);
        }
        return result;
    }

    /** 原版 {@code getPathTypeFromState} 是 protected static，这里用同包子类暴露它。 */
    private static final class WalkNodeEvaluatorBridge extends WalkNodeEvaluator {
        static PathType fromState(net.minecraft.world.level.BlockGetter reader, int x, int y, int z) {
            return getPathTypeFromState(reader, new BlockPos(x, y, z));
        }
    }

    /** 枚举名转稳定的小写蛇形名（两侧枚举次序不同，名字才是可比口径）。 */
    static String pathTypeName(PathType type) {
        return type.name().toLowerCase(java.util.Locale.ROOT);
    }

    /** 小写蛇形名反查枚举值。 */
    static PathType parsePathTypeName(String name) {
        for (PathType type : PathType.values()) {
            if (pathTypeName(type).equals(name)) {
                return type;
            }
        }
        throw new IllegalArgumentException("未知的路径类型名：" + name);
    }

    /** 供其它 oracle 复用的方块名解析（同一套原版语法）。 */
    static BlockState parseBlock(ServerLevel level, String text) {
        var blocks = level.registryAccess().lookupOrThrow(Registries.BLOCK);
        return parseBlockChecked(blocks, text);
    }

    /** 解析方块状态；语法错误转成运行时异常（探针按行报错，不中断整个运行）。 */
    static BlockState parseBlockChecked(
        net.minecraft.core.HolderLookup<net.minecraft.world.level.block.Block> blocks, String text) {
        try {
            return BlockStateParser.parseForBlock(blocks, text, false).blockState();
        }
        catch (com.mojang.brigadier.exceptions.CommandSyntaxException error) {
            throw new IllegalArgumentException("无法解析方块状态：" + text, error);
        }
    }

    /** 供其它 oracle 复用：把一格的方块设成空气。 */
    static void clearBlock(ServerLevel level, BlockPos pos) {
        level.setBlock(pos, Blocks.AIR.defaultBlockState(), net.minecraft.world.level.block.Block.UPDATE_ALL);
    }

    /** 供其它 oracle 复用：按资源名取方块 id（诊断用）。 */
    static String blockId(BlockState state) {
        return net.minecraft.core.registries.BuiltInRegistries.BLOCK.getKey(state.getBlock()).toString();
    }

    /** 供其它 oracle 复用：解析实体注册名。 */
    static net.minecraft.world.entity.EntityType<?> entityType(String id) {
        return net.minecraft.core.registries.BuiltInRegistries.ENTITY_TYPE
            .getOptional(Identifier.parse(id))
            .orElseThrow(() -> new IllegalArgumentException("未知实体：" + id));
    }
}
