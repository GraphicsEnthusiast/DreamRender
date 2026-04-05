#ifndef LIGHT_GLSL
#define LIGHT_GLSL

#include "shape/shape.glsl"
#include "material/material.glsl"

uniform samplerBuffer MeshLightTable;
layout(location = 1) uniform float MeshLightTableMax;
layout(location = 2) uniform float MeshLightTableSum;
layout(location = 3) uniform int MeshLightTableSize;

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
    float distance;
    float pdf;
    SampledSpectrum emission;
};

/**
 * @brief Retrieves the final emission color with texture mapping support
 * @param tri Triangle containing emission data (stored in .rgb for base color, .w for texture ID)
 * @param uv Texture coordinates for texture sampling
 * @param lambda Sampled wavelengths for spectral rendering conversion
 * @return SampledSpectrum representing the final emission color; uses texture if available, otherwise falls back to triangle's base emission color
 */
SampledSpectrum GetFinalEmission(Triangle tri, vec2 uv, SampledWavelengths lambda) {
    int emission_texture_id = int(tri.emission.w);
    if (emission_texture_id >= 0 && emission_texture_id < TextureCount) {
        vec4 tex_color = SampleTextureArray(emission_texture_id, uv);
        
        RGB emission_rgb = RGBNew(tex_color.r, tex_color.g, tex_color.b);
        RGBIlluminantSpectrum emission_spectrum = RGBIlluminantSpectrumNew(emission_rgb);

        return RGBIlluminantSpectrumSample(emission_spectrum, lambda);
    }
    else {
        RGB emission_rgb = RGBNew(tri.emission.r, tri.emission.g, tri.emission.b);
        RGBIlluminantSpectrum emission_spectrum = RGBIlluminantSpectrumNew(emission_rgb);

        return RGBIlluminantSpectrumSample(emission_spectrum, lambda);
    }
}

/**
 * @brief Computes the solid angle (projected area) of a spherical triangle
 * @param va First vertex of spherical triangle (normalized)
 * @param vb Second vertex of spherical triangle (normalized)
 * @param vc Third vertex of spherical triangle (normalized)
 * @return Solid angle of the spherical triangle in steradians
 */
float SphericalTriangleSolidAngle(vec3 va, vec3 vb, vec3 vc) {
    // Calculate cosines of the spherical triangle's edge lengths
    float cos_a = clamp(dot(vb, vc), -1.0f, 1.0f);
    float cos_b = clamp(dot(vc, va), -1.0f, 1.0f);
    float cos_c = clamp(dot(va, vb), -1.0f, 1.0f);
    
    // Calculate sines
    float sin_a = sqrt(max(1.0f - cos_a * cos_a, 0.0f));
    float sin_b = sqrt(max(1.0f - cos_b * cos_b, 0.0f));
    float sin_c = sqrt(max(1.0f - cos_c * cos_c, 0.0f));
    
    // If two vertices are coincident, area is zero
    if (cos_a == 1.0f || cos_b == 1.0f || cos_c == 1.0f) {
        return 0.0f;
    }
    
    // Calculate cosines of the angles at the vertices
    float cos_va = clamp((cos_a - cos_b * cos_c) / (sin_b * sin_c), -1.0f, 1.0f);
    float cos_vb = clamp((cos_b - cos_c * cos_a) / (sin_c * sin_a), -1.0f, 1.0f);
    float cos_vc = clamp((cos_c - cos_a * cos_b) / (sin_a * sin_b), -1.0f, 1.0f);
    
    // Calculate the angles themselves, in radians
    float ang_va = acos(cos_va);
    float ang_vb = acos(cos_vb);
    float ang_vc = acos(cos_vc);
    
    // Calculate and return the solid angle of the triangle
    return ang_va + ang_vb + ang_vc - PI;
}

/**
 * @brief Computes orthogonal component of vector v relative to reference vector n
 * @param v Vector to project
 * @param n Reference vector (must be normalized)
 * @return Orthogonal component of v relative to n (not normalized)
 */
vec3 OrthogonalComponent(vec3 v, vec3 n) {
    return v - n * dot(v, n);
}

/**
 * @brief Generates a uniform sample on a spherical triangle using two uniform random variables
 * @param va First vertex of spherical triangle (normalized)
 * @param vb Second vertex of spherical triangle (normalized)
 * @param vc Third vertex of spherical triangle (normalized)
 * @param i First uniform random variable in [0, 1)
 * @param j Second uniform random variable in [0, 1)
 * @return Uniformly sampled direction on the spherical triangle (normalized)
 */
