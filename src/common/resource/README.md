# Resource Module

This module provides a comprehensive resource pack loading system with MC 1.12-1.19+ compatibility support.

## Directory Structure

```
resource/
├── ResourceLocation.hpp/cpp       # Resource identifier (namespace:path format)
├── IResourcePack.hpp/cpp          # Resource pack interface
├── FolderResourcePack.hpp/cpp     # Folder-based resource pack implementation
├── ZipResourcePack.hpp/cpp        # ZIP-based resource pack implementation
├── InMemoryResourcePack.hpp/cpp   # In-memory resource pack for built-in resources
├── PackMetadata.hpp/cpp           # pack.mcmeta parsing
├── ResourcePackList.hpp/cpp       # Multi-pack management with priority system
├── VanillaResources.hpp/cpp       # Built-in vanilla models and blockstates
├── compat/                        # MC version compatibility layer
│   ├── PackFormat.hpp/cpp         # Pack format version enumeration
│   ├── ResourceMapper.hpp/cpp     # Abstract resource path mapper interface
│   ├── TextureMapper.hpp/cpp      # 250+ texture name bidirectional mappings
│   ├── unified/                   # Unified resource representations
│   │   ├── UnifiedResource.hpp    # Base unified resource types
│   │   ├── UnifiedModel.hpp       # Unified model representation
│   │   └── UnifiedBlockState.hpp  # Unified blockstate representation
│   ├── v1_12/                     # MC 1.11-1.12.2 resource mapping
│   │   └── ResourceMapperV112.hpp/cpp
│   └── v1_13/                     # MC 1.13+ resource mapping
│       └── ResourceMapperV113.hpp/cpp
└── loader/                        # Resource loading pipeline
    └── ResourceLoader.hpp/cpp     # Unified resource loading with format detection
```

## File Descriptions

### Core Files

#### ResourceLocation.hpp/cpp
Resource location identifier following Minecraft's standard format (`namespace:path`).
- Parses and stores namespace and path components
- Converts to file paths (`assets/namespace/path`)
- Supports comparison and hashing for use in containers

#### IResourcePack.hpp/cpp
Abstract interface for resource pack implementations.
- `initialize()` - Initialize the resource pack
- `hasResource()` - Check if a resource exists
- `readResource()` - Read binary resource data
- `readTextResource()` - Read text resource (helper method)
- `listResources()` - List resources in a directory

#### FolderResourcePack.hpp/cpp
Folder-based resource pack implementation.
- Loads resources from a filesystem directory
- Reads `pack.mcmeta` for metadata
- Supports recursive directory listing

#### ZipResourcePack.hpp/cpp
ZIP-based resource pack implementation using libarchive.
- Loads resources from ZIP files
- Builds file index on initialization for fast lookup
- Caches read resources in memory

#### InMemoryResourcePack.hpp/cpp
In-memory resource pack for built-in resources.
- Used for vanilla resources that are embedded in the binary
- Highest priority, always loaded
- Provides methods to add resources programmatically

#### PackMetadata.hpp/cpp
Parses `pack.mcmeta` files.
- Extracts `pack_format` version
- Extracts description text
- Validates version compatibility

#### ResourcePackList.hpp/cpp
Manages multiple resource packs with priority system.
- **Priority order**: Higher priority packs override lower priority packs
- Scan directory for resource packs
- Enable/disable packs
- Move packs up/down in priority
- Synchronize with settings
- Resource lookup across all enabled packs

#### VanillaResources.hpp/cpp
Provides built-in vanilla models and blockstates.
- Base models: `cube`, `cube_all`, `cube_column`, `leaves`, `cross`, `tinted_cross`, `air`
- Block-specific models: stone, dirt, grass_block, ores, planks, logs, leaves, wool, etc.
- Blockstates for all registered blocks
- Used as fallback when external resource packs don't provide a resource

### Compat Layer

#### PackFormat.hpp/cpp
Enumerates Minecraft resource pack format versions.
| Format | MC Version |
|--------|------------|
| 1 | 1.6.1 - 1.8.9 |
| 2 | 1.9 - 1.10.2 |
| 3 | 1.11 - 1.12.2 |
| 4 | 1.13 - 1.14.4 |
| 5 | 1.15 - 1.16.1 |
| 6 | 1.16.2 - 1.16.5 |
| 7 | 1.17.x |
| 8 | 1.18.x |
| 9 | 1.19.x |

Functions:
- `detectPackFormat()` - Detect format from pack_format value
- `usesOldTexturePaths()` - Check if format uses `textures/blocks/`
- `usesNewTexturePaths()` - Check if format uses `textures/block/`
- `requiresTextureNameMapping()` - Check if format needs name mapping

