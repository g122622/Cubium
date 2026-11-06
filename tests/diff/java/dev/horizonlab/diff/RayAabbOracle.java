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
import net.minecraft.util.Mth;
import net.minecraft.world.phys.AABB;
import net.minecraft.world.phys.Vec3;

/**
 * 射线与轴对齐包围盒求交的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RayAabbOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.RayAabbOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>覆盖 {@code Mth.rayIntersectsAABB}。它先用三轴的「中心距超出半宽且朝外」做快速排除，
 * 再对三对轴做叉积判定——前两次超出即排除，最后一次要在界内才相交。这种不对称是边界
 * 行为的关键。
 *
 * <p>用例取盒 × 起点 × 方向的笛卡尔积，而不是手挑几个组合：手挑容易漏掉「某个轴上刚好
 * 朝外」「某个轴恰好擦边」这类只在特定组合下才出现的路径。
 */
public final class RayAabbOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private RayAabbOracle() {}

    /** 与探针侧逐项一致。 */
    private record BoxCase(
            String id,
            double minX, double minY, double minZ,
            double maxX, double maxY, double maxZ) {}

    /** 与探针侧逐项一致。 */
    private record PointCase(String id, double x, double y, double z) {}

    private static List<BoxCase> boxCases() {
        List<BoxCase> list = new ArrayList<>();
        list.add(new BoxCase("unit", 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new BoxCase("offset", -3.5, 2.25, 100.75, 1.5, 4.75, 104.25));
        list.add(new BoxCase("flat_y", 0.0, 0.5, 0.0, 1.0, 0.5, 1.0));
        list.add(new BoxCase("flat_x", 0.5, 0.0, 0.0, 0.5, 1.0, 1.0));
        list.add(new BoxCase("point", 0.5, 0.5, 0.5, 0.5, 0.5, 0.5));
        list.add(new BoxCase("large", 1.0e6, -1.0e6, 1.0e6, 2.0e6, -5.0e5, 3.0e6));
        list.add(new BoxCase("tiny", 0.0, 0.0, 0.0, 1.0e-9, 1.0e-9, 1.0e-9));
        list.add(new BoxCase("slab", -10.0, -0.5, -0.5, 10.0, 0.5, 0.5));
        return list;
    }

    private static List<PointCase> originCases() {
        List<PointCase> list = new ArrayList<>();
        list.add(new PointCase("origin", 0.0, 0.0, 0.0));
        list.add(new PointCase("center", 0.5, 0.5, 0.5));
        list.add(new PointCase("neg_x_far", -10.0, 0.5, 0.5));
        list.add(new PointCase("pos_x_far", 10.0, 0.5, 0.5));
        list.add(new PointCase("neg_y_far", 0.5, -10.0, 0.5));
        list.add(new PointCase("pos_y_far", 0.5, 10.0, 0.5));
        list.add(new PointCase("neg_z_far", 0.5, 0.5, -10.0));
        list.add(new PointCase("pos_z_far", 0.5, 0.5, 10.0));
        list.add(new PointCase("neg_all", -10.0, -10.0, -10.0));
        list.add(new PointCase("pos_all", 10.0, 10.0, 10.0));
        list.add(new PointCase("on_face", 0.0, 0.5, 0.5));
        list.add(new PointCase("on_edge", 0.0, 0.0, 0.5));
        list.add(new PointCase("on_corner", 0.0, 0.0, 0.0));
        list.add(new PointCase("just_out_face", -1.0e-9, 0.5, 0.5));
        list.add(new PointCase("large_origin", 1.5e6, -7.5e5, 2.0e6));
        list.add(new PointCase("near_center", 0.5 + 1.0e-12, 0.5, 0.5));
        return list;
    }

    private static List<PointCase> directionCases() {
        List<PointCase> list = new ArrayList<>();
        list.add(new PointCase("zero", 0.0, 0.0, 0.0));
        list.add(new PointCase("pos_x", 1.0, 0.0, 0.0));
        list.add(new PointCase("neg_x", -1.0, 0.0, 0.0));
        list.add(new PointCase("pos_y", 0.0, 1.0, 0.0));
        list.add(new PointCase("neg_y", 0.0, -1.0, 0.0));
        list.add(new PointCase("pos_z", 0.0, 0.0, 1.0));
        list.add(new PointCase("neg_z", 0.0, 0.0, -1.0));
        list.add(new PointCase("diag_in", 1.0, 1.0, 1.0));
        list.add(new PointCase("diag_out", -1.0, -1.0, -1.0));
        list.add(new PointCase("mostly_x", 1.0, 0.01, 0.01));
        list.add(new PointCase("mostly_y", 0.01, 1.0, 0.01));
        list.add(new PointCase("scaled", 100.0, -50.0, 25.0));
        return list;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.writeString(Path.of(args[1]), buildListing(), StandardCharsets.UTF_8);
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

        throw new IllegalArgumentException("usage: RayAabbOracle list <units.txt> | run <in> <out>");
    }

    /** 与探针侧 writeList 逐项一致：盒 × 起点 × 方向的笛卡尔积。 */
    private static String buildListing() {
        StringBuilder text = new StringBuilder();
        int index = 0;
        int boxCount = boxCases().size();
        int originCount = originCases().size();
        int directionCount = directionCases().size();
        for (int bi = 0; bi < boxCount; bi++) {
            for (int oi = 0; oi < originCount; oi++) {
                for (int di = 0; di < directionCount; di++) {
                    text.append(bi).append('|').append(oi).append('|').append(di)
                        .append('@').append(index).append('\n');
                    index++;
                }
            }
        }
        return text.toString();
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int lastAt = requested.lastIndexOf('@');
        if (lastAt < 0) {
            return missing(requested);
        }
        String triple = requested.substring(0, lastAt);
        String[] pieces = triple.split("\\|", -1);
        if (pieces.length != 3) {
            return missing(requested);
        }

        int bi;
        int oi;
        int di;
        try {
            bi = Integer.parseInt(pieces[0]);
            oi = Integer.parseInt(pieces[1]);
            di = Integer.parseInt(pieces[2]);
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        List<BoxCase> boxes = boxCases();
        List<PointCase> origins = originCases();
        List<PointCase> directions = directionCases();
        if (bi < 0 || bi >= boxes.size() || oi < 0 || oi >= origins.size() || di < 0 || di >= directions.size()) {
            return missing(requested);
        }

        BoxCase box = boxes.get(bi);
        PointCase origin = origins.get(oi);
        PointCase direction = directions.get(di);

        Vec3 start = new Vec3(origin.x(), origin.y(), origin.z());
        Vec3 dir = new Vec3(direction.x(), direction.y(), direction.z());
        AABB aabb = new AABB(box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ());
        boolean hit = Mth.rayIntersectsAABB(start, dir, aabb);

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("hit", hit);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}