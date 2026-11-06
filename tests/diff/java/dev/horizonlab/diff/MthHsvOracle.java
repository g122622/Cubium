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
 * {@code Mth.hsvToArgb} / {@code hsvToRgb} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.MthHsvOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.MthHsvOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>色相按 60 度一档切成六段，每段走不同的分量排列。用例取色相 × 饱和度 × 明度 × alpha
 * 的笛卡尔积，覆盖每段的中点与边界、饱和度为 0 的灰色、明度为 0 的黑色，以及 alpha 的
 * 越界值。
 *
 * <p>注意 {@code hsvToRgb} 的 alpha 恒为 <b>0</b> 而不是 255——原版就是这么写的。
 */
public final class MthHsvOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private MthHsvOracle() {}

    /** 与探针侧逐项一致。 */
    private record NamedFloat(String id, float value) {}

    private static List<NamedFloat> hueCases() {
        List<NamedFloat> list = new ArrayList<>();
        // 六段中点
        list.add(new NamedFloat("h_0", 0.0833333f));
        list.add(new NamedFloat("h_1", 0.25f));
        list.add(new NamedFloat("h_2", 0.4166667f));
        list.add(new NamedFloat("h_3", 0.5833333f));
        list.add(new NamedFloat("h_4", 0.75f));
        list.add(new NamedFloat("h_5", 0.9166667f));
        // 段边界
        list.add(new NamedFloat("b_0", 0.0f));
        list.add(new NamedFloat("b_1", 0.1666667f));
        list.add(new NamedFloat("b_2", 0.3333333f));
        list.add(new NamedFloat("b_3", 0.5f));
        list.add(new NamedFloat("b_4", 0.6666667f));
        list.add(new NamedFloat("b_5", 0.8333333f));
        // 回绕
        list.add(new NamedFloat("wrap_1", 1.0f));
        // 纯红端点
        list.add(new NamedFloat("red", 0.0f));
        return list;
    }

    private static List<NamedFloat> saturationCases() {
        List<NamedFloat> list = new ArrayList<>();
        list.add(new NamedFloat("s_0", 0.0f));
        list.add(new NamedFloat("s_q", 0.25f));
        list.add(new NamedFloat("s_h", 0.5f));
        list.add(new NamedFloat("s_1", 1.0f));
        return list;
    }

    private static List<NamedFloat> valueCases() {
        List<NamedFloat> list = new ArrayList<>();
        list.add(new NamedFloat("v_0", 0.0f));
        list.add(new NamedFloat("v_q", 0.25f));
        list.add(new NamedFloat("v_h", 0.5f));
        list.add(new NamedFloat("v_1", 1.0f));
        return list;
    }

    private static final int[] ALPHA_CASES = {0, 127, 255, 256};

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

        throw new IllegalArgumentException("usage: MthHsvOracle list <units.txt> | run <in> <out>");
    }

    /** 与探针侧 writeList 逐项一致。 */
    private static String buildListing() {
        StringBuilder text = new StringBuilder();
        int index = 0;
        for (NamedFloat hue : hueCases()) {
            for (NamedFloat sat : saturationCases()) {
                for (NamedFloat val : valueCases()) {
                    for (int alpha : ALPHA_CASES) {
                        text.append(hue.id()).append('|').append(sat.id()).append('|')
                            .append(val.id()).append('|').append(alpha).append('@').append(index).append('\n');
                        index++;
                    }
                }
            }
        }
        return text.toString();
    }

    private static NamedFloat lookup(List<NamedFloat> cases, String id) {
        for (NamedFloat item : cases) {
            if (item.id().equals(id)) {
                return item;
            }
        }
        return null;
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String text = input.get("unit").getAsString();

        int at = text.lastIndexOf('@');
        if (at < 0) {
            return missing(text);
        }
        String[] parts = text.substring(0, at).split("\\|", -1);
        if (parts.length != 4) {
            return missing(text);
        }

        NamedFloat hue = lookup(hueCases(), parts[0]);
        NamedFloat sat = lookup(saturationCases(), parts[1]);
        NamedFloat val = lookup(valueCases(), parts[2]);
        if (hue == null || sat == null || val == null) {
            return missing(text);
        }

        int alpha;
        try {
            alpha = Integer.parseInt(parts[3]);
        } catch (NumberFormatException error) {
            return missing(text);
        }

        JsonObject output = new JsonObject();
        output.addProperty("unit", text);
        // 用无符号转 long，避免 Java int 的符号位在序列化成 JSON 数字时变负。
        output.addProperty(
                "argb", Integer.toUnsignedLong(Mth.hsvToArgb(hue.value(), sat.value(), val.value(), alpha)));
        output.addProperty("rgb", Integer.toUnsignedLong(Mth.hsvToRgb(hue.value(), sat.value(), val.value())));
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}