#### ResourceMapper.hpp/cpp
Abstract interface for resource path conversion between MC versions.
- `toUnifiedTexturePath()` - Convert texture path to modern format
- `getTexturePathVariants()` - Get all possible path variants
- `toModernTextureName()` - Convert texture name to modern format
- `toLegacyTextureName()` - Convert texture name to legacy format
- `toUnifiedModelPath()` - Convert model path to unified format
- `toUnifiedBlockStatePath()` - Convert blockstate path
- `create()` - Factory method to create appropriate mapper

#### TextureMapper.hpp/cpp
Bidirectional texture name mapping database with 250+ entries.
- Maps legacy names to modern names (e.g., `log_jungle` <-> `jungle_log`)
- Maps legacy paths to modern paths (`textures/blocks/` <-> `textures/block/`)
- Thread-safe singleton pattern

Example mappings:
| Legacy (1.12) | Modern (1.13+) |
|---------------|----------------|
| log_jungle | jungle_log |
| wool_colored_white | white_wool |
| stone_granite | granite |
| flower_rose | poppy |
| flower_houstonia | azure_bluet |
| hardened_clay_stained_white | white_terracotta |
| tallgrass | short_grass |

#### unified/UnifiedResource.hpp
Base types for unified resource representation.
- `ResourceType` enum: Texture, Model, BlockState, Sound, Language, Data
- `UnifiedResource` struct: Base class with location, original path, format
- `PixelData` struct: RGBA pixel data with width/height
- `UnifiedTexture` struct: Texture with pixel data

#### unified/UnifiedModel.hpp
Unified model representation.
- `Direction` enum: Down, Up, North, South, West, East
- `ModelFaceUV` struct: UV coordinates (u1, v1, u2, v2)
- `ModelFace` struct: Face with texture reference, UV, rotation, tint
- `ModelElement` struct: Cube with from/to coordinates and faces
- `UnifiedModel` struct: Complete model with parent, textures, elements
- Supports texture reference resolution (`#all` -> actual texture path)

#### unified/UnifiedBlockState.hpp
Unified blockstate representation.
- `ModelVariant` struct: Model reference with rotation and UV lock
- `VariantList` struct: Weighted list of variants for random selection
- `MultipartCondition` struct: Condition for multipart models
- `MultipartSelector` struct: Condition + variants combination
- `UnifiedBlockState` struct: Complete blockstate with variants/multipart

#### v1_12/ResourceMapperV112.hpp/cpp
Resource mapper for MC 1.11-1.12.2.
- Converts `textures/blocks/` to `textures/block/`
- Converts `textures/items/` to `textures/item/`
- Applies TextureMapper for name conversion
- Provides path variants for fallback lookup

#### v1_13/ResourceMapperV113.hpp/cpp
Resource mapper for MC 1.13+.
- Modern paths are already unified
- Provides legacy path fallbacks for compatibility
- Minimal transformation needed

### Loader

#### ResourceLoader.hpp/cpp
Resource loading pipeline with format detection.
- `addResourcePack()` - Add pack with auto-detection
- `loadTextures()` - Load all textures from all packs
- `loadTexture()` - Load single texture by location
- `loadModel()` - Load single model (TODO)
- `loadBlockState()` - Load single blockstate (TODO)
- `detectFormat()` - Auto-detect pack format from metadata

## Module Overview

### Responsibility
This module provides:
1. **Resource Pack Loading**: Support for ZIP and folder-based resource packs
2. **Version Compatibility**: Transparent conversion between MC 1.12 and 1.13+ resource formats
3. **Priority System**: Later packs override earlier packs
4. **Unified Representation**: Version-agnostic internal resource format

### Inputs
- ZIP resource packs (`.zip` files with `pack.mcmeta`)
- Folder resource packs (directories with `pack.mcmeta`)
- Resource paths (e.g., `minecraft:textures/block/stone.png`)
- Built-in vanilla resources (hardcoded models and blockstates)

### Outputs
- Raw resource data (binary)
- Unified textures (RGBA pixel data)
- Unified models (parsed JSON)
- Unified blockstates (parsed JSON)

### Dependencies
- **nlohmann-json**: JSON parsing for models and blockstates
- **libarchive**: ZIP file reading
- **stb_image**: Image loading (PNG, JPEG, etc.)
- **spdlog**: Logging

### Usage

