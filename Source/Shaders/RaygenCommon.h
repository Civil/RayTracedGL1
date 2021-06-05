// Copyright (c) 2021 Sultim Tsyrendashiev
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#extension GL_EXT_ray_tracing : require


#include "ShaderCommonGLSLFunc.h"


#if !defined(DESC_SET_TLAS) || \
    !defined(DESC_SET_GLOBAL_UNIFORM) || \
    !defined(DESC_SET_VERTEX_DATA) || \
    !defined(DESC_SET_TEXTURES) || \
    !defined(DESC_SET_RANDOM) || \
    !defined(DESC_SET_LIGHT_SOURCES)
        #error Descriptor set indices must be set!
#endif


layout(set = DESC_SET_TLAS, binding = BINDING_ACCELERATION_STRUCTURE_MAIN)   uniform accelerationStructureEXT topLevelAS;
layout(set = DESC_SET_TLAS, binding = BINDING_ACCELERATION_STRUCTURE_SKYBOX) uniform accelerationStructureEXT skyboxTopLevelAS;

#ifdef DESC_SET_CUBEMAPS
layout(set = DESC_SET_CUBEMAPS, binding = BINDING_CUBEMAPS) uniform samplerCube globalCubemaps[];
#endif
#ifdef DESC_SET_RENDER_CUBEMAP
layout(set = DESC_SET_RENDER_CUBEMAP, binding = BINDING_RENDER_CUBEMAP) uniform samplerCube renderCubemap;
#endif


layout(location = PAYLOAD_INDEX_DEFAULT) rayPayloadEXT ShPayload payload;

#ifdef RAYGEN_SHADOW_PAYLOAD
layout(location = PAYLOAD_INDEX_SHADOW) rayPayloadEXT ShPayloadShadow payloadShadow;
#endif // RAYGEN_SHADOW_PAYLOAD



uint getPrimaryVisibilityCullMask()
{
    return INSTANCE_MASK_ALL & (~INSTANCE_MASK_FIRST_PERSON_VIEWER);
}

uint getShadowCullMask(uint surfInstCustomIndex)
{
    if ((surfInstCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON) != 0)
    {
        // no first-person viewer shadows -- on first-person
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON;
    }
    else if ((surfInstCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON_VIEWER) != 0)
    {
        // no first-person shadows -- on first-person viewer
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON_VIEWER;
    }
    else
    {
        // no first-person shadows -- on world
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON_VIEWER;
    }
    
    // blended geometry doesn't have shadows
}

uint getIndirectIlluminationCullMask(uint surfInstCustomIndex)
{
    if ((surfInstCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON) != 0)
    {
        // no first-person viewer indirect illumination -- on first-person
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON;
    }
    else if ((surfInstCustomIndex & INSTANCE_CUSTOM_INDEX_FLAG_FIRST_PERSON_VIEWER) != 0)
    {
        // no first-person indirect illumination -- on first-person viewer
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON_VIEWER;
    }
    else
    {
        // no first-person indirect illumination -- on first-person viewer
        return INSTANCE_MASK_WORLD | INSTANCE_MASK_FIRST_PERSON_VIEWER;
    }
    
    // blended geometry doesn't have indirect illumination
}



bool isPayloadConsistent(const ShPayload p)
{
    return p.instIdAndIndex != UINT32_MAX && p.geomAndPrimIndex != UINT32_MAX;
}

void resetPayload()
{
    payload.baryCoords = vec2(0.0);
    payload.instIdAndIndex = UINT32_MAX;
    payload.geomAndPrimIndex = UINT32_MAX;
}

ShPayload tracePrimaryRay(vec3 origin, vec3 direction)
{
    resetPayload();

    uint cullMask = getPrimaryVisibilityCullMask();

    traceRayEXT(
        topLevelAS,
        gl_RayFlagsNoneEXT, 
        cullMask, 
        0, 0,     // sbtRecordOffset, sbtRecordStride
        SBT_INDEX_MISS_DEFAULT, 
        origin, 0.001, direction, MAX_RAY_LENGTH, 
        PAYLOAD_INDEX_DEFAULT);

    return payload; 
}

