#ifndef _INTEGRATOR__GLSL__
#define _INTEGRATOR__GLSL__

#include "shape/shape.glsl"
#include "material/material.glsl"
#include "sample/filter.glsl"
#include "camera/camera.glsl"
#include "spectrum/spectrum.glsl"
#include "light/light.glsl"

/**
 * @brief Gets intersection information from hit data
 * @param hit Hit information
 * @param ray_direction The direction of the incoming ray (normalized)
 * @param is_light Whether this is a light triangle
 * @param lambda Sampled wavelengths
 * @return IntersectionInfo with material and texture data
 */
IntersectionInfo GetIntersectionInfo(Hit hit, vec3 ray_direction, bool is_light, SampledWavelengths lambda) {
    Triangle tri;
    if (is_light) {
        tri = FetchTriangle(hit.tri_index, TrianglesLight);
    } 
    else {
        tri = FetchTriangle(hit.tri_index, Triangles);
    }
    
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
    IntersectionInfo info;
    info.tri_index = hit.tri_index;
    info.distance = hit.distance;
    info.position = position;
    info.uv = uv;
    
    // Set and adjust normal information
    info = SetNormal(info, ray_direction, geometry_normal, shading_normal);
    
    // Create material
    Material mat;
    mat.type = tri.material_type;
    
    int diffuse_texture_id = int(tri.diffuse.w);
    int roughness_texture_id = int(tri.roughness.w);
    
    // Get base diffuse color
    RGB base_diffuse_rgb = RGBNew(tri.diffuse.x, tri.diffuse.y, tri.diffuse.z);
    RGBAlbedoSpectrum base_diffuse_spectrum = RGBAlbedoSpectrumNew(base_diffuse_rgb);
    mat.diffuse = RGBAlbedoSpectrumSample(base_diffuse_spectrum, lambda);
    mat.diffuse_texture = diffuse_texture_id;
    
    // Get base roughness
    mat.roughness = tri.roughness.x;
    mat.roughness_texture = roughness_texture_id;
    
    // Get emission from triangle data
    RGB emission_rgb = RGBNew(tri.emission.r, tri.emission.g, tri.emission.b);
    RGBIlluminantSpectrum emission_spectrum = RGBIlluminantSpectrumNew(emission_rgb);
    mat.emission = RGBIlluminantSpectrumSample(emission_spectrum, lambda);
    
    // Set the material
    info.material = mat;
    
    return info;
}

/**
 * @brief Performs binary visibility test from start point in given direction and distance
 * @param start_point Starting point of the visibility test
 * @param direction Normalized direction to test visibility towards
 * @param distance Maximum distance to test for occlusion
 * @return float representing visibility weight: 1.0f for completely visible, 0.0f for occluded
 */