vec3 SphericalTriangleSampleUniform(vec3 va, vec3 vb, vec3 vc, float i, float j) {
    // Calculate cosines of the spherical triangle's edge lengths
    float cos_a = clamp(dot(vb, vc), -1.0f, 1.0f);
    float cos_b = clamp(dot(vc, va), -1.0f, 1.0f);
    float cos_c = clamp(dot(va, vb), -1.0f, 1.0f);
    
    // Calculate sines
    float sin_a = sqrt(max(1.0f - cos_a * cos_a, 0.0f));
    float sin_b = sqrt(max(1.0f - cos_b * cos_b, 0.0f));
    float sin_c = sqrt(max(1.0f - cos_c * cos_c, 0.0f));
    
    // If two vertices are coincident, return first vertex
    if (cos_a == 1.0f || cos_b == 1.0f || cos_c == 1.0f) {
        return va;
    }
    
    // Calculate cosines of the angles at the vertices
    float cos_va = clamp((cos_a - cos_b * cos_c) / (sin_b * sin_c), -1.0f, 1.0f);
    float cos_vb = clamp((cos_b - cos_c * cos_a) / (sin_c * sin_a), -1.0f, 1.0f);
    float cos_vc = clamp((cos_c - cos_a * cos_b) / (sin_a * sin_b), -1.0f, 1.0f);
    
    // Calculate sine for angle at vertex A
    float sin_va = sqrt(max(1.0f - cos_va * cos_va, 0.0f));
    
    // Calculate the angles themselves, in radians
    float ang_va = acos(cos_va);
    float ang_vb = acos(cos_vb);
    float ang_vc = acos(cos_vc);
    
    // Calculate the area (solid angle) of the spherical triangle
    float area = ang_va + ang_vb + ang_vc - PI;
    
    // Implementation of "Stratified Sampling of Spherical Triangles" by James Arvo
    float area_2 = area * i;
    
    float s = sin(area_2 - ang_va);
    float t = cos(area_2 - ang_va);
    float u = t - cos_va;
    float v = s + (sin_va * cos_c);
    
    // Calculate q (cos(beta^))
    float q_top = ((v * t) - (u * s)) * cos_va - v;
    float q_bottom = (v * s + u * t) * sin_va;
    
    // Prevent division by zero
    float q = 1.0f;
    if (abs(q_bottom) > Epsilon) {
        q = q_top / q_bottom;
    }
    
    // The new vertex of the sub-triangle
    vec3 ortho_comp = OrthogonalComponent(vc, va);
    float ortho_len = length(ortho_comp);
    vec3 vc_2 = vec3(0.0f);
    if (ortho_len > Epsilon) {
        ortho_comp /= ortho_len; // Normalize
        vc_2 = va * q + ortho_comp * sqrt(max(1.0f - q * q, 0.0f));
    } 
    else {
        vc_2 = va;
    }
    
    // Final sampling along edge BC^
    float z = 1.0f - j * (1.0f - dot(vc_2, vb));
    
    vec3 ortho_vc2_vb = OrthogonalComponent(vc_2, vb);
    float ortho_len2 = length(ortho_vc2_vb);
    if (ortho_len2 > Epsilon) {
        ortho_vc2_vb /= ortho_len2; // Normalize
    }
    
    return vb * z + ortho_vc2_vb * sqrt(max(1.0f - z * z, 0.0f));
}

/**
 * @brief Samples a point on a triangle using spherical triangle sampling
 * @param triangle_vertices Array of 3 triangle vertices in world space
 * @param position Position of the shading point
 * @param i First uniform random sample in [0, 1)
 * @param j Second uniform random sample in [0, 1)
 * @param[out] direction Sampled direction (from shading point to triangle)
 * @param[out] distance Distance to intersection point
 * @param[out] u Barycentric u coordinate
 * @param[out] v Barycentric v coordinate
 * @return True if intersection found, false otherwise
 */
bool TriangleSphericalSample(const vec3 triangle_vertices[3], const vec3 position, float i, float j,
    out vec3 direction, out float distance, out float u, out float v) {
    // Calculate directions from shading point to triangle vertices
    vec3 va = triangle_vertices[0] - position;
    vec3 vb = triangle_vertices[1] - position;
    vec3 vc = triangle_vertices[2] - position;
    
    // Sample direction on spherical triangle
    direction = SphericalTriangleSampleUniform(normalize(va), normalize(vb), normalize(vc), i, j);
    
    // Ray-triangle intersection to find actual intersection point
    // Möller–Trumbore intersection algorithm
    vec3 edge1 = triangle_vertices[1] - triangle_vertices[0];
    vec3 edge2 = triangle_vertices[2] - triangle_vertices[0];
    vec3 h = cross(direction, edge2);
    float a = dot(edge1, h);
    
    if (abs(a) < Epsilon * Epsilon) {
        return false; // Ray is parallel to triangle
    }
    
    float f = 1.0f / a;
    vec3 s = position - triangle_vertices[0];
    u = f * dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false; // Intersection outside triangle
    }
    
    vec3 q = cross(s, edge1);
    v = f * dot(direction, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false; // Intersection outside triangle
    }
    
    distance = f * dot(edge2, q);
    
    if (distance < Epsilon * Epsilon) {
        return false; // Intersection behind ray origin
    }
    
    return true;
}

