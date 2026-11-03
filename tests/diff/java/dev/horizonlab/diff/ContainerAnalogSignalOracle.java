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

/**
 * 容器红石信号强度的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ContainerAnalogSignalOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么不直接调原版方法</h2>
 *
 * {@code AbstractContainerMenu.getRedstoneSignalFromContainer} 需要一个 {@code Container}
 * 实例。本能力把它的公式完整搬到参数上，逐行对应原版：
 *
 * <pre>
 *   float f = 0.0F;
 *   for (int i = 0; i < container.getContainerSize(); i++) {
 *       ItemStack stack = container.getItem(i);
 *       if (!stack.isEmpty()) f += (float)stack.getCount() / container.getMaxStackSize(stack);
 *   }
 *   f /= container.getContainerSize();
 *   return Mth.lerpDiscrete(f, 0, 15);
 * </pre>
 *
 * 其中 {@code Mth.lerpDiscrete} 直接调用原版实现，验的是真实的取整语义。
 */
public final class ContainerAnalogSignalOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 C++ 探针 SCENES 完全一致。 */
    private static final String[] SCENES = {
        "27;",
        "9;0/64|0/64|0/64|0/64|0/64|0/64|0/64|0/64|0/64",
        "1;1/64",
        "1;32/64",
        "1;64/64",
        "1;1/1",
        "27;1/64",
        "27;64/64",
        "9;1/1|1/1|1/1|||",
        "9;64/64|64/64|64/64|64/64|||",
        "9;64/64|64/64|64/64|64/64|64/64|64/64|64/64|64/64|64/64",
        "1;16/64",
        "1;48/64",
        "54;64/64",
        "54;64/64|64/64|64/64",
    };

    private ContainerAnalogSignalOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            Files.write(Path.of(args[1]), new ArrayList<>(List.of(SCENES)), StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: ContainerAnalogSignalOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int semicolon = unit.indexOf(';');
        int slots = Integer.parseInt(unit.substring(0, semicolon));
        String cellsPart = unit.substring(semicolon + 1);

        // 逐格取 (count, max)，空格子为 (0, 64)。
        int[][] cells = new int[slots][2];
        for (int i = 0; i < slots; i++) {
            cells[i][0] = 0;
            cells[i][1] = 64;
        }

        if (!cellsPart.isEmpty()) {
            String[] tokens = cellsPart.split("\\|", -1);
            for (int i = 0; i < tokens.length && i < slots; i++) {
                if (tokens[i].isEmpty()) {
                    continue;
                }
                int slash = tokens[i].indexOf('/');
                cells[i][0] = Integer.parseInt(tokens[i].substring(0, slash));
                cells[i][1] = Integer.parseInt(tokens[i].substring(slash + 1));
            }
        }

        out.addProperty("signal", signalFrom(cells));
        return out;
    }

    /** 原版 {@code getRedstoneSignalFromContainer} 的公式。 */
    private static int signalFrom(int[][] cells) {
        if (cells.length == 0) {
            return 0;
        }

        float f = 0.0F;
        for (int[] cell : cells) {
            if (cell[0] > 0) {
                f += (float) cell[0] / (float) cell[1];
            }
        }
        f /= cells.length;

        // 直接调原版实现，验真实的取整语义。
        return Mth.lerpDiscrete(f, 0, 15);
    }
}
