package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.BlockPos;
import net.minecraft.core.Direction;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.block.state.properties.BellAttachType;
import net.minecraft.world.level.block.state.properties.BlockStateProperties;

/**
 * 钟放置时附着方式的原版 oracle（{@code BellBlock#getStateForPlacement}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.BellPlacementOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code up:north:3}（点击面:玩家朝向:四邻坚固性位掩码）。
 *
 * <p>原版的 {@code getStateForPlacement} 需要真实的 {@code Level} 与 {@code BlockPlaceContext}。
 * 这里复现它的判定规则，输入全部显式给定：
 *
 * <ul>
 *   <li>放置位下方恒为坚固方块、上方恒为空气（否则 FLOOR / CEILING 都活不下来）
 *   <li>四邻按掩码：1=西、2=东、4=北、8=南 有「足够坚固的侧面」
 * </ul>
 *
 * <p>是否「足够坚固」用 {@code Block.canSupportRigidBlock} 判定 —— 原版
 * {@code FaceAttachedHorizontalDirectionalBlock.canAttach} 与
 * {@code BellBlock.canSurvive} 走的都是这条规则（stone 的 isFaceSturdy 全向为真）。
 */
public final class BellPlacementOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final Direction[] FACES = {
        Direction.DOWN, Direction.UP, Direction.NORTH, Direction.SOUTH, Direction.WEST, Direction.EAST
    };
    private static final Direction[] YAWS = {Direction.NORTH, Direction.SOUTH, Direction.WEST, Direction.EAST};

    private BellPlacementOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (Direction face : FACES) {
                for (Direction yaw : YAWS) {
                    for (int mask = 0; mask < 16; mask++) {
                        units.add(name(face) + ":" + name(yaw) + ":" + mask);
                    }
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: BellPlacementOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        String[] parts = unit.split(":");
        if (parts.length != 3) {
            out.addProperty("missing", true);
            return out;
        }
        Direction face = byName(parts[0]);
        Direction yaw = byName(parts[1]);
        int mask;
        try {
            mask = Integer.parseInt(parts[2]);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (face == null || yaw == null || mask < 0 || mask > 15) {
            out.addProperty("missing", true);
            return out;
        }

        // 场景：放置位下方坚固、上方空气；四邻按掩码
        boolean west = (mask & 1) != 0;
        boolean east = (mask & 2) != 0;
        boolean north = (mask & 4) != 0;
        boolean south = (mask & 8) != 0;

        Direction.Axis axis = face.getAxis();
        if (axis == Direction.Axis.Y) {
            // 顶面 -> FLOOR，底面 -> CEILING；朝向取玩家水平朝向
            BellAttachType attachment = face == Direction.DOWN ? BellAttachType.CEILING : BellAttachType.FLOOR;
            // 下方恒坚固 -> FLOOR 可存活；上方恒空气 -> CEILING 不可存活，回退到默认（floor/north）
            if (attachment == BellAttachType.FLOOR) {
                out.addProperty("facing", name(yaw));
                out.addProperty("attachment", "floor");
            } else {
                // CEILING 需要上方有中心支撑，场景里上方是空气 -> 原版 canSurvive 失败，返回 null，
                // 此时方块不会被放置；Cubium 回退到 defaultState。两侧口径取「原版放置结果」。
                out.addProperty("rejected", true);
            }
            return out;
        }

        // 侧向点击：朝向取点击面的反方向
        Direction supportDir = face.getOpposite();
        boolean doubleWall = axis == Direction.Axis.X ? (west && east) : (north && south);
        // supportDir 是「钟挂在哪面墙上」的方向，墙本身在 supportDir 那一侧：
        //   点东面 -> 墙在西侧 -> 看西位；点西面 -> 墙在东侧 -> 看东位；北南同理。
        boolean supportOk = axis == Direction.Axis.X ? (supportDir == Direction.WEST ? west : east)
            : (supportDir == Direction.NORTH ? north : south);

        if (supportOk) {
            out.addProperty("facing", name(supportDir));
            out.addProperty("attachment", doubleWall ? "double_wall" : "single_wall");
            return out;
        }

        // 侧墙不行 -> 地面（下方恒坚固）
        out.addProperty("facing", name(supportDir));
        out.addProperty("attachment", "floor");
        return out;
    }

    private static String name(Direction direction) {
        return direction.getSerializedName();
    }

    private static Direction byName(String value) {
        for (Direction direction : FACES) {
            if (direction.getSerializedName().equals(value)) {
                return direction;
            }
        }
        return null;
    }
}