ShPayload traceIndirectRay(uint surfInstCustomIndex, vec3 surfPosition, vec3 bounceDirection)
{
    resetPayload();

    uint cullMask = getIndirectIlluminationCullMask(surfInstCustomIndex);

    traceRayEXT(
        topLevelAS,
        gl_RayFlagsNoneEXT, 
        cullMask, 
        0, 0,     // sbtRecordOffset, sbtRecordStride
        SBT_INDEX_MISS_DEFAULT, 
        surfPosition, 0.001, bounceDirection, MAX_RAY_LENGTH, 
        PAYLOAD_INDEX_DEFAULT); 

    return payload;
}



#ifdef DESC_SET_CUBEMAPS
ShPayload traceSkyRay(vec3 origin, vec3 direction)
{
    resetPayload();

    uint cullMask = INSTANCE_MASK_SKYBOX;

    traceRayEXT(
        skyboxTopLevelAS,
        gl_RayFlagsNoneEXT, 
        cullMask, 
        0, 0,     // sbtRecordOffset, sbtRecordStride
        SBT_INDEX_MISS_DEFAULT, 
        origin, 0.001, direction, MAX_RAY_LENGTH, 
        PAYLOAD_INDEX_DEFAULT); 

    return payload;
}

// Get sky color for primary visibility, i.e. without skyColorMultiplier
vec3 getSkyPrimary(vec3 direction)
{
    uint skyType = globalUniform.skyType;

#ifdef DESC_SET_RENDER_CUBEMAP
    if (skyType == SKY_TYPE_RASTERIZED_GEOMETRY)
    {
        return texture(renderCubemap, direction).rgb;
    }
    else 
#endif
    if (skyType == SKY_TYPE_CUBEMAP)
    {
        return texture(globalCubemaps[nonuniformEXT(globalUniform.skyCubemapIndex)], direction).rgb;
    }
    else if (skyType == SKY_TYPE_RAY_TRACED_GEOMETRY)
    {
        ShPayload p = traceSkyRay(globalUniform.skyViewerPosition.xyz, direction);

        if (isPayloadConsistent(p))
        {
            return getHitInfoAlbedoOnly(p);
        }
    }

    return globalUniform.skyColorDefault.xyz;
}

vec3 getSky(vec3 direction)
{
    vec3 col = getSkyPrimary(direction);
    float l = getLuminance(col);
    
    const float saturation = 0.5;

    return mix(vec3(l), col, saturation) * globalUniform.skyColorMultiplier;
}
#endif



#ifdef RAYGEN_SHADOW_PAYLOAD

// l is pointed to the light
bool traceShadowRay(uint surfInstCustomIndex, vec3 o, vec3 l, float maxDistance)
{
    // prepare shadow payload
    payloadShadow.isShadowed = 1;  

    uint cullMask = getShadowCullMask(surfInstCustomIndex);

    traceRayEXT(
        topLevelAS, 
        gl_RayFlagsSkipClosestHitShaderEXT, 
        cullMask, 
        0, 0, 	// sbtRecordOffset, sbtRecordStride
        SBT_INDEX_MISS_SHADOW, 		// shadow missIndex
        o, 0.001, l, maxDistance, 
        PAYLOAD_INDEX_SHADOW);

    return payloadShadow.isShadowed == 1;
}

bool traceShadowRay(uint surfInstCustomIndex, vec3 o, vec3 l)
{
    return traceShadowRay(surfInstCustomIndex, o, l, MAX_RAY_LENGTH);
}



#define SHADOW_RAY_EPS_MIN      0.001
#define SHADOW_RAY_EPS_MAX      0.1
#define SHADOW_RAY_EPS_MAX_DIST 25

#define SHADOW_CAST_LUMINANCE_THRESHOLD 0.01



