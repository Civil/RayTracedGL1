// This file was generated by GenerateShaderCommon.py

#pragma once

namespace RTGL1
{

#include <stdint.h>

#define MAX_STATIC_VERTEX_COUNT (1048576)
#define MAX_DYNAMIC_VERTEX_COUNT (2097152)
#define MAX_INDEXED_PRIMITIVE_COUNT (1048576)
#define MAX_BOTTOM_LEVEL_GEOMETRIES_COUNT (8192)
#define MAX_BOTTOM_LEVEL_GEOMETRIES_COUNT_POW (13)
#define MAX_GEOMETRY_PRIMITIVE_COUNT (524288)
#define MAX_GEOMETRY_PRIMITIVE_COUNT_POW (19)
#define MAX_TOP_LEVEL_INSTANCE_COUNT (36)
#define BINDING_VERTEX_BUFFER_STATIC (0)
#define BINDING_VERTEX_BUFFER_DYNAMIC (1)
#define BINDING_INDEX_BUFFER_STATIC (2)
#define BINDING_INDEX_BUFFER_DYNAMIC (3)
#define BINDING_GEOMETRY_INSTANCES (4)
#define BINDING_GEOMETRY_INSTANCES_MATCH_PREV (5)
#define BINDING_PREV_POSITIONS_BUFFER_DYNAMIC (6)
#define BINDING_PREV_INDEX_BUFFER_DYNAMIC (7)
#define BINDING_GLOBAL_UNIFORM (0)
#define BINDING_ACCELERATION_STRUCTURE_MAIN (0)
#define BINDING_ACCELERATION_STRUCTURE_SKYBOX (1)
#define BINDING_TEXTURES (0)
#define BINDING_CUBEMAPS (0)
#define BINDING_RENDER_CUBEMAP (0)
#define BINDING_BLUE_NOISE (0)
#define BINDING_LUM_HISTOGRAM (0)
#define BINDING_LIGHT_SOURCES_SPHERICAL (0)
#define BINDING_LIGHT_SOURCES_DIRECTIONAL (1)
#define BINDING_LIGHT_SOURCES_SPH_MATCH_PREV (2)
#define BINDING_LIGHT_SOURCES_DIR_MATCH_PREV (3)
#define INSTANCE_CUSTOM_INDEX_FLAG_DYNAMIC (1 << 0)
#define INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON (1 << 1)
#define INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON_VIEWER (1 << 2)
#define INSTANCE_CUSTOM_INDEX_FLAG_SKYBOX (1 << 3)
#define INSTANCE_CUSTOM_INDEX_FLAG_REFLECT (1 << 4)
#define INSTANCE_MASK_ALL (0xFF)
#define INSTANCE_MASK_WORLD (1 << 0)
#define INSTANCE_MASK_FIRST_PERSON (1 << 1)
#define INSTANCE_MASK_FIRST_PERSON_VIEWER (1 << 2)
#define INSTANCE_MASK_SKYBOX (1 << 3)
#define INSTANCE_MASK_BLENDED (1 << 4)
#define INSTANCE_MASK_EMPTY_5 (1 << 5)
#define INSTANCE_MASK_EMPTY_6 (1 << 6)
#define INSTANCE_MASK_EMPTY_7 (1 << 7)
#define PAYLOAD_INDEX_DEFAULT (0)
#define PAYLOAD_INDEX_SHADOW (1)
#define SBT_INDEX_RAYGEN_PRIMARY (0)
#define SBT_INDEX_RAYGEN_DIRECT (1)
#define SBT_INDEX_RAYGEN_INDIRECT (2)
#define SBT_INDEX_MISS_DEFAULT (0)
#define SBT_INDEX_MISS_SHADOW (1)
#define SBT_INDEX_HITGROUP_FULLY_OPAQUE (0)
#define SBT_INDEX_HITGROUP_ALPHA_TESTED (1)
#define MATERIAL_ALBEDO_ALPHA_INDEX (0)
#define MATERIAL_NORMAL_METALLIC_INDEX (1)
#define MATERIAL_EMISSION_ROUGHNESS_INDEX (2)
#define MATERIAL_NO_TEXTURE (0)
#define MATERIAL_BLENDING_FLAG_OPAQUE (1 << 0)
#define MATERIAL_BLENDING_FLAG_ALPHA (1 << 1)
#define MATERIAL_BLENDING_FLAG_ADD (1 << 2)
#define MATERIAL_BLENDING_FLAG_SHADE (1 << 3)
#define MATERIAL_BLENDING_FLAG_BIT_COUNT (4)
#define MATERIAL_BLENDING_MASK_FIRST_LAYER (15)
#define MATERIAL_BLENDING_MASK_SECOND_LAYER (240)
#define MATERIAL_BLENDING_MASK_THIRD_LAYER (3840)
#define GEOM_INST_FLAG_GENERATE_NORMALS (1 << 29)
#define GEOM_INST_FLAG_INVERTED_NORMALS (1 << 30)
#define GEOM_INST_FLAG_IS_MOVABLE (1 << 31)
#define SKY_TYPE_COLOR (0)
#define SKY_TYPE_CUBEMAP (1)
#define SKY_TYPE_RASTERIZED_GEOMETRY (2)
#define SKY_TYPE_RAY_TRACED_GEOMETRY (3)
#define BLUE_NOISE_TEXTURE_COUNT (128)
#define BLUE_NOISE_TEXTURE_SIZE (128)
#define BLUE_NOISE_TEXTURE_SIZE_POW (7)
#define COMPUTE_COMPOSE_GROUP_SIZE_X (16)
#define COMPUTE_COMPOSE_GROUP_SIZE_Y (16)
#define COMPUTE_LUM_HISTOGRAM_GROUP_SIZE_X (16)
#define COMPUTE_LUM_HISTOGRAM_GROUP_SIZE_Y (16)
#define COMPUTE_LUM_HISTOGRAM_BIN_COUNT (256)
#define COMPUTE_VERT_PREPROC_GROUP_SIZE_X (256)
#define VERT_PREPROC_MODE_ONLY_DYNAMIC (0)
#define VERT_PREPROC_MODE_DYNAMIC_AND_MOVABLE (1)
#define VERT_PREPROC_MODE_ALL (2)
#define COMPUTE_GRADIENT_SAMPLES_GROUP_SIZE_X (16)
#define COMPUTE_GRADIENT_MERGING_GROUP_SIZE_X (16)
#define COMPUTE_GRADIENT_ATROUS_GROUP_SIZE_X (16)
#define COMPUTE_SVGF_TEMPORAL_GROUP_SIZE_X (16)
#define COMPUTE_SVGF_VARIANCE_GROUP_SIZE_X (16)
#define COMPUTE_SVGF_ATROUS_GROUP_SIZE_X (16)
#define COMPUTE_SVGF_ATROUS_ITERATION_COUNT (4)
#define COMPUTE_ASVGF_STRATA_SIZE (3)
#define COMPUTE_ASVGF_GRADIENT_ATROUS_ITERATION_COUNT (4)

struct ShVertexBufferStatic
{
    float positions[3145728];
    float normals[3145728];
    float tangents[3145728];
    float texCoords[2097152];
    float texCoordsLayer1[2097152];
    float texCoordsLayer2[2097152];
};

struct ShVertexBufferDynamic
{
    float positions[6291456];
    float normals[6291456];
    float tangents[3145728];
    float texCoords[4194304];
};

struct ShGlobalUniform
{
    float view[16];
    float invView[16];
    float viewPrev[16];
    float projection[16];
    float invProjection[16];
    float projectionPrev[16];
    uint32_t positionsStride;
    uint32_t normalsStride;
    uint32_t texCoordsStride;
    float renderWidth;
    float renderHeight;
    uint32_t frameId;
    float timeDelta;
    float minLogLuminance;
    float maxLogLuminance;
    float luminanceWhitePoint;
    uint32_t stopEyeAdaptation;
    uint32_t lightCountSpherical;
    uint32_t lightCountDirectional;
    uint32_t skyType;
    float skyColorMultiplier;
    uint32_t skyCubemapIndex;
    float skyColorDefault[4];
    float skyViewerPosition[4];
    float cameraPosition[4];
    uint32_t dbgShowMotionVectors;
    uint32_t dbgShowGradients;
    uint32_t lightCountSphericalPrev;
    uint32_t lightCountDirectionalPrev;
    int32_t instanceGeomInfoOffset[72];
    int32_t instanceGeomInfoOffsetPrev[72];
    int32_t instanceGeomCount[72];
    float viewProjCubemap[264];
};

struct ShGeometryInstance
{
    float model[16];
    float prevModel[16];
    uint32_t materials[3][4];
    float materialColors[3][4];
    uint32_t flags;
    uint32_t baseVertexIndex;
    uint32_t baseIndexIndex;
    uint32_t prevBaseVertexIndex;
    uint32_t prevBaseIndexIndex;
    uint32_t vertexCount;
    uint32_t indexCount;
    float defaultRoughness;
    float defaultMetallicity;
    float defaultEmission;
    uint32_t __pad0;
    uint32_t __pad1;
};

struct ShTonemapping
{
    uint32_t histogram[256];
    float avgLuminance;
};

struct ShLightSpherical
{
    float position[3];
    float radius;
    float color[3];
    float falloff;
};

struct ShLightDirectional
{
    float direction[3];
    float tanAngularRadius;
    float color[3];
    uint32_t __pad0;
};

struct ShVertPreprocessing
{
    uint32_t tlasInstanceCount;
    uint32_t skyboxTlasInstanceCount;
    uint32_t tlasInstanceIsDynamicBits[2];
    uint32_t skyboxTlasInstanceIsDynamicBits[2];
};

}