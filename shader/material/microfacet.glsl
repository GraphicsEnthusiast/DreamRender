#ifndef MICROFACET_GLSL
#define MICROFACET_GLSL

#include "material/fresnel.glsl"

/**
 * @brief Computes the Smith geometry masking term for GGX distribution
 * @param world_in View direction vector
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Geometry masking term value
 */
float GGXG1(vec3 world_in, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float cos_v_n = dot(world_in, n);
    
    // Early exit for back-facing directions
    if (cos_v_n * dot(world_in, h) <= 0.0f) {
        return 0.0f;
    }
    
    // Handle near-normal incidence
    if (abs(cos_v_n - 1.0f) < Epsilon) {
        return 1.0f;
    }
    
    if (alpha_u == alpha_v) {
        // Isotropic case
        float cos_v_n_2 = cos_v_n * cos_v_n;
        float tan_v_n_2 = (1.0f - cos_v_n_2) / cos_v_n_2;
        float alpha_2 = alpha_u * alpha_u;
        
        return 2.0f / (1.0f + sqrt(1.0f + alpha_2 * tan_v_n_2));
    }
    else {
        // Anisotropic case
        vec3 local_dir = ToLocalFromUp(world_in, n);
        float xy_alpha_2 = pow(alpha_u * local_dir.x, 2.0f) + pow(alpha_v * local_dir.y, 2.0f);
        float tan_v_n_alpha_2 = xy_alpha_2 / (local_dir.z * local_dir.z);
        
        return 2.0f / (1.0f + sqrt(1.0f + tan_v_n_alpha_2));
    }
}

/**
 * @brief Computes the full Smith geometry function for GGX distribution
 * @param world_in View direction vector
 * @param world_out Light direction vector
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Full geometry function value
 */
float GGXG2(vec3 world_in, vec3 world_out, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float G1_v = GGXG1(world_in, h, n, alpha_u, alpha_v);
    float G1_l = GGXG1(world_out, h, n, alpha_u, alpha_v);
    
    return G1_v * G1_l;
}

/**
 * @brief Computes the GGX normal distribution function (NDF)
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Distribution value
 */
float GGXD(vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float cos_theta = dot(h, n);
    if (cos_theta <= 0.0f) {
        return 0.0f;
    }
    
    float cos_theta_2 = cos_theta * cos_theta;
    float tan_theta_2 = (1.0f - cos_theta_2) / cos_theta_2;
    float alpha_2 = alpha_u * alpha_v;
    
    if (alpha_u == alpha_v) {
        // Isotropic case
        float denom = PI * cos_theta_2 * cos_theta_2 * pow(alpha_2 + tan_theta_2, 2.0f);
        
        return alpha_2 / denom;
    }
    else {
        // Anisotropic case
        vec3 local_dir = ToLocalFromUp(h, n);
        float denom = PI * alpha_2 * pow(pow(local_dir.x / alpha_u, 2.0f) + pow(local_dir.y / alpha_v, 2.0f) + local_dir.z * local_dir.z, 2.0f);
        
        return 1.0f / denom;
    }
}

/**
 * @brief Computes the visible normal distribution for GGX distribution
 * @param world_in View direction vector
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Visible normal distribution value
 */
float GGXDV(vec3 world_in, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float D = GGXD(h, n, alpha_u, alpha_v);
    float G1 = GGXG1(world_in, h, n, alpha_u, alpha_v);
    float dot_v_h = dot(world_in, h);
    float dot_n_v = dot(n, world_in);
    
    if (dot_n_v <= Epsilon) {
        return 0.0f;
    }
    
    return G1 * dot_v_h * D / dot_n_v;
}

/**
 * @brief Samples a visible normal from the GGX distribution
 * @param n Surface normal vector
 * @param ve View direction vector in world space
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @param sample_xy Random sample_xy in [0,1]^2
 * @return Sampled half-vector
 */
vec3 GGXSampleVisible(vec3 n, vec3 ve, float alpha_u, float alpha_v, vec2 sample_xy) {
    // Transform view direction to local space
    vec3 v_local = ToLocalFromUp(ve, n);
    
    // Stretch the view direction
    vec3 vh = normalize(vec3(alpha_u * v_local.x, alpha_v * v_local.y, v_local.z));
    
    // Section 4.1: Orthonormal basis (with special case if cross product is zero)
    float len2 = vh.x * vh.x + vh.y * vh.y;
    vec3 T1 = len2 > 0.0f ? vec3(-vh.y, vh.x, 0.0f) * inversesqrt(len2) : vec3(1.0f, 0.0f, 0.0f);
    vec3 T2 = cross(vh, T1);
    
    // Section 4.2: Parameterization of the projected area
    float u = sample_xy.x;
    float v = sample_xy.y;
    float r = sqrt(u);
    float phi = v * 2.0f * PI;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + vh.z);
    t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
    
    // Section 4.3: Reprojection onto hemisphere
    vec3 nh = t1 * T1 + t2 * T2 + sqrt(max(0.0f, 1.0f - t1 * t1 - t2 * t2)) * vh;
    
    // Section 3.4: Transforming the normal back to the ellipsoid configuration
    vec3 h_local = normalize(vec3(alpha_u * nh.x, alpha_v * nh.y, max(0.0f, nh.z)));
    
    return h_local;
}

#endif // MICROFACET_GLSL