// toViewerDir -- is direction to viewer
// distanceToViewer -- used for shadow ray origin fix, so it can't be under the surface
void processDirectionalLight(
    uint seed, 
    uint surfInstCustomIndex, vec3 surfPosition, vec3 surfNormal, vec3 surfNormalGeom, float surfRoughness, 
    vec3 toViewerDir, float distanceToViewer,
    bool isGradientSample,
    out vec3 outDiffuse, out vec3 outSpecular)
{
    uint dirLightCount;
    uint dirLightIndex;

    if (!isGradientSample)
    {
        dirLightCount = globalUniform.lightCountDirectional;
 
        if (dirLightCount == 0)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }
        
        const float randomIndex = dirLightCount * getRandomSample(seed, RANDOM_SALT_DIRECTIONAL_LIGHT_INDEX).x;
        dirLightIndex = clamp(uint(randomIndex), 0, dirLightCount - 1);
    }
    else
    {
        dirLightCount = globalUniform.lightCountDirectionalPrev;

        if (dirLightCount == 0)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }

        // choose light using prev frame's info
        const float randomIndex = dirLightCount * getRandomSample(seed, RANDOM_SALT_DIRECTIONAL_LIGHT_INDEX).x;
        const uint prevFrameDirLightIndex = clamp(uint(randomIndex), 0, dirLightCount - 1);

        // get cur frame match for the chosen light
        dirLightIndex = lightSourcesDirMatchPrev[prevFrameDirLightIndex];

        // if light disappeared
        if (dirLightIndex == UINT32_MAX)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }
    }

    const float oneOverPdf = dirLightCount;

    const ShLightDirectional dirLight = lightSourcesDirecitional[dirLightIndex];

    const vec2 u = getRandomSample(seed, RANDOM_SALT_DIRECTIONAL_LIGHT_DISK).xy;    
    const vec2 disk = sampleDisk(dirLight.tanAngularRadius, u[0], u[1]);

    const mat3 basis = getONB(dirLight.direction);
    const vec3 dir = normalize(dirLight.direction + basis[0] * disk.x + basis[1] * disk.y);

    const float nl = dot(surfNormal, dir);
    const float ngl = dot(surfNormalGeom, dir);

    if (nl <= 0 || ngl <= 0)
    {
        outDiffuse = vec3(0.0);
        outSpecular = vec3(0.0);
        return;
    }

    outDiffuse = evalBRDFLambertian(1.0) * dirLight.color * nl * M_PI;
    outSpecular = evalBRDFSmithGGX(surfNormal, toViewerDir, dirLight.direction, surfRoughness) * dirLight.color * nl;

    outDiffuse *= oneOverPdf;
    outSpecular *= oneOverPdf;

    // if too dim, don't cast shadow ray
    if (getLuminance(outDiffuse) + getLuminance(outSpecular) < SHADOW_CAST_LUMINANCE_THRESHOLD)
    {
        return;
    }

    const float shadowRayEps = mix(SHADOW_RAY_EPS_MIN, SHADOW_RAY_EPS_MAX, distanceToViewer / SHADOW_RAY_EPS_MAX_DIST);

    const bool isShadowed = traceShadowRay(surfInstCustomIndex, surfPosition, dir);

    outDiffuse *= float(!isShadowed);
    outSpecular *= float(!isShadowed);
}

