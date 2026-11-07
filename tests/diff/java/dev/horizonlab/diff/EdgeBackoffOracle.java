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
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.core.BlockPos;
import net.minecraft.world.level.EmptyBlockGetter;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.CollisionContext;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * 潜行时的边缘保护（原版 {@code Player.maybeBackOffFromEdge} + {@code canFallAtLeast}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.EdgeBackoffOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版这三个方法都是 private，且依赖真实 {@code Level}。这里复刻它们的**算法**——
 * 三条 while 回退循环是公开可见的短逻辑，且已逐字对齐；真正的被测对象是
 * 「平移并向下扩张的盒子是否有碰撞」这一世界查询，用 {@link MapBlockGetter} 按坐标
 * 给出石头/空气来驱动 {@code BlockState.getCollisionShape}。
 *
 * <p>场景表与 {@code probes/edge_backoff.cpp} 逐项对应。
 */
public final class EdgeBackoffOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private EdgeBackoffOracle() {}

    private record Scene(
        String name,
        double boxMinX,
        double boxMinY,
        double boxMinZ,
        double boxMaxX,
        double boxMaxY,
        double boxMaxZ,
        double moveX,
        double moveZ,
        double stepHeight,
        List<int[]> solids) {}

    private static List<int[]> platform(int fromX, int toX, int fromZ, int toZ) {
        List<int[]> cells = new ArrayList<>();
        for (int x = fromX; x <= toX; x++) {
            for (int z = fromZ; z <= toZ; z++) {
                cells.add(new int[] {x, 0, z});
            }
        }
        return cells;
    }

    private static final List<Scene> SCENES = List.of(
        new Scene("center_of_3x3_move_x", 1.7, 1.0, 0.7, 2.3, 2.8, 1.3, 0.5, 0.0, 0.6, platform(0, 2, 0, 2)),
        new Scene("walk_off_edge_x", 2.2, 1.0, 0.7, 2.8, 2.8, 1.3, 1.0, 0.0, 0.6, platform(0, 2, 0, 2)),
        new Scene("slide_along_edge", 1.9, 1.0, 0.7, 2.5, 2.8, 1.3, 1.0, 1.0, 0.6, platform(0, 2, 0, 2)),
        new Scene("corner_both_fall", 1.9, 1.0, 1.9, 2.5, 2.8, 2.5, 1.0, 1.0, 0.6, platform(0, 2, 0, 2)),
        new Scene("move_inward", 1.9, 1.0, 0.7, 2.5, 2.8, 1.3, -1.0, 0.0, 0.6, platform(0, 2, 0, 2)),
        new Scene("single_block", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, platform(0, 0, 0, 0)),
        new Scene("narrow_strip_z", 0.2, 1.0, -0.3, 0.8, 2.8, 0.3, 0.0, 1.0, 0.6, platform(0, 0, -1, 1)),
        new Scene("small_step_inward", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.1, 0.0, 0.6, platform(0, 1, 0, 1)),
        new Scene("exactly_step_epsilon", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.05, 0.0, 0.6, platform(0, 0, 0, 0)),
        new Scene("tall_step_absorbs", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, platform(0, 1, 0, 1)),
        new Scene("move_negative_x", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, -1.0, 0.0, 0.6, platform(0, 0, 0, 0)),
        new Scene("no_ground", 0.2, 1.0, 0.2, 0.8, 2.8, 0.8, 0.5, 0.0, 0.6, List.of())
    );

    /** 按坐标给方块的最小世界：石头或空气。 */
    private static final class MapBlockGetter {
        private final Map<Long, Boolean> solid = new HashMap<>();

        void addSolid(int x, int y, int z) {
            solid.put(BlockPos.asLong(x, y, z), Boolean.TRUE);
        }

        BlockState getBlockState(int x, int y, int z) {
            return solid.containsKey(BlockPos.asLong(x, y, z)) ? Blocks.STONE.defaultBlockState()
                : Blocks.AIR.defaultBlockState();
        }

        /** 与 {@code Level.noCollision(entity, box)} 等价：盒子里没有任何方块碰撞形状。 */
        boolean noCollision(AABB box) {
            int minX = (int) Math.floor(box.minX) - 1;
            int maxX = (int) Math.floor(box.maxX) + 1;
            int minY = (int) Math.floor(box.minY) - 1;
            int maxY = (int) Math.floor(box.maxY) + 1;
            int minZ = (int) Math.floor(box.minZ) - 1;
            int maxZ = (int) Math.floor(box.maxZ) + 1;
            for (int x = minX; x <= maxX; x++) {
                for (int y = minY; y <= maxY; y++) {
                    for (int z = minZ; z <= maxZ; z++) {
                        BlockState state = getBlockState(x, y, z);
                        VoxelShape shape = state.getCollisionShape(EmptyBlockGetter.INSTANCE, new BlockPos(x, y, z));
                        if (shape.isEmpty()) {
                            continue;
                        }
                        // 方块碰撞形状要平移到世界坐标再比。
                        for (AABB part : shape.toAabbs()) {
                            AABB world = part.move(x, y, z);
                            if (world.intersects(box)) {
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }
    }

    /** 原版 {@code canFallAtLeast(dx, dz, step)}。 */
    private static boolean canFallAtLeast(MapBlockGetter level, AABB box, double dx, double dz, double step) {
        return level.noCollision(new AABB(
            box.minX + 1.0E-7 + dx,
            box.minY - step - 1.0E-7,
            box.minZ + 1.0E-7 + dz,
            box.maxX - 1.0E-7 + dx,
            box.minY,
            box.maxZ - 1.0E-7 + dz));
    }

    /** 原版 {@code maybeBackOffFromEdge} 的三条 while 回退循环。 */
    private static double[] backOff(MapBlockGetter level, AABB box, double dx, double dz, double stepHeight) {
        final double backOffStep = 0.05;
        double stepX = Math.signum(dx) * backOffStep;
        double stepZ = Math.signum(dz) * backOffStep;

        while (dx != 0.0 && canFallAtLeast(level, box, dx, 0.0, stepHeight)) {
            if (Math.abs(dx) <= backOffStep) {
                dx = 0.0;
                break;
            }
            dx -= stepX;
        }

        while (dz != 0.0 && canFallAtLeast(level, box, 0.0, dz, stepHeight)) {
            if (Math.abs(dz) <= backOffStep) {
                dz = 0.0;
                break;
            }
            dz -= stepZ;
        }

        while (dx != 0.0 && dz != 0.0 && canFallAtLeast(level, box, dx, dz, stepHeight)) {
            if (Math.abs(dx) <= backOffStep) {
                dx = 0.0;
            } else {
                dx -= stepX;
            }
            if (Math.abs(dz) <= backOffStep) {
                dz = 0.0;
            } else {
                dz -= stepZ;
            }
        }

        return new double[] {dx, dz};
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> names = new ArrayList<>();
            for (Scene scene : SCENES) {
                names.add(scene.name());
            }
            names.sort(Comparator.naturalOrder());
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: EdgeBackoffOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        Scene scene = SCENES.stream().filter(candidate -> candidate.name().equals(requested)).findFirst().orElse(null);
        if (scene == null) {
            return missing(requested);
        }

        MapBlockGetter level = new MapBlockGetter();
        for (int[] cell : scene.solids()) {
            level.addSolid(cell[0], cell[1], cell[2]);
        }

        AABB box = new AABB(
            scene.boxMinX(), scene.boxMinY(), scene.boxMinZ(), scene.boxMaxX(), scene.boxMaxY(), scene.boxMaxZ());
        double[] resolved = backOff(level, box, scene.moveX(), scene.moveZ(), scene.stepHeight());

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        JsonFloats.add(output, "resolvedX", resolved[0]);
        JsonFloats.add(output, "resolvedZ", resolved[1]);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}