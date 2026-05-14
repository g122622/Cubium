#pragma once

// 生物群系系统聚合头文件

#include "Biome.hpp"
#include "BiomeProvider.hpp"
#include "BiomeRegistry.hpp"

// Layer 系统
#include "layer/BiomeValues.hpp"
#include "layer/Layer.hpp"
#include "layer/LayerContext.hpp"
#include "layer/LayerUtil.hpp"
#include "layer/transformers/BiomeLayers.hpp"
#include "layer/transformers/ClimateLayers.hpp"
#include "layer/transformers/EdgeLayers.hpp"
#include "layer/transformers/MergeLayers.hpp"
#include "layer/transformers/SourceLayers.hpp"
#include "layer/transformers/TransformerTraits.hpp"
#include "layer/transformers/ZoomLayers.hpp"