```cpp
#include "resource/ResourcePackList.hpp"
#include "resource/compat/TextureMapper.hpp"
#include "resource/loader/ResourceLoader.hpp"

using namespace mc;
using namespace mc::resource;

// 1. Create resource pack list
ResourcePackList packList;

// 2. Add built-in vanilla resources (highest priority)
auto vanillaPack = VanillaResources::createResourcePack();
packList.addPack(std::filesystem::path("vanilla"), true, 1000);

// 3. Scan resource pack directory
packList.scanDirectory("resourcepacks");

// 4. Load a resource
auto result = packList.readResource("assets/minecraft/textures/block/stone.png");
if (result.success()) {
    const auto& data = result.value();
    // Use data...
}

// 5. Check texture name mapping
const auto& mapper = compat::TextureMapper::instance();
String legacy = mapper.getLegacyName("jungle_log");  // -> "log_jungle"
String modern = mapper.getModernName("log_jungle");  // -> "jungle_log"

// 6. Use ResourceLoader for unified loading
loader::ResourceLoader loader;
loader.addResourcePack(pack);
auto textures = loader.loadTextures();
```

## Common Pitfalls

### 1. Path Separator Issues
- Always use forward slashes (`/`) in resource paths
- ResourceLocation automatically normalizes paths
- Windows backslashes are converted internally

### 2. Pack Format Detection
- Packs without `pack.mcmeta` default to format 4 (1.13-1.14)
- Always check format before assuming version-specific behavior
- Use `detectPackFormat()` for reliable detection

### 3. Texture Name Mapping
- Not all textures have mappings; check with `hasMapping()`
- Some mappings are not 1:1 (e.g., `rose` -> `poppy`)
- Use `getNameVariants()` for fallback chains

### 4. Priority Order
- Higher priority = later in the list
- When using `ResourcePackList::moveUp()`, priority increases
- Vanilla resources should have highest priority to ensure fallbacks work

### 5. Memory Management
- `ZipResourcePack` caches resources; call `clearCache()` for memory-constrained environments
- `InMemoryResourcePack` holds all resources in memory permanently
- `ResourcePackList` holds shared pointers to packs

### 6. TextureMapper Singleton
- First access initializes all 250+ mappings
- Thread-safe initialization via mutex
- Access via `TextureMapper::instance()`

## Test Coverage

Test file: `tests/common/resource/compat/CompatLayerTest.cpp`

### PackFormatTest
- `DetectFormat_ValidValues` - Tests all valid format values 1-9
- `DetectFormat_UnknownValue` - Tests invalid format values
- `UsesOldTexturePaths` - Verifies old path detection
- `UsesNewTexturePaths` - Verifies new path detection
- `RequiresTextureNameMapping` - Verifies name mapping requirement
- `PackFormatToString` - Tests string conversion

### TextureMapperTest
- `LogTextures` - Log texture bidirectional mapping
- `LeafTextures` - Leaf texture mapping
- `WoolTextures` - Wool color mapping (including `silver` -> `light_gray`)
- `StoneVariants` - Granite, diorite, andesite mapping
- `GrassBlock` - Grass block texture mapping
- `FlowerTextures` - Flower mapping (including `rose` -> `poppy`)
- `ConcreteTextures` - Concrete color mapping
- `TerracottaTextures` - Terracotta mapping
- `SandstoneTextures` - Sandstone variant mapping
- `TallGrassTextures` - Tall grass mapping
- `HasMapping` - Mapping existence check
- `GetNameVariants` - Variant generation
- `PathTransformation` - Path conversion
- `GetPathVariants` - Path variant generation

### ResourceMapperFactoryTest
- `CreateV112Mapper` - Factory creates 1.12 mapper
- `CreateV113Mapper` - Factory creates 1.13 mapper
- `CreateV116Mapper` - Factory creates correct mapper for 1.16
- `CreateUnknownMapper` - Factory defaults to modern mapper

### ResourceMapperV112Test
- `ToUnifiedTexturePath` - Path conversion
- `GetTexturePathVariants` - Variant generation for fallback

## Architecture Notes

### Version Compatibility Strategy
The module uses a "compiler frontend" pattern:
1. **Input**: Version-specific resource path
2. **Conversion**: Apply appropriate `ResourceMapper`
3. **Output**: Unified (modern MC 1.13+) format

This allows the rest of the codebase to work with a single, consistent resource format.

### Resource Pack Priority
Resource packs are loaded in priority order (highest first):
1. Vanilla (built-in) - lowest priority, fallback
2. External packs - loaded in order
3. User-added packs - highest priority

When looking up a resource:
1. Check highest priority pack first
2. Fall back to lower priority packs
3. Finally check vanilla resources

### Memory Layout
- `ResourcePackList` owns `shared_ptr<IResourcePack>`
- `ResourceLoader` holds `PackContext` with pack + mapper
- `UnifiedResource` types are value types (can be copied)
