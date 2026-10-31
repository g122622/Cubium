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
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.world.item.crafting.Recipe;
import net.minecraft.world.item.crafting.RecipeHolder;

/**
 * 配方书页签归属的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.RecipeBookCategoryOracle list &lt;units.txt&gt;
 *   java dev.horizonlab.diff.RecipeBookCategoryOracle run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>对每条配方输出两样东西：
 *
 * <ul>
 *   <li>{@code type}：配方序列化器 ID（{@code minecraft:crafting_shaped} 等），
 *       用于确认两侧对「这是什么配方」的判断一致；</li>
 *   <li>{@code page}：{@code Recipe#recipeBookCategory()} 对应的注册表键，例如
 *       {@code minecraft:crafting_equipment}、{@code minecraft:smoker_food}。</li>
 * </ul>
 *
 * <p>{@code page} 是原版<b>自己的</b>映射结果——它已经把「配方类型」与「JSON 里的
 * category 字段」两个维度压成一个值。因此只比这一个字段就能同时发现「分类解析错了」
 * 与「类型映射错了」，不必两侧各自复述映射规则。
 */
public final class RecipeBookCategoryOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private RecipeBookCategoryOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String id : recipeIds()) {
                text.append(id).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: RecipeBookCategoryOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static List<RecipeHolder<?>> sortedRecipes() {
        List<RecipeHolder<?>> recipes = new ArrayList<>(
            LootServerData.get().resources.getRecipeManager().getRecipes());
        recipes.sort(Comparator.comparing(holder -> holder.id().identifier().toString()));
        return recipes;
    }

    private static List<String> recipeIds() {
        List<String> ids = new ArrayList<>();
        for (RecipeHolder<?> holder : sortedRecipes()) {
            ids.add(holder.id().identifier().toString());
        }
        return ids;
    }

    private static void run(Path input, Path output) throws IOException {
        // 先建一张 ID → 配方 的表：逐行线性扫描会把 1470 条配方扫 1470 遍。
        java.util.Map<String, RecipeHolder<?>> byId = new java.util.HashMap<>();
        for (RecipeHolder<?> holder : sortedRecipes()) {
            byId.put(holder.id().identifier().toString(), holder);
        }

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject request = JsonParser.parseString(line).getAsJsonObject();
            text.append(GSON.toJson(summarise(request.get("recipe").getAsString(), byId.get(request.get("recipe").getAsString()))))
                .append('\n');
        }
        Files.writeString(output, text.toString(), StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(String id, RecipeHolder<?> holder) {
        JsonObject result = new JsonObject();
        result.addProperty("recipe", id);
        if (holder == null) {
            result.addProperty("ok", false);
            result.addProperty("error", "unknown recipe");
            return result;
        }

        Recipe<?> recipe = holder.value();
        result.addProperty("ok", true);
        // 类型用序列化器 ID：Cubium 的 RecipeType 字符串与之一一对应
        // （`minecraft:crafting_shaped` 等），两侧因此可以直接比字符串。
        result.addProperty("type", BuiltInRegistries.RECIPE_SERIALIZER.getKey(recipe.getSerializer()).toString());
        // page：原版把类型 + category 压成注册表键的结果。
        result.addProperty("page", BuiltInRegistries.RECIPE_BOOK_CATEGORY.getKey(recipe.recipeBookCategory()).toString());
        return result;
    }
}