#ifndef _LIGHT__GLSL__
#define _LIGHT__GLSL__

#include "shape/shape.glsl"
#include "sample/sampler.glsl"
#include "sample/sampling.glsl"

uniform int MeshLightSize;

uniform samplerBuffer MeshLightTable;
uniform float MeshLightTableSum;
uniform int MeshLightTableSize;

/**
 * @struct LightEvalInfo
 * @brief Stores the result of light evaluation
 */
struct LightEvalInfo {
    SampledSpectrum emission;
    float pdf;
};

/**
 * @struct LightSampleInfo  
 * @brief Stores the result of light sampling
 */
struct LightSampleInfo {
    vec3 world_l;
    float dist;
    float pdf;
    SampledSpectrum emission;
};

/**
 * @brief Calculates the area of a triangle by its index in the mesh light buffer
 * @param index Triangle index to calculate area for
 * @return float Area of the specified triangle in world units squared
 */
float TriangleArea(int index) {
    // Fetch triangle vertices from the mesh light buffer
    int base = index * 3; // 3 vertices per triangle
    Triangle tri;
    
    vec4 pos1 = texelFetch(Triangles, base + 0);
    vec4 pos2 = texelFetch(Triangles, base + 1); 
    vec4 pos3 = texelFetch(Triangles, base + 2);
    
    tri.p1 = pos1.xyz;
    tri.p2 = pos2.xyz;
    tri.p3 = pos3.xyz;
    
    // Calculate triangle area using cross product method
    vec3 edge1 = tri.p2 - tri.p1;
    vec3 edge2 = tri.p3 - tri.p1;
    vec3 cross_product = cross(edge1, edge2);
    
    return 0.5f * length(cross_product);
}

/**
 * @brief Evaluates mesh light contribution for a given direction
 * @param world_l Light direction (from surface to light)
 * @param info Intersection information
 * @return LightEvalInfo containing emission spectrum and PDF
 */
LightEvalInfo MeshLightEvaluate(vec3 world_l, IntersectionInfo info) {
    LightEvalInfo result;
    result.emission = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    float cos_theta = dot(world_l, info.geometry_normal);
    
    // Early out if light direction is above the surface
    if (cos_theta > 0.0f || info.tri_index >= MeshLightSize) {
        return result;
    }
    
    // Get triangle weight from precomputed table
    float weight = texelFetch(MeshLightTable, info.tri_index).x;
    float area = TriangleArea(info.tri_index);
    
    // Calculate PDF using solid angle conversion
    result.pdf = 1.0f / area;
    result.pdf *= info.distance * info.distance / abs(cos_theta);
    result.pdf *= weight / MeshLightTableSum;

    // Get emission from material
    result.emission = info.material.emission;
    
    return result;
}

/**
 * @brief Samples a point on the mesh area light using Sobol sampling and alias table optimization
 * @param sobol_sampler Sobol sequence sampler (passed as inout reference for state management)
 * @param info Intersection information for the shading point
 * @return LightSampleInfo containing sampled direction, distance, PDF and emission spectrum
 */
LightSampleInfo MeshLightSample(inout SobolSampler sobol_sampler, IntersectionInfo info) {
    LightSampleInfo result;
    result.world_l = vec3(0.0f);
    result.dist = 0.0f;
    result.pdf = 0.0f;
    result.emission = SampledSpectrumNewFloat(0.0f);
    
    // Sample triangle index using alias table for O(1) time complexity
    vec2 alias_sample = vec2(
        SobolSamplerGet1(sobol_sampler), 
        SobolSamplerGet1(sobol_sampler)
    );
    int tri_index = AliasTable1DSample(
        MeshLightTable, 
        MeshLightTableSize, 
        0, 
        MeshLightTableSum, 
        alias_sample
    );
    
    // Fetch triangle geometry data for the sampled index
    Triangle tri = FetchTriangle(tri_index);
    
    // Uniformly sample point on triangle using barycentric coordinates
    float sqrt_xi1 = sqrt(SobolSamplerGet1(sobol_sampler));
    float xi2 = SobolSamplerGet1(sobol_sampler);
    
    float b1 = 1.0f - sqrt_xi1;
    float b2 = sqrt_xi1 * xi2;
    
    // Calculate sampled point using barycentric interpolation
    vec3 p = (1.0f - b1 - b2) * tri.p1 + b1 * tri.p2 + b2 * tri.p3;
    
    // Calculate light direction vector and distance
    result.world_l = p - info.position;
    result.dist = length(result.world_l);
    result.world_l /= result.dist;
    
    // Compute triangle geometric normal for visibility testing
    vec3 edge1 = tri.p2 - tri.p1;
    vec3 edge2 = tri.p3 - tri.p1;
    vec3 ng = normalize(cross(edge1, edge2));
    
    float cos_theta = dot(result.world_l, ng);
    
    // Early exit if light direction points above the surface (invisible)
    if (cos_theta > 0.0f) {
        return result;
    }
    
    // Retrieve triangle weight from precomputed table
    float weight = texelFetch(MeshLightTable, tri_index).x;
    float area = TriangleArea(tri_index);
    
    // Calculate solid angle PDF using area-to-solid angle conversion
    result.pdf = 1.0f / area;
    result.pdf *= result.dist * result.dist / abs(cos_theta);
    result.pdf *= weight / MeshLightTableSum;
    
    // Assign emission spectrum from material properties
    result.emission = info.material.emission;
    
    return result;
}

#endif // _LIGHT__GLSL__