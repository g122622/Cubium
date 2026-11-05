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
 * 行为的关键，因此用例包含擦边、正好在界上、退化盒（零体积/扁平）等情形。
 */
public final class RayAabbOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private RayAabbOracle() {}

    /** 与探针侧逐项一致。 */
    private record RayCase(
            String id,
            double ox, double oy, double oz,
            double dx, double dy, double dz,
            double minX, double minY, double minZ,
            double maxX, double maxY, double maxZ) {}

    private static List<RayCase> rayCases() {
        List<RayCase> list = new ArrayList<>();
        list.add(new RayCase(
                "hit_x", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "inside", 0.5, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "away", -10.0, 0.5, 0.5, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "graze_edge", -10.0, 1.0, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "just_outside", -10.0, 1.0000001, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "diagonal", -5.0, -5.0, -5.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "corner_graze", -5.0, 3.0, -5.0, 1.0, -1.0, 1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "degenerate_point", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5));
        list.add(new RayCase(
                "degenerate_flat", -10.0, 0.5, 0.5, 1.0, 0.0, 0.0, 0.0, 0.5, 0.0, 1.0, 0.5, 1.0));
        list.add(new RayCase(
                "large_coords", -1.0e6, 5.0e5, 5.0e5, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0e6, 1.0e6, 1.0e6));
        list.add(new RayCase(
                "zero_dir", -10.0, 0.5, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
        list.add(new RayCase(
                "oblique", -3.0, 1.5, -3.0, 1.0, 0.25, 1.0, 0.0, 0.0, 0.0, 2.0, 3.0, 2.0));
        return list;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            List<RayCase> cases = rayCases();
            for (int i = 0; i < cases.size(); i++) {
                text.append(cases.get(i).id()).append('@').append(i).append('\n');
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

        throw new IllegalArgumentException("usage: RayAabbOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int at = requested.lastIndexOf('@');
        if (at < 0) {
            return missing(requested);
        }
        String caseId = requested.substring(0, at);
        int index;
        try {
            index = Integer.parseInt(requested.substring(at + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }
        List<RayCase> cases = rayCases();
        if (index < 0 || index >= cases.size()) {
            return missing(requested);
        }
        RayCase c = cases.get(index);
        if (!caseId.equals(c.id())) {
            return missing(requested);
        }

        Vec3 origin = new Vec3(c.ox(), c.oy(), c.oz());
        Vec3 direction = new Vec3(c.dx(), c.dy(), c.dz());
        AABB box = new AABB(c.minX(), c.minY(), c.minZ(), c.maxX(), c.maxY(), c.maxZ());
        boolean hit = Mth.rayIntersectsAABB(origin, direction, box);

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