/**
 * @brief Evaluates mesh light contribution for a given direction
 * @param world_l Light direction (from surface to light)
 * @param info Intersection information of the shading point(located on the light source)
 * @param last_position The previous shading point before hitting the light source
 * @param lambda Sampled wavelengths
 * @return LightEvalInfo containing emission spectrum and PDF
 */
LightEvalInfo MeshLightEvaluate(vec3 world_l, IntersectionInfo info, vec3 last_position, SampledWavelengths lambda) {
    LightEvalInfo result;
    result.emission = SampledSpectrumNewFloat(0.0f);
    result.pdf = 0.0f;
    
    // Early out if light direction is above the surface
    if (!info.front_face) {
        return result;
    }
    
    Triangle tri = FetchTriangle(info.tri_index, TrianglesLight);

    result.emission = GetFinalEmission(tri, info.uv, lambda);
    
    // Calculate directions from shading point to triangle vertices
    // Get triangle weight from precomputed table
    float weight = texelFetch(MeshLightTable, info.tri_index).y;
    
    // Normalize to get directions on unit sphere
    vec3 a = normalize(tri.p1 - last_position);
    vec3 b = normalize(tri.p2 - last_position);
    vec3 c = normalize(tri.p3 - last_position);
    
    // Calculate solid angle of spherical triangle
    float solid_angle = SphericalTriangleSolidAngle(a, b, c);
    if (solid_angle <= 0.0f) {
        return result;
    }
    
    // PDF is 1/solid_angle for uniform sampling on spherical triangle
    result.pdf = 1.0f / solid_angle;
    result.pdf *= weight / MeshLightTableSum;

    return result;
}

/**
 * @brief Samples a point on the mesh area light using spherical triangle sampling
 * @param sobol_sampler Sobol sequence sampler
 * @param info Intersection information for the shading point
 * @param lambda Sampled wavelengths
 * @return LightSampleInfo containing sampled direction, distance, PDF and emission spectrum
 */
LightSampleInfo MeshLightSample(inout SobolSampler sobol_sampler, IntersectionInfo info, SampledWavelengths lambda) {
    LightSampleInfo result;
    result.world_l = vec3(0.0f);
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.emission = SampledSpectrumNewFloat(0.0f);
    
    // Sample triangle index using alias table for O(1) time complexity
    vec2 alias_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
    int tri_index = AliasTable1DSample(
        MeshLightTable, 
        MeshLightTableSize, 
        0, 
        MeshLightTableMax, 
        alias_sample
    );
    
    Triangle tri = FetchTriangle(tri_index, TrianglesLight);
    
    // Prepare triangle vertices
    vec3 triangle_vertices[3] = { tri.p1, tri.p2, tri.p3 };
    
    // Generate random samples for spherical triangle sampling
    float rand_u = SobolSamplerGet1(sobol_sampler);
    float rand_v = SobolSamplerGet1(sobol_sampler);
    
    // Sample using spherical triangle method
    float u, v;
    if (!TriangleSphericalSample(
        triangle_vertices,
        info.position,
        rand_u,
        rand_v,
        result.world_l,
        result.distance,
        u,
        v)) {
        // Failed to intersect, return zero PDF
        return result;
    }
    
    // Compute triangle geometric normal for visibility testing
    vec3 edge1 = tri.p2 - tri.p1;
    vec3 edge2 = tri.p3 - tri.p1;
    vec3 ng = normalize(cross(edge1, edge2));
    
    float cos_theta = dot(result.world_l, ng);
    
    // Early exit if light direction points above the surface (invisible)
    if (cos_theta >= 0.0f) {
        return result;
    }
    
    // Calculate directions from shading point to triangle vertices
    // Normalize to get directions on unit sphere
    vec3 a = normalize(tri.p1 - info.position);
    vec3 b = normalize(tri.p2 - info.position);
    vec3 c = normalize(tri.p3 - info.position);
    
    // Calculate solid angle of spherical triangle
    float solid_angle = SphericalTriangleSolidAngle(a, b, c);
    if (solid_angle <= 0.0f) {
        return result;
    }
    
    // Retrieve triangle weight from precomputed table
    float weight = texelFetch(MeshLightTable, tri_index).y;
    
    // PDF for uniform spherical triangle sampling is 1/solid_angle
    result.pdf = 1.0f / solid_angle;
    result.pdf *= weight / MeshLightTableSum;

    result.emission = GetFinalEmission(tri, vec2(u, v), lambda);
    
    return result;
}

#endif // LIGHT_GLSL