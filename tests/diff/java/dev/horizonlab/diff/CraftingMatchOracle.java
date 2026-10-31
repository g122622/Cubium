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
import java.util.List;
import java.util.TreeSet;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;
import net.minecraft.world.item.Items;
import net.minecraft.world.item.crafting.CraftingInput;
import net.minecraft.world.item.crafting.CraftingRecipe;
import net.minecraft.world.item.crafting.Ingredient;
import net.minecraft.world.item.crafting.Recipe;
import net.minecraft.world.item.crafting.RecipeHolder;
import net.minecraft.world.item.crafting.RecipeType;
import net.minecraft.world.item.crafting.ShapedRecipe;
import net.minecraft.world.item.crafting.ShapelessRecipe;

/**
 * 合成配方**匹配与产出**的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CraftingMatchOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.CraftingMatchOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/crafting_match.cpp 同构：输入是一份「配方 + 固定尺寸的网格」，两侧用
 * 同一份数据各自调 {@code matches} / {@code assemble}。差异因此只可能来自匹配逻辑。
 *
 * <h2>关键：网格必须先过 CraftingInput.ofPositioned</h2>
 *
 * <p>原版玩家真的摆东西时，网格进 {@code matches} 之前会被
 * {@code CraftingInput.ofPositioned} 裁掉四周的空行空列（见 {@code CraftingContainer
 * #asCraftInput}）。{@code ShapedRecipePattern.matches} 随后要求「裁剪后的包围盒尺寸
 * 与配方尺寸**严格相等**」。如果把未裁剪的网格直接喂进去，原版对「2×2 配方摆在 3×3
 * 网格左上角」这类输入会判否——那不是原版玩家的实际结果，而是一个 oracle 自身造出来
 * 的假差异。因此这里与探针**同样**先裁剪，两侧的输入口径才一致。
 */
