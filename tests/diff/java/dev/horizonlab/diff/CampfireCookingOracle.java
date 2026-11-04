package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Stream;

/**
 * 营火烹饪配方表的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CampfireCookingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <h2>为什么直接读数据包 JSON</h2>
 *
 * 营火只认 {@code minecraft:campfire_cooking} 类型的配方，而 {@code cookingtime} /
 * {@code experience} / 结果物品**全部来自数据包**（原版运行时也如此）。
 * {@code RecipeManager.getRecipeFor} 需要一个真实 {@code Level}，而本能力比对的是纯数据，
 * 因此直接从同一个数据包根目录读 JSON —— 这与 Cubium 侧
 * （{@code RecipeLoader::loadRecipeJson}）读的是同一份文件，比对是公平的。
 *
 * <h2>单元格式</h2>
 *
 * - 配方单元：{@code recipe:<配方id>}
 * - 查询单元：{@code lookup:<输入物品id>}
 */
public final class CampfireCookingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 与 C++ 探针 INPUTS 完全一致。 */
    private static final String[] INPUTS = {
        "minecraft:beef",
        "minecraft:chicken",
        "minecraft:cod",
        "minecraft:mutton",
        "minecraft:porkchop",
        "minecraft:rabbit",
        "minecraft:salmon",
        "minecraft:potato",
        "minecraft:kelp",
        "minecraft:brown_mushroom",
        "minecraft:red_mushroom",
        "minecraft:cobblestone",
        "minecraft:iron_ore",
        "minecraft:sand",
        "minecraft:porkchop",
        "minecraft:stick",
        "minecraft:apple",
    };

    /** 配方 ID -> 解析后的字段。 */
    private record RecipeEntry(int cookTime, float experience, String result, List<String> ingredients) {}

    private static Map<String, RecipeEntry> RECIPES;

    private CampfireCookingOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String id : recipes().keySet()) {
                units.add("recipe:" + id);
            }
            for (String input : INPUTS) {
                units.add("lookup:" + input);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: CampfireCookingOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (unit.startsWith("lookup:")) {
            String itemId = unit.substring(7);

            // 按配方 ID 排序后取第一条匹配，与 RecipeManager 的 findFirst 语义一致。
            for (Map.Entry<String, RecipeEntry> entry : recipes().entrySet()) {
                if (entry.getValue().ingredients().contains(itemId)) {
                    out.addProperty("found", true);
                    out.addProperty("recipe", entry.getKey());
                    out.addProperty("cookTime", entry.getValue().cookTime());
                    out.addProperty("result", entry.getValue().result());
                    out.addProperty("experience", bits(entry.getValue().experience()));
                    return out;
                }
            }
            out.addProperty("found", false);
            return out;
        }

        if (unit.startsWith("recipe:")) {
            RecipeEntry entry = recipes().get(unit.substring(7));
            if (entry == null) {
                out.addProperty("missing", true);
                return out;
            }
            out.addProperty("cookTime", entry.cookTime());
            out.addProperty("result", entry.result());
            out.addProperty("experience", bits(entry.experience()));
            return out;
        }

        throw new IllegalArgumentException("unknown unit type: " + unit);
    }

    /** 惰性加载并缓存全部营火配方。 */
    private static synchronized Map<String, RecipeEntry> recipes() {
        if (RECIPES != null) {
            return RECIPES;
        }

        String root = System.getenv("MC_DIFF_VANILLA_DATAPACK");
        if (root == null || root.isBlank()) {
            throw new IllegalStateException("MC_DIFF_VANILLA_DATAPACK is not set");
        }

        Path recipeDir = Path.of(root, "data", "minecraft", "recipe");
        List<Map.Entry<String, RecipeEntry>> parsed = new ArrayList<>();

        if (Files.isDirectory(recipeDir)) {
            try (Stream<Path> files = Files.list(recipeDir)) {
                for (Path file : files.filter(p -> p.toString().endsWith(".json")).toList()) {
                    JsonObject json;
                    try {
                        json = JsonParser.parseString(Files.readString(file, StandardCharsets.UTF_8))
                                   .getAsJsonObject();
                    } catch (IOException | RuntimeException error) {
                        continue;
                    }
                    if (!"minecraft:campfire_cooking".equals(json.get("type").getAsString())) {
                        continue;
                    }

                    String id = "minecraft:" + file.getFileName().toString().replace(".json", "");
                    int cookTime = json.get("cookingtime").getAsInt();
                    float experience = (float) json.get("experience").getAsDouble();
                    String result = json.getAsJsonObject("result").get("id").getAsString();
                    parsed.add(Map.entry(id, new RecipeEntry(cookTime, experience, result, ingredients(json))));
                }
            } catch (IOException error) {
                throw new IllegalStateException("failed to list " + recipeDir, error);
            }
        }

        parsed.sort(Map.Entry.comparingByKey(Comparator.naturalOrder()));
        Map<String, RecipeEntry> out = new LinkedHashMap<>();
        for (Map.Entry<String, RecipeEntry> entry : parsed) {
            out.put(entry.getKey(), entry.getValue());
        }
        RECIPES = out;
        return out;
    }

    /**
     * 取出配方的输入物品集合。
     *
     * <p>1.21 起 {@code ingredient} 是**字符串或数组**（数组表示多选一），不再是
     * {@code {"item": ...}} 包装。这里统一摊平成物品 ID 列表。
     */
    private static List<String> ingredients(JsonObject json) {
        JsonElement element = json.get("ingredient");
        List<String> out = new ArrayList<>();

        if (element.isJsonPrimitive()) {
            out.add(element.getAsString());
        } else if (element.isJsonArray()) {
            JsonArray array = element.getAsJsonArray();
            for (JsonElement item : array) {
                if (item.isJsonPrimitive()) {
                    out.add(item.getAsString());
                } else if (item.isJsonObject() && item.getAsJsonObject().has("items")) {
                    JsonElement items = item.getAsJsonObject().get("items");
                    if (items.isJsonPrimitive()) {
                        out.add(items.getAsString());
                    } else {
                        for (JsonElement one : items.getAsJsonArray()) {
                            out.add(one.getAsString());
                        }
                    }
                }
            }
        }
        return out;
    }
}
