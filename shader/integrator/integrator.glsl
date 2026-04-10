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
bool IsBoundaryMaterial(IntersectionInfo info) {
    return MaterialType_Boundary == info.material.type;
}

/**
 * @brief Checks if hit represents nothing (no intersection)
 * @param hit Hit information
 * @return Boolean indicating if no intersection occurred
 */
bool HitNothing(Hit hit) {
    return MaxFloat == hit.distance;
}

/**
 * @brief Checks if hit represents a light source
 * @param hit Hit information
 * @return Boolean indicating if intersection is with a light source
 */
bool HitLight(Hit hit) {
    return hit.is_light;
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

// =================================================== Path Tracing ===================================================
/**
 * @brief Computes visibility from shading point to light source, accounting for medium transmittance
 * @param position Starting position of the ray (shading point)
 * @param light_sample_info Information about the sampled light point
 * @param beta Current path throughput (used for medium evaluation)
 * @param lambda Sampled wavelengths for spectral rendering
 * @param mult_trans_pdf Accumulated PDF of the transmittance sampling (product of segment PDFs)
 * @param sobol_sampler Sobol sampler for random number generation
 * @return SampledSpectrum representing the visibility (including transmittance)
 */
SampledSpectrum DirectLightVisibility(vec3 position, LightSampleInfo light_sample_info, SampledSpectrum beta, 
                                 SampledWavelengths lambda, out float mult_trans_pdf, inout SobolSampler sobol_sampler) {
    
    float shadow_distance_remaining = light_sample_info.distance;
    vec3 shadow_origin = position;
    SampledSpectrum visibility = SampledSpectrumNewFloat(1.0f);
    mult_trans_pdf = 1.0f;
    
    while (true) {
        Ray shadow_ray = SpawnShadowRay(shadow_origin, light_sample_info.world_l, shadow_distance_remaining);
        Hit occlusion_hit = BVHTraverse(shadow_ray);
        
        // If no intersection, we've reached the light
        if (occlusion_hit.distance >= shadow_ray.tmax) {
            if (light_sample_info.has_medium) {
                IntersectionInfo medium_info;
                medium_info.has_medium = true;
                medium_info.medium = light_sample_info.out_medium;
                medium_info = UpdateIntersectionInfoByMedium(medium_info, shadow_distance_remaining, shadow_origin, light_sample_info.world_l);
                
                MediumEvalInfo medium_eval_info = MediumDistanceEvaluate(medium_info, beta, false);
                
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
            IntersectionInfo occlusion_info = GetIntersectionInfo(occlusion_hit, shadow_ray.direction, lambda);
            
            // Check if it's a boundary material
            if (!IsBoundaryMaterial(occlusion_info)) {
                // Not a boundary material, occluded
                visibility = MulFloat(visibility, 0.0f);

                break;
            }
            
            // Handle boundary material
            if (occlusion_info.has_medium) {
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
 * @return SampledSpectrum representing accumulated power
 */
SampledSpectrum PathTracing(ivec2 pixel_coords, Camera camera, inout SobolSampler sobol_sampler, 
                            SampledWavelengths lambda, float max_bounce) {
    vec2 pixel_center = vec2(pixel_coords) + vec2(0.5f);
    vec2 filter_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
    FilterSample filter_sample_info = GaussianFilterSample(0.5f, filter_sample);
    pixel_center += filter_sample_info.offset;

    SampledSpectrum beta = SampledSpectrumNewFloat(1.0f);
    SampledSpectrum radiance = SampledSpectrumNewFloat(0.0f);
    
    vec2 lens_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
    Ray ray = GeneratePrimaryRay(camera, pixel_center.x, pixel_center.y, lens_sample);

    float cos_theta = dot(ray.direction, -camera.forward);
    float camera_pdf = CameraPDF(camera, ray.direction);
    float we = CameraWe(camera, ray.direction);

    if (camera_pdf > 0.0f) {
        float camera_sampling_weight = we * cos_theta / camera_pdf;
        beta = MulFloat(beta, camera_sampling_weight);
    }

    if (filter_sample_info.pdf > 0.0f) {
        beta = MulFloat(beta, filter_sample_info.weight / filter_sample_info.pdf);
    }

    vec3 world_v = -ray.direction;
    vec3 world_l = ray.direction;
    vec3 last_position = ray.origin;
    float bsdf_phase_pdf = 0.0f;  // bsdf or phase pdf
    float mult_trans_pdf = 1.0f;

    for (int bounce = 0; bounce < max_bounce; ++bounce) {
        Hit hit = BVHTraverse(ray);
        IntersectionInfo info = GetIntersectionInfo(hit, world_l, lambda);
        
        // Handle medium interaction
        bool is_medium_scattered = false;
        if (info.has_medium) {
            // ========================= Medium Distance Sampling =========================
            // Determine whether the next ray bounce is a surface bounce or a volume bounce
            vec2 medium_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
            MediumSampleInfo medium_sample_info = MediumDistanceSample(info, beta, medium_sample);
            
            if (medium_sample_info.pdf > 0.0f) {
                beta = Mul(beta, DivFloat(medium_sample_info.transmittance, medium_sample_info.pdf));
                mult_trans_pdf *= medium_sample_info.pdf;
            }
            // ========================= Medium Distance Sampling =========================
            
            if (medium_sample_info.scattered) {
                is_medium_scattered = true;

                // Update to scattering point
                info = UpdateIntersectionInfoByMedium(info, medium_sample_info.distance, last_position, world_l);
                
                // ========================= Volume Light Sampling =========================
                // Sampling direct illumination within the volume
                float mult_trans_pdf_nee = 1.0f;
                LightSampleInfo light_sample_info = MeshLightSample(sobol_sampler, info, lambda);
                
                // Compute visibility and medium transmittance
                SampledSpectrum visibility = DirectLightVisibility(
                    info.position, light_sample_info, beta, lambda, 
                    mult_trans_pdf_nee, sobol_sampler
                );
                
                // Evaluate phase function
                PhaseEvalInfo phase_eval_info = PhaseEvaluate(info, world_v, light_sample_info.world_l);
                
                if (light_sample_info.pdf > 0.0f && phase_eval_info.pdf > 0.0f && mult_trans_pdf_nee > 0.0f) {
                    float mis_weight = PowerHeuristic(light_sample_info.pdf, phase_eval_info.pdf * mult_trans_pdf_nee, 2.0f);

                    // L_light = β * visibility * mis_weight * Le * phase / light_pdf
                    SampledSpectrum L_light = Mul(MulFloat(Mul(beta, visibility), mis_weight),
                        DivFloat(Mul(light_sample_info.emission, phase_eval_info.phase), light_sample_info.pdf));
                    
                    radiance = Add(radiance, L_light);
                }
                // ========================= Volume Light Sampling =========================
                
                // ========================= Phase Sampling =========================
                // Sample phase function direction
                vec2 phase_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
                PhaseSampleInfo phase_sample_info = PhaseSample(info, world_v, phase_sample);
                
                if (phase_sample_info.pdf > 0.0f) {
                    beta = Mul(beta, DivFloat(phase_sample_info.phase, phase_sample_info.pdf));
                    world_l = phase_sample_info.world_l;
                    bsdf_phase_pdf = phase_sample_info.pdf;
                }
                // ========================= Phase Sampling =========================
            }
        }
        
        // If not scattered in medium
        if (!info.has_medium || !is_medium_scattered) {
            // Handle hitting a light source
            if (HitLight(hit)) {
                // ========================= BSDF or Phase Sampling =========================
                float mis_weight = 1.0f;
                LightEvalInfo light_eval_info = MeshLightEvaluate(world_l, info, last_position, lambda);
                
                if (0 != bounce) {
                    mis_weight = PowerHeuristic(bsdf_phase_pdf * mult_trans_pdf, light_eval_info.pdf, 2.0f);
                }
                
                if (light_eval_info.pdf > 0.0f && bsdf_phase_pdf > 0.0f && mult_trans_pdf > 0.0f) {
                    // L_material_or_medium = β * visibility * mis_weight * Le * (bsdf * cos or phase) / bsdf_phase_pdf                 
                    //                      = β' * mis_weight * Le
                    // β' = β * visibility * (bsdf * cos or phase) / bsdf_phase_pdf (Already calculated in the previous bounce)
                    SampledSpectrum L_material_or_medium = MulFloat(Mul(beta, light_eval_info.emission), mis_weight);

                    radiance = Add(radiance, L_material_or_medium);
                }
                // ========================= BSDF or Phase Sampling =========================
                
                break;
            }
            // Handle hitting nothing (environment)
            else if (HitNothing(hit)) {
                // We don't have environment light, so just break
                break;
            }
            // Handle hitting medium boundary
            else if (IsBoundaryMaterial(info)) {
                world_v = -world_l;
                last_position = info.position;
                ray = SpawnRay(info.position, world_l, info.geometry_normal, 0.0f, MaxFloat);
                bounce--;  // Don't count this as a bounce

                continue;
            }
            // Handle regular surface interaction
            else {
                // ========================= Surface Light Sampling =========================
                // Sampling direct illumination on the surface
                float mult_trans_pdf_nee = 1.0f;
                LightSampleInfo light_sample_info = MeshLightSample(sobol_sampler, info, lambda);
                
                // Compute visibility and medium transmittance
                SampledSpectrum visibility = DirectLightVisibility(
                    info.position, light_sample_info, beta, lambda, 
                    mult_trans_pdf_nee, sobol_sampler
                );
                
                // Evaluate BSDF
                MaterialEvalInfo mat_eval_info = MaterialEvaluate(info, world_v, light_sample_info.world_l, lambda);
                
                if (light_sample_info.pdf > 0.0f && mat_eval_info.pdf > 0.0f && mult_trans_pdf_nee > 0.0f) {
                    float mis_weight = PowerHeuristic(light_sample_info.pdf, mat_eval_info.pdf * mult_trans_pdf_nee, 2.0f);
                    // L_light = β * visibility * mis_weight * Le * bsdf * cos / light_pdf
                    SampledSpectrum L_light = Mul(MulFloat(Mul(beta, visibility), mis_weight),
                        DivFloat(Mul(light_sample_info.emission, mat_eval_info.bsdf_cosine), light_sample_info.pdf));
                    
                    radiance = Add(radiance, L_light);
                }
                // ========================= Surface Light Sampling =========================
                

                // ========================= BSDF Sampling =========================
                // Sample BSDF direction
                vec2 bsdf_sample = vec2(SobolSamplerGet1(sobol_sampler), SobolSamplerGet1(sobol_sampler));
                MaterialSampleInfo mat_sample_info = MaterialSample(info, world_v, bsdf_sample, lambda);
                
                if (mat_sample_info.pdf > 0.0f) {
                    beta = Mul(beta, DivFloat(mat_sample_info.bsdf_cosine, mat_sample_info.pdf));
                    world_l = mat_sample_info.world_l;
                    bsdf_phase_pdf = mat_sample_info.pdf;
                }
                // ========================= BSDF Sampling =========================
            }
        }
        
        // Russian roulette
        if (bounce > 3 && Max(beta) < 0.1f) {
            float q = max(0.05f, 1.0f - Max(beta));
            if (SobolSamplerGet1(sobol_sampler) < q) {
                break;
            }
            beta = DivFloat(beta, 1.0f - q);
        }

        // Update for next bounce
        world_v = -world_l;
        mult_trans_pdf = 1.0f;
        last_position = info.position;
        ray = SpawnRay(info.position, world_l, info.geometry_normal, 0.0f, MaxFloat);
    }
    
    return radiance;
}
// =================================================== Path Tracing ===================================================

#endif // INTEGRATOR_GLSL