float VisibilityTest(vec3 start_point, vec3 direction, float distance) {
    // Create shadow ray with epsilon offset to avoid self-intersection
    Ray shadow_ray = SpawnShadowRay(start_point, direction, distance);
    
    // Traverse acceleration structure to find closest intersection
    Hit occlusion_hit = BVHTraverse(shadow_ray);
    
    // Check if any geometry was hit before reaching the target distance
    // shadow_ray.tmax defines the maximum distance to test (typically the light distance)
    if (occlusion_hit.distance >= shadow_ray.tmax) {
        return 1.0f;
    }
    
    return 0.0f;
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

/**
 * @brief Main path tracing integrator with multiple importance sampling
 * @param pixel_coords Current pixel coordinates
 * @param cam Camera parameters
 * @param sobol_sampler Pre-initialized Sobol quasi-random sequence sampler for Monte Carlo integration
 * @param lambda Pre-sampled wavelengths for spectral rendering; contains wavelength values
 * @param max_bounce Maximum number of light bounces
 * @return SampledSpectrum representing accumulated radiance
 */
SampledSpectrum PathTracing(ivec2 pixel_coords, Camera cam, inout SobolSampler sobol_sampler, SampledWavelengths lambda, float max_bounce) {
    // ========================= Initialization =========================
    // Generate pixel sample
    vec2 pixel_center = vec2(pixel_coords);
    vec2 jitter = GaussianFilter(vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler)));
    pixel_center += jitter;

    // Initialize path tracing
    SampledSpectrum beta = SampledSpectrumNewFloat(1.0f);
    SampledSpectrum L = SampledSpectrumNewFloat(0.0f);
    IntersectionInfo info;
    // ========================= Initialization =========================

    // ========================= Process first intersection (bounce = 0) separately =========================
    // Generate primary ray
    vec2 lens_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
    Ray ray = GeneratePrimaryRay(cam, pixel_center.x, pixel_center.y, lens_sample);

    float cos_theta = dot(ray.direction, -cam.forward);
    float camera_pdf = CameraPDF(cam, ray.direction);
    float we = CameraWe(cam, cos_theta);
    
    if (camera_pdf > 0.0f) {
        // camera_sampling_weight: we * cosθ / camera_pdf = 1
        float camera_sampling_weight = we * cos_theta / camera_pdf;
        
        // Multiply current path throughput by camera weight
        beta = MulFloat(beta, camera_sampling_weight);
    }
    else {
        return L;
    }

    Hit hit = BVHTraverse(ray);
    // Get intersection info
    bool is_light = hit.is_light;

    // No environment light
    if (MaxFloat == hit.distance) {
        return L;
    }

    info = GetIntersectionInfo(hit, ray.direction, is_light, lambda);
    // Handle direct light hit
    if (is_light) {
        L = Add(L, Mul(beta, info.material.emission));

        return L;
    }
    // ========================= Process first intersection (bounce = 0) separately =========================
    
    // ========================= Start main path tracing loop from bounce = 1 =========================
    vec3 last_shading_point = info.position;
    vec3 world_v = -normalize(ray.direction);
    for (int bounce = 1; bounce < max_bounce; ++bounce) {
        // ========================= Light Sampling =========================
        LightSampleInfo light_sample_info = MeshLightSample(sobol_sampler, info, lambda);
        MaterialEvalInfo mat_eval_info = MaterialEvaluate(info, world_v, light_sample_info.world_l, lambda);
        float visibility = VisibilityTest(info.position, light_sample_info.world_l, light_sample_info.distance);

        if (light_sample_info.pdf > 0.0f && mat_eval_info.pdf > 0.0f) {
            // Le * f * cosθ / light_pdf
            SampledSpectrum Le_f_cos = Mul(light_sample_info.emission, mat_eval_info.bsdf_cosine);
            SampledSpectrum light_sampling_contribution = DivFloat(Le_f_cos, light_sample_info.pdf);
    
            // mis_weight = (light_pdf^2) / (light_pdf^2 + bsdf_pdf^2)
            float mis_weight = PowerHeuristic(light_sample_info.pdf, mat_eval_info.pdf, 2.0f);
    
            // light_sampling_contribution = β * visibility * ((mis_weight * Le * f * cosθ) / light_pdf)
            SampledSpectrum final_light_sampling_contribution = Mul(MulFloat(MulFloat(light_sampling_contribution, mis_weight), visibility), beta);
    
            // Accumulate to total radiance
            L = Add(L, final_light_sampling_contribution);
        }
        // ========================= Light Sampling =========================
        
        // ========================= Material Sampling =========================
        vec2 bsdf_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        MaterialSampleInfo mat_sample_info = MaterialSample(info, world_v, bsdf_sample, lambda);

        // Cast ray in sampled direction
        Ray material_ray = SpawnRay(info.position, mat_sample_info.world_l, info.geometry_normal, 0.0f, MaxFloat);
        Hit new_hit = BVHTraverse(material_ray);

        // No environment light
        if (MaxFloat == new_hit.distance) {
            break;
        }

        // Check if ray hits a light source
        // If it does not hit the light source, the contribution is 0, so it can be disregarded
        if (new_hit.is_light) {
            info = GetIntersectionInfo(new_hit, normalize(material_ray.direction), true, lambda);
            LightEvalInfo light_eval_info = MeshLightEvaluate(mat_sample_info.world_l, info, last_shading_point, lambda);
    
            if (light_eval_info.pdf > 0.0f && mat_sample_info.pdf > 0.0f) {
                // Le * f * cosθ / bsdf_pdf
                SampledSpectrum Le_f_cos = Mul(info.material.emission, mat_sample_info.bsdf_cosine);
                SampledSpectrum bsdf_sampling_contribution = DivFloat(Le_f_cos, mat_sample_info.pdf);
        
                // mis_weight = (bsdf_pdf^2) / (bsdf_pdf^2 + light_pdf^2)
                float mis_weight = PowerHeuristic(mat_sample_info.pdf, light_eval_info.pdf, 2.0f);

                // bsdf_sampling_contribution = β * ((mis_weight * Le * f * cosθ) / bsdf_pdf)
                SampledSpectrum final_bsdf_sampling_contribution = Mul(MulFloat(bsdf_sampling_contribution, mis_weight), beta);
        
                // Accumulate to total radiance
                L = Add(L, final_bsdf_sampling_contribution);
            }
    
            // Terminate path after hitting a light source
            break;
        }
        // ========================= Material Sampling =========================

        // ========================= Russian Roulette Wheel =========================
        if (bounce > 3 && Max(beta) < 0.1f) {
            float q = max(0.05f, 1.0f - Max(beta));
            if (SobolSamplerGet1(sobol_sampler) < q) {
                break;
            }

            beta = DivFloat(beta, 1.0f - q);
        }

        // Update path throughput
        beta = Mul(beta, DivFloat(mat_sample_info.bsdf_cosine, mat_sample_info.pdf));
        // Update view direction for next bounce
        ray = material_ray;
        world_v = -normalize(ray.direction);
        last_shading_point = info.position;
        // ========================= Russian Roulette Wheel =========================
    }
    // ========================= Start main path tracing loop from bounce = 1 =========================
    
    return L;
}

#endif // _INTEGRATOR__GLSL__