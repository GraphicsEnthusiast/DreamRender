#ifndef INTEGRATOR_GLSL
#define INTEGRATOR_GLSL

#include "sample/filter.glsl"
#include "camera/camera.glsl"
#include "light/light.glsl"
#include "medium/medium.glsl"

/**
 * @brief Gets intersection information from hit data
 * @param hit Hit information
 * @param ray_direction The direction of the incoming ray (normalized)
 * @param lambda Sampled wavelengths
 * @return IntersectionInfo with material, texture, and medium data
 */
IntersectionInfo GetIntersectionInfo(Hit hit, vec3 ray_direction, SampledWavelengths lambda) {
    Triangle tri;
    if (hit.is_light) {
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
    // If front_face is true, ray is hitting from outside, so outside medium is incoming medium
    // If front_face is false, ray is hitting from inside, so inside medium is incoming medium
    if (info.front_face) {
        // Ray is hitting from outside, so use outside medium parameters
        if (tri.has_out_medium) {
            info.has_medium = true;

            medium.phase_type = tri.out_phase_type;
            medium.g = tri.out_g;
            medium.type = tri.out_medium_type;

            // Convert sigma_s and sigma_t from RGB to spectral
            RGB sigma_s_rgb = RGBNew(tri.out_sigma_s.r, tri.out_sigma_s.g, tri.out_sigma_s.b);
            RGB sigma_t_rgb = RGBNew(tri.out_sigma_t.r, tri.out_sigma_t.g, tri.out_sigma_t.b);

            RGBAlbedoSpectrum sigma_s_spectrum = RGBAlbedoSpectrumNew(sigma_s_rgb);
            RGBAlbedoSpectrum sigma_t_spectrum = RGBAlbedoSpectrumNew(sigma_t_rgb);

            medium.sigma_s = RGBAlbedoSpectrumSample(sigma_s_spectrum, lambda);
            medium.sigma_t = RGBAlbedoSpectrumSample(sigma_t_spectrum, lambda);
        }
    }
    else {
        // Ray is hitting from inside, so use inside medium parameters
        if (tri.has_in_medium) {
            info.has_medium = true;

            medium.phase_type = tri.in_phase_type;
            medium.g = tri.in_g;
            medium.type = tri.in_medium_type;

            // Convert sigma_s and sigma_t from RGB to spectral
            RGB sigma_s_rgb = RGBNew(tri.in_sigma_s.r, tri.in_sigma_s.g, tri.in_sigma_s.b);
            RGB sigma_t_rgb = RGBNew(tri.in_sigma_t.r, tri.in_sigma_t.g, tri.in_sigma_t.b);

            RGBAlbedoSpectrum sigma_s_spectrum = RGBAlbedoSpectrumNew(sigma_s_rgb);
            RGBAlbedoSpectrum sigma_t_spectrum = RGBAlbedoSpectrumNew(sigma_t_rgb);

            medium.sigma_s = RGBAlbedoSpectrumSample(sigma_s_spectrum, lambda);
            medium.sigma_t = RGBAlbedoSpectrumSample(sigma_t_spectrum, lambda);
        }
    }

    // Set the medium
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
    // Only using position, distance, medium
    // Update the position to the scattering point
    info.position = last_position + actual_distance * world_l;
    // Adjust the remaining distance
    info.distance = actual_distance;

    return info;
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

// ================================================== PathTracing ==================================================
/**
 * @brief Samples direct lighting from a point and calculates its contribution
 * @param position Starting position of the ray (shading point)
 * @param distance Total distance to the light sample
 * @param light_sample_info Information about the sampled light point (direction, medium properties)
 * @param beta Current path throughput (used for Russian roulette decisions)
 * @param lambda Sampled wavelengths for spectral rendering
 * @param mult_trans_pdf Accumulated PDF of the transmittance sampling (product of segment PDFs)
 * @param sobol_sampler Sobol sampler for random number generation
 * @return SampledSpectrum representing the direct lighting contribution
 */
SampledSpectrum DirectLightVisibility(vec3 position, float distance, LightSampleInfo light_sample_info, SampledSpectrum beta, SampledWavelengths lambda, 
    out float mult_trans_pdf, inout SobolSampler sobol_sampler) {
    float shadow_distance_remaining = distance;
    vec3 shadow_origin = position;
    SampledSpectrum visibility = SampledSpectrumNewFloat(1.0f);
    mult_trans_pdf = 1.0f;

    while (true) {
        Ray shadow_ray = SpawnShadowRay(shadow_origin, light_sample_info.world_l, shadow_distance_remaining);
        Hit occlusion_hit = BVHTraverse(shadow_ray);
        IntersectionInfo occlusion_info;

        // If no intersection, we've reached the light
        if (occlusion_hit.distance >= shadow_ray.tmax) {
            if (light_sample_info.has_medium) {
                occlusion_info.has_medium = true;
                occlusion_info.medium = light_sample_info.out_medium;

                occlusion_info = UpdateIntersectionInfoByMedium(occlusion_info, shadow_distance_remaining, shadow_origin, light_sample_info.world_l);
                MediumEvalInfo medium_eval_info = MediumDistanceEvaluate(occlusion_info, beta, false);

                if (medium_eval_info.pdf > 0.0f) {
                    // Update transmittance and PDF
                    mult_trans_pdf *= medium_eval_info.pdf;
                    visibility = Mul(visibility, DivFloat(medium_eval_info.transmittance, medium_eval_info.pdf));
                }
            }

            // Successfully reached the light
            break;
        }
        else {
            // Get information about the intersected object
            occlusion_info = GetIntersectionInfo(occlusion_hit, shadow_ray.direction, lambda);

            // Check if it's a boundary material
            if (!IsBoundaryMaterial(occlusion_info)) {
                // Not a boundary material, occluded
                visibility = Mul(visibility, SampledSpectrumNewFloat(0.0f));

                break;
            }

            // Handle boundary material
            if (occlusion_info.has_medium) {
                occlusion_info = UpdateIntersectionInfoByMedium(occlusion_info, shadow_distance_remaining, shadow_origin, light_sample_info.world_l);
                MediumEvalInfo medium_eval_info = MediumDistanceEvaluate(occlusion_info, beta, false);

                if (medium_eval_info.pdf > 0.0f) {
                    // Update transmittance and PDF
                    mult_trans_pdf *= medium_eval_info.pdf;
                    visibility = Mul(visibility, DivFloat(medium_eval_info.transmittance, medium_eval_info.pdf));
                }
            }

            shadow_distance_remaining -= occlusion_info.distance;
            shadow_origin = occlusion_info.position;
        }
    }

    return visibility;
}

/**
 * @brief Path tracing integrator with multiple importance sampling
 * @param pixel_coords Current pixel coordinates
 * @param camera Camera parameters
 * @param sobol_sampler Pre-initialized Sobol quasi-random sequence sampler for Monte Carlo integration
 * @param lambda Pre-sampled wavelengths for spectral rendering; contains wavelength values
 * @param max_bounce Maximum number of ray bounces
 * @return SampledSpectrum representing accumulated radiance
 */
SampledSpectrum PathTracing(ivec2 pixel_coords, Camera camera, inout SobolSampler sobol_sampler, SampledWavelengths lambda, float max_bounce) {
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
    Ray ray = GeneratePrimaryRay(camera, pixel_center.x, pixel_center.y, lens_sample);

    float cos_theta = dot(ray.direction, -camera.forward);
    float camera_pdf = CameraPDF(camera, ray.direction);
    float we = CameraWe(camera, cos_theta);

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

    // TODO:Temporarily disregard the medium in the air
    info = GetIntersectionInfo(hit, ray.direction, lambda);
    // Handle direct light hit
    if (is_light) {
        LightEvalInfo first_light_eval_info = MeshLightEvaluate(ray.direction, info, camera.position, lambda);
        L = Add(L, Mul(beta, first_light_eval_info.emission));

        return L;
    }
    // ========================= Process first intersection (bounce = 0) separately =========================

    // ========================= Start main path tracing loop from bounce = 1 =========================
    float mult_trans_pdf = 1.0f;
    vec3 last_position = info.position;
    vec3 world_v = -ray.direction;
    vec3 world_l = ray.direction;
    for (int bounce = 1; bounce < max_bounce; ++bounce) {
        if (info.has_medium) {
            vec2 medium_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
            MediumSampleInfo medium_sample_info = MediumDistanceSample(info, beta, medium_sample);

            if (medium_sample_info.pdf > 0.0f) {
                mult_trans_pdf *= medium_sample_info.pdf;
                beta = Mul(beta, DivFloat(medium_sample_info.transmittance, medium_sample_info.pdf));
            }

            if (medium_sample_info.scattered) {
                info = UpdateIntersectionInfoByMedium(info, medium_sample_info.distance, last_position, world_l);

            }
        }

        //// ========================= Light Sampling =========================
        //LightSampleInfo light_sample_info = MeshLightSample(sobol_sampler, info, lambda);
        //MaterialEvalInfo mat_eval_info = MaterialEvaluate(info, world_v, light_sample_info.world_l, lambda);
        //
        //// Performs visibility test from start point in given direction and distance
        //float visibility = 0.0f;
        //// Create shadow ray with epsilon offset to avoid self-intersection
        //Ray shadow_ray = SpawnShadowRay(info.position, light_sample_info.world_l, light_sample_info.distance);
        //// Traverse acceleration structure to find closest intersection
        //Hit occlusion_hit = BVHTraverse(shadow_ray);
        //// Check if any geometry was hit before reaching the target distance
        //// shadow_ray.tmax defines the maximum distance to test (typically the light distance)
        //if (occlusion_hit.distance >= shadow_ray.tmax) {
        //    visibility = 1.0f;
        //}
        //
        //if (light_sample_info.pdf > 0.0f && mat_eval_info.pdf > 0.0f) {
        //    // Le * f * cosθ / light_pdf
        //    SampledSpectrum Le_f_cos = Mul(light_sample_info.emission, mat_eval_info.bsdf_cosine);
        //    SampledSpectrum light_sampling_contribution = DivFloat(Le_f_cos, light_sample_info.pdf);
        //
        //    // mis_weight = (light_pdf^2) / (light_pdf^2 + bsdf_pdf^2)
        //    float mis_weight = PowerHeuristic(light_sample_info.pdf, mat_eval_info.pdf, 2.0f);
        //
        //    // light_sampling_contribution = β * visibility * ((mis_weight * Le * f * cosθ) / light_pdf)
        //    light_sampling_contribution = Mul(MulFloat(MulFloat(light_sampling_contribution, mis_weight), visibility), beta);
        //
        //    // Accumulate to total radiance
        //    L = Add(L, light_sampling_contribution);
        //}
        // ========================= Light Sampling =========================

        // ========================= Material Sampling =========================
        vec2 bsdf_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
        MaterialSampleInfo mat_sample_info = MaterialSample(info, world_v, bsdf_sample, lambda);

        // Cast ray in sampled direction
        Ray material_ray = SpawnRay(info.position, mat_sample_info.world_l, info.geometry_normal, 0.0f, MaxFloat);
        world_l = material_ray.direction;
        Hit new_hit = BVHTraverse(material_ray);

        // No environment light
        if (MaxFloat == new_hit.distance) {
            break;
        }

        // Check if ray hits a light source
        // If it does not hit the light source, the contribution is 0, so it can be disregarded
        if (new_hit.is_light) {
            info = GetIntersectionInfo(new_hit, material_ray.direction, lambda);
            LightEvalInfo light_eval_info = MeshLightEvaluate(mat_sample_info.world_l, info, last_position, lambda);

            if (light_eval_info.pdf > 0.0f && mat_sample_info.pdf > 0.0f) {
                // Le * f * cosθ / bsdf_pdf
                SampledSpectrum Le_f_cos = Mul(light_eval_info.emission, mat_sample_info.bsdf_cosine);
                SampledSpectrum bsdf_sampling_contribution = DivFloat(Le_f_cos, mat_sample_info.pdf);

                // mis_weight = (bsdf_pdf^2) / (bsdf_pdf^2 + light_pdf^2)
                float mis_weight = PowerHeuristic(mat_sample_info.pdf, light_eval_info.pdf, 2.0f);
                mis_weight = 1.0f;
                // bsdf_sampling_contribution = β * ((mis_weight * Le * f * cosθ) / bsdf_pdf)
                bsdf_sampling_contribution = Mul(MulFloat(bsdf_sampling_contribution, mis_weight), beta);

                // Accumulate to total radiance
                L = Add(L, bsdf_sampling_contribution);
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
        last_position = info.position;
        // ========================= Russian Roulette Wheel =========================
    }
    // ========================= Start main path tracing loop from bounce = 1 =========================

    return L;
}
// ================================================== PathTracing ==================================================

#endif // INTEGRATOR_GLSL