void processSphericalLight(
    uint seed,
    uint surfInstCustomIndex, vec3 surfPosition, vec3 surfNormal, vec3 surfNormalGeom, float surfRoughness, 
    vec3 toViewerDir, 
    bool isGradientSample,
    out vec3 outDiffuse, out vec3 outSpecular)
{
    uint sphLightCount;
    uint sphLightIndex;

    if (!isGradientSample)
    {
        sphLightCount = globalUniform.lightCountSpherical;
 
        if (sphLightCount == 0)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }
        
        const float randomIndex = sphLightCount * getRandomSample(seed, RANDOM_SALT_SPHERICAL_LIGHT_INDEX).x;
        sphLightIndex = clamp(uint(randomIndex), 0, sphLightCount - 1);
    }
    else
    {
        sphLightCount = globalUniform.lightCountSphericalPrev;

        if (sphLightCount == 0)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }

        // choose light using prev frame's info
        const float randomIndex = sphLightCount * getRandomSample(seed, RANDOM_SALT_SPHERICAL_LIGHT_INDEX).x;
        const uint prevFrameSphLightIndex = clamp(uint(randomIndex), 0, sphLightCount - 1);

        // get cur frame match for the chosen light
        sphLightIndex = lightSourcesSphMatchPrev[prevFrameSphLightIndex];

        // if light disappeared
        if (sphLightIndex == UINT32_MAX)
        {
            outDiffuse = vec3(0.0);
            outSpecular = vec3(0.0);
            return;
        }
    }
    
    const float oneOverPdf = sphLightCount;

    const ShLightSpherical sphLight = lightSourcesSpherical[sphLightIndex];

    const vec2 u = getRandomSample(seed, RANDOM_SALT_SPHERICAL_LIGHT_DISK).xy;
    const vec3 posOnSphere = sphLight.position + sampleSphere(u[0], u[1]) * sphLight.radius;

    vec3 dir = posOnSphere - surfPosition;
    const float distance = length(dir);

    surfPosition += (toViewerDir + surfNormalGeom) * SHADOW_RAY_EPS_MAX;

    if (distance < sphLight.radius)
    {
        outDiffuse = sphLight.color;
        outSpecular = sphLight.color;
        return;
    }
    
    dir = dir / distance;

    vec3 dirToCenter = sphLight.position - surfPosition;

    const float r = sphLight.radius;
    const float z = sphLight.falloff;
    const float d = max(length(dirToCenter), 0.0001);
    dirToCenter /= d;
    
    const float i = pow(clamp((z - d) / max(z - r, 1), 0, 1), 2);
    const vec3 c = i * sphLight.color;

    const vec3 irradiance = M_PI * c * max(dot(surfNormal, dirToCenter), 0.0);
    const vec3 radiance = evalBRDFLambertian(1.0) * irradiance;

    outDiffuse = radiance;
    outSpecular = evalBRDFSmithGGX(surfNormal, toViewerDir, dir, surfRoughness) * sphLight.color * dot(surfNormal, dir);

    outDiffuse *= oneOverPdf;
    outSpecular *= oneOverPdf;
    
    if (getLuminance(outDiffuse) + getLuminance(outSpecular) < SHADOW_CAST_LUMINANCE_THRESHOLD)
    {
        return;
    }
    
    const bool isShadowed = traceShadowRay(surfInstCustomIndex, surfPosition, dir, distance);

    outDiffuse *= float(!isShadowed);
    outSpecular *= float(!isShadowed);
}

void processDirectIllumination(
    uint seed, 
    uint surfInstCustomIndex, vec3 surfPosition, vec3 surfNormal, vec3 surfNormalGeom, float surfRoughness,
    const vec3 toViewerDir, float distanceToViewer,
    bool isGradientSample,
    out vec3 outDiffuse, out vec3 outSpecular)
{
    vec3 dirDiff, dirSpec;
    processDirectionalLight(
        seed, 
        surfInstCustomIndex, surfPosition, surfNormal, surfNormalGeom, surfRoughness, 
        toViewerDir, distanceToViewer,
        isGradientSample, 
        dirDiff, dirSpec);
    
    vec3 sphDiff, sphSpec;
    processSphericalLight(
        seed, 
        surfInstCustomIndex, surfPosition, surfNormal, surfNormalGeom, surfRoughness, 
        toViewerDir, 
        isGradientSample, 
        sphDiff, sphSpec);
    
    outDiffuse = dirDiff + sphDiff;
    outSpecular = dirSpec + sphSpec;
}

void processDirectIllumination(
    uint seed, 
    uint surfInstCustomIndex, vec3 surfPosition, vec3 surfNormal, vec3 surfNormalGeom, float surfRoughness,
    vec3 toViewerDir,
    bool isGradientSample,
    out vec3 outDiffuse, out vec3 outSpecular)
{
    processDirectIllumination(
        seed, 
        surfInstCustomIndex, surfPosition, surfNormal, surfNormalGeom, surfRoughness, 
        toViewerDir, length(surfPosition - globalUniform.cameraPosition.xyz),
        isGradientSample, 
        outDiffuse, outSpecular);
}
#endif // RAYGEN_SHADOW_PAYLOAD