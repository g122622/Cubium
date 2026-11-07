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
import net.minecraft.core.Direction;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.shapes.Shapes;
import net.minecraft.world.phys.shapes.VoxelShape;

/**
 * 实体与方块形状的碰撞求解（原版 {@code Shapes.collide}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.ShapeCollideOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>场景表与 {@code probes/shape_collide.cpp} 逐项对应。两侧都调各自的真实实现，
 * 不在这里重写「求穿透深度、决定贴哪一面」的算法。
 */
public final class ShapeCollideOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private ShapeCollideOracle() {}

    /** 形状：方块局部坐标的包围盒。 */
    private record ShapeSpec(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {}

    /** 场景：实体箱 + 形状列表 + 移动轴与位移。 */
    private record Scene(
        String name,
        double boxMinX,
        double boxMinY,
        double boxMinZ,
        double boxMaxX,
        double boxMaxY,
        double boxMaxZ,
        List<ShapeSpec> shapes,
        Direction.Axis axis,
        double movement) {}

    private static ShapeSpec box(double minX, double minY, double minZ, double maxX, double maxY, double maxZ) {
        return new ShapeSpec(minX, minY, minZ, maxX, maxY, maxZ);
    }

    private static final List<Scene> SCENES = List.of(
        new Scene("wall_full_block_x_pos", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 1.0, 1.0)), Direction.Axis.X, 0.5),
        new Scene("wall_full_block_short", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 1.0, 1.0)), Direction.Axis.X, 0.2),
        new Scene("wall_full_block_x_neg", 0.3, 0.0, -0.3, 0.9, 1.8, 0.3,
            List.of(box(-2.0, 0.0, 0.0, -1.0, 1.0, 1.0)), Direction.Axis.X, -0.5),
        new Scene("pinched_two_blocks", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(-2.0, 0.0, 0.0, -1.0, 1.0, 1.0), box(1.0, 0.0, 0.0, 2.0, 1.0, 1.0)),
            Direction.Axis.X, 0.5),
        new Scene("slab_below_entity", -0.3, 0.5, -0.3, 0.3, 2.3, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 0.5, 1.0)), Direction.Axis.X, 0.5),
        new Scene("slab_flush_below", -0.3, 0.5, -0.3, 0.3, 2.3, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 0.5, 1.0)), Direction.Axis.X, 0.5),
        new Scene("fence_column", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.375, 0.0, 0.375, 1.625, 1.5, 0.625)), Direction.Axis.X, 0.5),
        new Scene("partial_y_overlap", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.0, 1.5, 0.0, 2.0, 2.5, 1.0)), Direction.Axis.X, 0.5),
        new Scene("fall_onto_ground", -0.3, 2.0, -0.3, 0.3, 3.8, 0.3,
            List.of(box(-1.0, 0.0, -1.0, 1.0, 1.0, 1.0)), Direction.Axis.Y, -1.5),
        new Scene("jump_into_ceiling", -0.3, 1.0, -0.3, 0.3, 2.8, 0.3,
            List.of(box(-1.0, 3.0, -1.0, 1.0, 4.0, 1.0)), Direction.Axis.Y, 0.5),
        new Scene("wall_full_block_z_pos", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(0.0, 0.0, 1.0, 1.0, 1.0, 2.0)), Direction.Axis.Z, 0.5),
        new Scene("no_shapes", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3, List.of(), Direction.Axis.X, 0.5),
        new Scene("zero_movement", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(-2.0, 0.0, -2.0, 2.0, 2.0, 2.0)), Direction.Axis.X, 0.0),
        new Scene("touching_no_overlap", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(0.3, 0.0, -1.0, 1.3, 1.0, 1.0)), Direction.Axis.X, 0.5),
        new Scene("already_overlapping", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(0.0, 0.0, -1.0, 1.0, 1.0, 1.0)), Direction.Axis.X, 0.5),
        new Scene("two_adjacent_shapes", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 1.0, 1.0), box(2.0, 0.0, 0.0, 3.0, 1.0, 1.0)),
            Direction.Axis.X, 3.0),
        new Scene("thin_shape_small_move", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(0.5, 0.0, 0.0, 0.5625, 1.0, 1.0)), Direction.Axis.X, 0.1),
        new Scene("fractional_move", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(1.0, 0.0, 0.0, 2.0, 1.0, 1.0)), Direction.Axis.X, 0.3737),
        new Scene("large_move", -0.3, 0.0, -0.3, 0.3, 1.8, 0.3,
            List.of(box(5.0, 0.0, 0.0, 6.0, 1.0, 1.0)), Direction.Axis.X, 10.0)
    );

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (Scene scene : SCENES) {
                text.append(scene.name()).append('\n');
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

        throw new IllegalArgumentException("usage: ShapeCollideOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        Scene scene = SCENES.stream().filter(candidate -> candidate.name().equals(requested)).findFirst().orElse(null);
        if (scene == null) {
            return missing(requested);
        }

        AABB entityBox = new AABB(
            scene.boxMinX(), scene.boxMinY(), scene.boxMinZ(), scene.boxMaxX(), scene.boxMaxY(), scene.boxMaxZ());

        List<VoxelShape> shapes = new ArrayList<>(scene.shapes().size());
        for (ShapeSpec spec : scene.shapes()) {
            shapes.add(Shapes.box(spec.minX(), spec.minY(), spec.minZ(), spec.maxX(), spec.maxY(), spec.maxZ()));
        }

        double resolved = Shapes.collide(scene.axis(), entityBox, shapes, scene.movement());

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        JsonFloats.add(output, "collided", resolved);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}