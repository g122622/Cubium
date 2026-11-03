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
 * 四类烹饪配方横向对照的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CookingRecipeTypesOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * 直接从 {@code MC_DIFF_VANILLA_DATAPACK} 指向的数据包读四类烹饪配方 —— 与 Cubium 侧
 * {@code RecipeLoader} 读的是同一批文件。{@code RecipeManager.getRecipeFor} 需要真实
 * {@code Level}，本能力比对的是纯数据，因此走 JSON。
 */
public final class CookingRecipeTypesOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] FACILITIES = {"smelting", "smoking", "blasting", "campfire_cooking"};

    /** 与 C++ 探针 INPUTS 完全一致。 */
    private static final String[] INPUTS = {
        "minecraft:beef",
        "minecraft:porkchop",
        "minecraft:chicken",
        "minecraft:cod",
        "minecraft:salmon",
        "minecraft:potato",
        "minecraft:kelp",
        "minecraft:iron_ore",
        "minecraft:gold_ore",
        "minecraft:copper_ore",
        "minecraft:sand",
        "minecraft:cobblestone",
        "minecraft:clay_ball",
        "minecraft:iron_ingot",
        "minecraft:stick",
        "minecraft:apple",
    };

    private record RecipeEntry(String recipeId, String type, int cookTime, float experience,
                               String result, List<String> ingredients) {}

    /** 设施 -> (输入物品 -> 配方条目)。每个设施内按配方 ID 排序，与 findFirst 一致。 */
    private static Map<String, Map<String, RecipeEntry>> BY_FACILITY;

    private CookingRecipeTypesOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String facility : FACILITIES) {
                for (String input : INPUTS) {
                    units.add(facility + "|" + input);
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
            "usage: CookingRecipeTypesOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int sep = unit.indexOf('|');
        String facility = unit.substring(0, sep);
        String itemId = unit.substring(sep + 1);

        RecipeEntry entry = recipes(facility).get(itemId);
        if (entry == null) {
            out.addProperty("found", false);
            return out;
        }

        out.addProperty("found", true);
        out.addProperty("recipe", entry.recipeId());
        out.addProperty("cookTime", entry.cookTime());
        out.addProperty("result", entry.result());
        out.addProperty("experience", bits(entry.experience()));
        return out;
    }

    private static Map<String, RecipeEntry> recipes(String facility) {
        return load().getOrDefault(facility, Map.of());
    }

    private static synchronized Map<String, Map<String, RecipeEntry>> load() {
        if (BY_FACILITY != null) {
            return BY_FACILITY;
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
                    if (!json.has("type")) {
                        continue;
                    }
                    String type = json.get("type").getAsString();
                    String shortType = type.substring(type.indexOf(':') + 1);
                    boolean isCooking = false;
                    for (String facility : FACILITIES) {
                        if (facility.equals(shortType)) {
                            isCooking = true;
                            break;
                        }
                    }
                    if (!isCooking) {
                        continue;
                    }

                    String id = "minecraft:" + file.getFileName().toString().replace(".json", "");
                    int cookTime = json.get("cookingtime").getAsInt();
                    float experience = (float) json.get("experience").getAsDouble();
                    String result = json.getAsJsonObject("result").get("id").getAsString();
                    parsed.add(Map.entry(id,
                        new RecipeEntry(id, shortType, cookTime, experience, result, ingredients(json))));
                }
            } catch (IOException error) {
                throw new IllegalStateException("failed to list " + recipeDir, error);
            }
        }

        parsed.sort(Map.Entry.comparingByKey(Comparator.naturalOrder()));

        Map<String, Map<String, RecipeEntry>> out = new LinkedHashMap<>();
        for (String facility : FACILITIES) {
            out.put(facility, new LinkedHashMap<>());
        }
        for (Map.Entry<String, RecipeEntry> entry : parsed) {
            Map<String, RecipeEntry> bucket = out.get(entry.getValue().type());
            for (String ingredient : entry.getValue().ingredients()) {
                // 每个输入物品保留**第一条**匹配（配方 ID 已排序），与 findFirst 一致。
                bucket.putIfAbsent(ingredient, entry.getValue());
            }
        }
        BY_FACILITY = out;
        return out;
    }

    /**
     * 取出配方的输入物品集合。
     *
     * <p>1.21 起 {@code ingredient} 是字符串或数组（数组表示多选一），且字符串可以是
     * **标签引用**（以 {@code #} 开头，例如玻璃配方用的是 {@code #minecraft:smelts_to_glass}）。
     * 标签必须展开成具体物品，否则会把「配方经标签接受该物品」误判成「没有配方」。
     */
    private static List<String> ingredients(JsonObject json) {
        JsonElement element = json.get("ingredient");
        List<String> out = new ArrayList<>();

        if (element.isJsonPrimitive()) {
            addEntry(element.getAsString(), out);
        } else if (element.isJsonArray()) {
            JsonArray array = element.getAsJsonArray();
            for (JsonElement item : array) {
                if (item.isJsonPrimitive()) {
                    addEntry(item.getAsString(), out);
                } else if (item.isJsonObject() && item.getAsJsonObject().has("items")) {
                    JsonElement items = item.getAsJsonObject().get("items");
                    if (items.isJsonPrimitive()) {
                        addEntry(items.getAsString(), out);
                    } else {
                        for (JsonElement one : items.getAsJsonArray()) {
                            addEntry(one.getAsString(), out);
                        }
                    }
                }
            }
        }
        return out;
    }

    /**
     * 单个食材：具体物品直接加入，{@code #} 开头的标签引用展开成它的全部成员。
     */
    private static void addEntry(String value, List<String> out) {
        if (value.startsWith("#")) {
            out.addAll(tagValues(value.substring(1)));
        } else {
            out.add(value);
        }
    }

    /** 标签缓存：{@code minecraft:smelts_to_glass} -> 其成员物品 ID。 */
    private static final Map<String, List<String>> TAGS = new LinkedHashMap<>();

    /**
     * 展开一个物品标签（读 {@code data/<ns>/tags/item/<path>.json}，递归处理嵌套标签）。
     *
     * <p>只用物品标签：配方 ingredient 解析的是物品，方块标签与它同名但不是同一注册表。
     */
    private static List<String> tagValues(String tagId) {
        List<String> cached = TAGS.get(tagId);
        if (cached != null) {
            return cached;
        }

        List<String> members = new ArrayList<>();
        TAGS.put(tagId, members);

        String root = System.getenv("MC_DIFF_VANILLA_DATAPACK");
        if (root == null || root.isBlank()) {
            return members;
        }

        final char colonChar = ':';
        int colon = tagId.indexOf(colonChar);
        String namespace = colon < 0 ? "minecraft" : tagId.substring(0, colon);
        String path = colon < 0 ? tagId : tagId.substring(colon + 1);
        Path file = Path.of(root, "data", namespace, "tags", "item", path + ".json");
        if (!Files.isRegularFile(file)) {
            return members;
        }

        try {
            JsonObject json = JsonParser.parseString(Files.readString(file, StandardCharsets.UTF_8))
                                   .getAsJsonObject();
            for (JsonElement element : json.getAsJsonArray("values")) {
                String raw = element.isJsonPrimitive()
                    ? element.getAsString()
                    : element.getAsJsonObject().get("id").getAsString();
                addEntry(raw, members);
            }
        } catch (IOException | RuntimeException error) {
            // 标签读取失败时保留已收集的成员，不阻断整体比对。
            return members;
        }
        return members;
    }

    private static String bits(float value) {
        return String.format("0x%08x", Float.floatToRawIntBits(value));
    }
}