public final class CraftingMatchOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private CraftingMatchOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            List<String> ids = collectIds();
            StringBuilder text = new StringBuilder();
            for (String id : ids) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            LootServerData data = LootServerData.get();
            run(data, Path.of(args[1]), Path.of(args[2]));
            return;
        }
        if (args.length == 2 && args[0].equals("dump")) {
            dump(Path.of(args[1]));
            return;
        }
        System.err.println(
            "usage: CraftingMatchOracle list <units.txt> | run <input.jsonl> <output.jsonl> | dump <layouts.jsonl>");
        System.exit(2);
    }

    /** 数据包里全部「有序 + 无形状」配方 ID（排序去重），与探针的清单同口径。 */
    private static List<String> collectIds() {
        var recipes = LootServerData.get().resources.getRecipeManager();
        TreeSet<String> ids = new TreeSet<>();
        for (RecipeHolder<?> holder : recipes.getRecipes()) {
            Recipe<?> recipe = holder.value();
            if (isGridRecipe(recipe)) {
                ids.add(holder.id().identifier().toString());
            }
        }
        return new ArrayList<>(ids);
    }

    /** 只认有序与无形状：锻造是三槽输入、熔炼/切石是单物品输入，都不走合成网格。 */
    private static boolean isGridRecipe(Recipe<?> recipe) {
        return recipe.getType() == RecipeType.CRAFTING
            && (recipe instanceof ShapedRecipe || recipe instanceof ShapelessRecipe);
    }

    /**
     * 导出每个「网格配方」的标准摆法：尺寸 + 逐格首选物品。
     *
     * <p>这是单元的**枚举输入**，不是比对的一部分：摆法是配方数据决定的事实，harness
     * 据此展开成各种反例。由 oracle 导出而不是两侧各自推断，是为了让「一格放什么」
     * 只有一个答案——标签原料在两侧的成员顺序可能不同，各取各的会把差异引到标签上。
     */
    private static void dump(Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        var manager = LootServerData.get().resources.getRecipeManager();
        List<RecipeHolder<?>> recipes = new ArrayList<>(manager.getRecipes());
        recipes.sort(java.util.Comparator.comparing(holder -> holder.id().identifier().toString()));

        for (RecipeHolder<?> holder : recipes) {
            Recipe<?> recipe = holder.value();
            if (!isGridRecipe(recipe)) {
                continue;
            }
            JsonObject fixture = new JsonObject();
            fixture.addProperty("recipe", holder.id().identifier().toString());

            // 尺寸：有序合成用图案尺寸；无形状按原版 CraftingGridAccessor 的做法取 3×3
            // （玩家工作台的实际网格），原料依次落在行优先的空位上。
            int width;
            int height;
            List<Ingredient> ingredients;
            if (recipe instanceof ShapedRecipe shaped) {
                width = shaped.getWidth();
                height = shaped.getHeight();
                // pattern.ingredients() 是 List<Optional<Ingredient>>：空位是 empty。
                ingredients = shaped.getIngredients().stream()
                    .map(option -> option.orElse(null))
                    .toList();
            } else {
                width = 3;
                height = 3;
                ingredients = List.copyOf(recipe.placementInfo().ingredients());
            }

            fixture.addProperty("width", width);
            fixture.addProperty("height", height);
            JsonArray cells = new JsonArray();
            for (Ingredient ingredient : ingredients) {
                if (ingredient == null) {
                    cells.add(com.google.gson.JsonNull.INSTANCE);
                } else {
                    JsonObject cell = new JsonObject();
                    cell.addProperty("id", firstOptionId(ingredient));
                    cells.add(cell);
                }
            }
            // 无形状配方的原料个数可能少于网格格数，尾部补空。
            while (cells.size() < width * height) {
                cells.add(com.google.gson.JsonNull.INSTANCE);
            }
            fixture.add("grid", cells);
            text.append(GSON.toJson(fixture)).append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    /** 原料的首选物品：按物品 ID 字典序取第一个，两侧因此对同一原料得到同一答案。 */
    private static String firstOptionId(Ingredient ingredient) {
        return ingredient.items()
            .map(holder -> net.minecraft.core.registries.BuiltInRegistries.ITEM
                .getKey(holder.value()).toString())
            .min(java.util.Comparator.naturalOrder())
            .orElse("");
    }

    private static void run(LootServerData data, Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            String unit = request.get("unit").getAsString();
            text.append(GSON.toJson(evaluate(data, unit, request))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject evaluate(LootServerData data, String unit, JsonObject request) {
        JsonObject result = new JsonObject();
        result.addProperty("unit", unit);

        String recipeId = request.get("recipe").getAsString();
        RecipeHolder<?> holder = findRecipe(data, recipeId);
        if (holder == null) {
            return failure(result, "unknown recipe");
        }
        Recipe<?> recipe = holder.value();
        if (!(recipe instanceof CraftingRecipe crafting)) {
            return failure(result, "not a crafting recipe");
        }

        int width = request.get("width").getAsInt();
        int height = request.get("height").getAsInt();
        JsonArray grid = request.getAsJsonArray("grid");
        if (grid.size() != width * height) {
            return failure(result, "grid size mismatch");
        }

        // 按行优先构造物品列表；物品不认识时与探针同样判为场景不可构造。
        List<ItemStack> items = new ArrayList<>(grid.size());
        for (JsonElement element : grid) {
            if (element.isJsonNull()) {
                items.add(ItemStack.EMPTY);
                continue;
            }
            JsonObject cell = element.getAsJsonObject();
            String itemId = cell.get("id").getAsString();
            Item item = data.registries.compositeAccess().lookupOrThrow(Registries.ITEM)
                .get(ResourceKey.create(Registries.ITEM, Identifier.parse(itemId)))
                .map(entry -> entry.value())
                .orElse(null);
            if (item == null || item == Items.AIR) {
                return failure(result, "unknown item: " + itemId);
            }
            int count = cell.has("count") ? cell.get("count").getAsInt() : 1;
            items.add(new ItemStack(item, count));
        }

        // 与玩家实际路径一致：先裁剪空行空列，再交给 matches。
        CraftingInput input = CraftingInput.ofPositioned(width, height, items).input();
        var level = LootLevel.get().level;
        boolean matches = crafting.matches(input, level);

        result.addProperty("ok", true);
        result.addProperty("matches", matches);
        if (matches) {
            result.add("result", summarise(crafting.assemble(input, data.resources.fullRegistries().lookup())));
        } else {
            result.add("result", com.google.gson.JsonNull.INSTANCE);
        }
        return result;
    }

    /**
     * 按 ID 取配方。遍历建表而不是 `byKey`：与 RecipeOracle 的做法一致，
     * 也免得依赖 `ResourceKey<Recipe<?>>` 的泛型形态。
     */
    private static RecipeHolder<?> findRecipe(LootServerData data, String id) {
        Identifier identifier = Identifier.parse(id);
        for (RecipeHolder<?> holder : data.resources.getRecipeManager().getRecipes()) {
            if (holder.id().identifier().equals(identifier)) {
                return holder;
            }
        }
        return null;
    }

    private static JsonObject failure(JsonObject result, String reason) {
        result.addProperty("ok", false);
        result.addProperty("error", reason);
        return result;
    }

    /** 与探针 {@code summariseStack} 同口径：只给 id 与 count，空堆为 null。 */
    private static JsonElement summarise(ItemStack stack) {
        if (stack.isEmpty()) {
            return com.google.gson.JsonNull.INSTANCE;
        }
        JsonObject entry = new JsonObject();
        entry.addProperty("id", stack.getItemHolder().unwrapKey().orElseThrow().identifier().toString());
        entry.addProperty("count", stack.getCount());
        return entry;
    }
}