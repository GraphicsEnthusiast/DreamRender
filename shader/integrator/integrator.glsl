#ifndef INTEGRATOR_GLSL
#define INTEGRATOR_GLSL

#include "camera/camera.glsl"
#include "light/light.glsl"
#include "sample/filter.glsl"

/**
 * @brief Checks if hit represents a boundary material
 * @param info Intersection information
 * @return Boolean indicating if material is a boundary
 */
bool HitBoundaryMaterial(IntersectionInfo info) {
    return MaterialType_Boundary == info.material.type;
}

/**
 * @brief Checks if hit represents nothing (no intersection)
 * @param info Intersection information
 * @return Boolean indicating if no intersection occurred
 */
bool HitNothing(IntersectionInfo info) {
    return MaxFloat == info.distance;
}

/**
 * @brief Checks if hit represents a light source
 * @param info Intersection information
 * @return Boolean indicating if intersection is with a light source
 */
bool HitLight(IntersectionInfo info) {
    return info.is_light;
}

/**
 * @brief Gets intersection information from hit data
 * @param hit Hit information
 * @param ray_direction The direction of the incoming ray (normalized)
 * @param lambda Sampled wavelengths
 * @return IntersectionInfo with material, texture, and medium data
 */
IntersectionInfo GetIntersectionInfo(Hit hit, vec3 ray_direction, SampledWavelengths lambda) {
    IntersectionInfo info;
    Triangle tri;
    if (hit.is_light) {
        tri = FetchTriangle(hit.tri_index, TrianglesLight);
    }
    else {
        tri = FetchTriangle(hit.tri_index, Triangles);
    }

    info.is_light = hit.is_light;

    // Interpolate position
    vec3 position = (1.0f - hit.u - hit.v) * tri.p1 + hit.u * tri.p2 + hit.v * tri.p3;

    // Interpolate shading normal
    vec3 shading_normal = normalize((1.0f - hit.u - hit.v) * tri.n1 + hit.u * tri.n2 + hit.v * tri.n3);

    // Interpolate texture coordinates
    vec2 uv = (1.0f - hit.u - hit.v) * tri.t1 + hit.u * tri.t2 + hit.v * tri.t3;

    // Calculate geometry normal
    vec3 edge1 = tri.p2 - tri.p1;
    vec3 edge2 = tri.p3 - tri.p1;
    vec3 geometry_normal = normalize(cross(edge1, edge2));

    // Create initial intersection info
    info.tri_index = hit.tri_index;
    info.distance = hit.distance;
    info.position = position;
    info.uv = uv;

    // Set and adjust normal information
    info = SetNormal(info, ray_direction, geometry_normal, shading_normal);

    // Create material
    Material mat;
    mat.type = tri.material_type;

    //int emission_texture_id = int(tri.emission.w);
    int diffuse_texture_id = int(tri.diffuse.w);
    int roughness_texture_id = int(tri.roughness.w);

    // Get emission
    //RGB emission_rgb = RGBNew(tri.emission.r, tri.emission.g, tri.emission.b);
    //RGBIlluminantSpectrum emission_spectrum = RGBIlluminantSpectrumNew(emission_rgb);
    //mat.emission = RGBIlluminantSpectrumSample(emission_spectrum, lambda);
    //mat.emission_texture = emission_texture_id;

    // Get diffuse
    RGB diffuse = RGBNew(tri.diffuse.r, tri.diffuse.g, tri.diffuse.b);
    RGBAlbedoSpectrum diffuse_spectrum = RGBAlbedoSpectrumNew(diffuse);
    mat.diffuse = RGBAlbedoSpectrumSample(diffuse_spectrum, lambda);
    mat.diffuse_texture = diffuse_texture_id;

    // Get roughness
    mat.roughness = tri.roughness.x;
    mat.roughness_texture = roughness_texture_id;

    // Set the material
    info.material = mat;

    // Create medium from triangle data
    Medium medium;
    info.has_medium = false;

    // Determine which side of the surface the ray is coming from
    if (info.front_face) {
        if (tri.has_out_medium) {
            info.has_medium = true;
            medium.phase_type = tri.out_phase_type;
            medium.g = tri.out_g;
            medium.type = tri.out_medium_type;

            RGB sigma_s_rgb = RGBNew(tri.out_sigma_s.r, tri.out_sigma_s.g, tri.out_sigma_s.b);
            RGB sigma_t_rgb = RGBNew(tri.out_sigma_t.r, tri.out_sigma_t.g, tri.out_sigma_t.b);
            RGBAlbedoSpectrum sigma_s_spectrum = RGBAlbedoSpectrumNew(sigma_s_rgb);
            RGBAlbedoSpectrum sigma_t_spectrum = RGBAlbedoSpectrumNew(sigma_t_rgb);
            medium.sigma_s = RGBAlbedoSpectrumSample(sigma_s_spectrum, lambda);
            medium.sigma_t = RGBAlbedoSpectrumSample(sigma_t_spectrum, lambda);
        }
    }
    else {
        if (tri.has_in_medium) {
            info.has_medium = true;
            medium.phase_type = tri.in_phase_type;
            medium.g = tri.in_g;
            medium.type = tri.in_medium_type;

            RGB sigma_s_rgb = RGBNew(tri.in_sigma_s.r, tri.in_sigma_s.g, tri.in_sigma_s.b);
            RGB sigma_t_rgb = RGBNew(tri.in_sigma_t.r, tri.in_sigma_t.g, tri.in_sigma_t.b);
            RGBAlbedoSpectrum sigma_s_spectrum = RGBAlbedoSpectrumNew(sigma_s_rgb);
            RGBAlbedoSpectrum sigma_t_spectrum = RGBAlbedoSpectrumNew(sigma_t_rgb);
            medium.sigma_s = RGBAlbedoSpectrumSample(sigma_s_spectrum, lambda);
            medium.sigma_t = RGBAlbedoSpectrumSample(sigma_t_spectrum, lambda);
        }
    }

    info.medium = medium;

    return info;
}

/**
 * @brief Updates intersection information after medium scattering
 * @param info Original intersection info before medium scattering
 * @param actual_distance Actual distance traveled in the medium
 * @param last_position Starting position before entering the medium
 * @param world_l Direction of the ray
 * @return Updated IntersectionInfo with new position and adjusted parameters
 */
IntersectionInfo UpdateIntersectionInfoByMedium(IntersectionInfo info, float actual_distance, vec3 last_position, vec3 world_l) {
    // Only using position, distance, has_medium, medium
    IntersectionInfo medium_info;

    medium_info.position = last_position + actual_distance * world_l;
    medium_info.distance = actual_distance;
    medium_info.has_medium = info.has_medium;
    medium_info.medium = info.medium;

    return medium_info;
}

/**
 * @brief Power heuristic for multiple importance sampling
 * @param pdf1 PDF of first sampling strategy
 * @param pdf2 PDF of second sampling strategy
 * @param beta Power parameter (typically 2.0)
 * @return Weight for combining samples
 */
float PowerHeuristic(float pdf1, float pdf2, float beta) {
    float w1 = pow(pdf1, beta);
    float w2 = pow(pdf2, beta);
    if (0.0f == w1 + w2) {
        return 0.0f;
    }

    return w1 / (w1 + w2);
}

#endif // INTEGRATOR_GLSL