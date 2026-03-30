#!/bin/bash

# 目录结构:
# src/common/entity/entities/passive/basic/ -> 需要 ../../../../item/ 和 ../../attribute/
# src/common/entity/entities/passive/tamable/ -> 同上
# src/common/entity/entities/passive/special/ -> 同上
# src/common/entity/entities/passive/water/ -> 同上
# src/common/entity/entities/passive/fish/ -> 同上
# src/common/entity/entities/passive/ambient/ -> 同上
# src/common/entity/entities/passive/golem/ -> 同上
# src/common/entity/entities/monster/ -> 需要 ../../../item/ 和 ../../../attribute/
# src/common/entity/entities/monster/X/ -> 需要 ../../../../item/ 和 ../../attribute/

# Passive entities (depth 5: common/entity/entities/passive/X/)
PASSIVE_DIRS=(
    "passive/basic"
    "passive/tamable"
    "passive/special"
    "passive/water"
    "passive/fish"
    "passive/ambient"
    "passive/golem"
)

# Monster entities at depth 4 (common/entity/entities/monster/)
# Monster entities at depth 5 (common/entity/entities/monster/X/)

BASE_DIR="D:/MiscProjects/minecraft-reborn/src/common/entity/entities"

# Fix passive entities
for dir in "${PASSIVE_DIRS[@]}"; do
    for f in "$BASE_DIR/$dir"/*.cpp; do
        if [ -f "$f" ]; then
            echo "Processing: $f"
            # Fix core/Types.hpp - needs to go up to common/ level
            sed -i 's|"\.\./\.\./\.\./\.\./\.\./\.\./core/Types\.hpp"|"../../../../core/Types.hpp"|g' "$f"
            sed -i 's|"\.\./\.\./\.\./\.\./core/Types\.hpp"|"../../../../core/Types.hpp"|g' "$f"
            sed -i 's|"\.\./\.\./\.\./core/Types\.hpp"|"../../../../core/Types.hpp"|g' "$f"

            # Fix item/ItemStack.hpp
            sed -i 's|"\.\./\.\./\.\./\.\./\.\./\.\./item/|"../../../../item/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./\.\./item/|"../../../../item/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./item/|"../../../../item/|g' "$f"

            # Fix world/World.hpp
            sed -i 's|"\.\./\.\./\.\./\.\./\.\./\.\./world/|"../../../../world/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./\.\./world/|"../../../../world/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./world/|"../../../../world/|g' "$f"

            # Fix attribute/Attributes.hpp
            sed -i 's|"\.\./\.\./\.\./\.\./\.\./entity/attribute/|"../../attribute/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./\.\./entity/attribute/|"../../attribute/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./entity/attribute/|"../../attribute/|g' "$f"
            sed -i 's|"\.\./\.\./attribute/|"../../attribute/|g' "$f"

            # Fix core/EntityRegistry.hpp
            sed -i 's|"\.\./\.\./\.\./\.\./\.\./entity/core/|"../../core/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./\.\./entity/core/|"../../core/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./entity/core/|"../../core/|g' "$f"
            sed -i 's|"\.\./\.\./core/|"../../core/|g' "$f"

            # Fix ai/goal/GoalSelector.hpp
            sed -i 's|"\.\./\.\./\.\./\.\./ai/|"../../ai/|g' "$f"
            sed -i 's|"\.\./\.\./\.\./ai/|"../../ai/|g' "$f"
        fi
    done
done

echo "Done fixing passive entities"
