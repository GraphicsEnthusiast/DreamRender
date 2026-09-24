#ifndef MICROFACET_GLSL
#define MICROFACET_GLSL

#include "material/fresnel.glsl"

/**
 * @brief Computes the Smith geometry masking term for GGX distribution
 * @param v Direction vector to evaluate masking for (view or light)
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Geometry masking term value in [0, 1]
 */
float GGXG1(vec3 v, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float v_dot_n = dot(v, n);
    float v_dot_h = dot(v, h);

    // Back-facing half-vector / direction pair
    if (v_dot_n * v_dot_h <= 0.0f) {
        return 0.0f;
    }

    // Avoid division by zero at normal incidence
    if (abs(v_dot_n - 1.0f) < 1e-6f) {
        return 1.0f;
    }

    if (alpha_u == alpha_v) {
        // Isotropic
        float v_dot_n_2 = v_dot_n * v_dot_n;
        float tan_v_n_2 = (1.0f - v_dot_n_2) / v_dot_n_2;
        float alpha_2 = alpha_u * alpha_u;

        return 2.0f / (1.0f + sqrt(1.0f + alpha_2 * tan_v_n_2));
    }
    else {
        // Anisotropic
        vec3 local_dir = ToLocalFromUp(v, n);   // assumes n maps to +Z
        float x_alpha = alpha_u * local_dir.x;
        float y_alpha = alpha_v * local_dir.y;
        float xy_alpha_2 = x_alpha * x_alpha + y_alpha * y_alpha;
        float tan_v_n_alpha_2 = xy_alpha_2 / (local_dir.z * local_dir.z);

        return 2.0f / (1.0f + sqrt(1.0f + tan_v_n_alpha_2));
    }
}

/**
 * @brief Computes the combined Smith geometry term for view and light directions
 * @param v View direction vector
 * @param l Light direction vector
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Product of masking terms for view and light directions
 */
float GGXG2(vec3 v, vec3 l, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    return GGXG1(v, h, n, alpha_u, alpha_v) *
           GGXG1(l, h, n, alpha_u, alpha_v);
}

/**
 * @brief Computes the GGX normal distribution function
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Microfacet normal distribution density
 */
float GGXD(vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float h_dot_n = dot(h, n);
    if (h_dot_n <= 0.0f) {
        return 0.0f;
    }

    float h_dot_n_2 = h_dot_n * h_dot_n;
    float tan_theta_2 = (1.0f - h_dot_n_2) / h_dot_n_2;
    float alpha_2 = alpha_u * alpha_v;

    if (alpha_u == alpha_v) {
        // Isotropic — D = α² / (π cos⁴θ (α² + tan²θ)²)
        float denom = PI * h_dot_n_2 * h_dot_n_2 *
                      (alpha_2 + tan_theta_2) * (alpha_2 + tan_theta_2);
        return alpha_2 / denom;
    }
    else {
        // Anisotropic
        vec3 local_dir = ToLocalFromUp(h, n);
        float x_alpha = local_dir.x / alpha_u;
        float y_alpha = local_dir.y / alpha_v;
        float term = x_alpha * x_alpha +
                     y_alpha * y_alpha +
                     local_dir.z * local_dir.z;
        float denom = PI * alpha_2 * term * term;

        return 1.0f / denom;
    }
}

/**
 * @brief Computes the visible normal distribution function D*G1*(V·H)/(N·V)
 * @param v View direction vector
 * @param h Half-vector (normalized vector halfway between view and light)
 * @param n Surface normal vector
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @return Visible normal distribution density value
 */
float GGXDV(vec3 v, vec3 h, vec3 n, float alpha_u, float alpha_v) {
    float n_dot_v = dot(n, v);
    if (n_dot_v <= 1e-6f) {
        return 0.0f;
    }

    float d = GGXD(h, n, alpha_u, alpha_v);
    float g1 = GGXG1(v, h, n, alpha_u, alpha_v);
    float v_dot_h = dot(v, h);

    return g1 * v_dot_h * d / n_dot_v;
}

/**
 * @brief Samples the visible GGX distribution (Heitz 2018) in local space
 * @param n Surface normal vector
 * @param ve View direction in world space
 * @param alpha_u Roughness in tangent direction
 * @param alpha_v Roughness in bitangent direction
 * @param sample_xy 2D random sample in [0,1]^2
 * @return Sampled half-vector in local space where the normal is +Z
 */
vec3 GGXSampleVisible(vec3 n, vec3 ve, float alpha_u, float alpha_v, vec2 sample_xy) {
    // Convert to the local frame where n maps to +Z
    vec3 v = ToLocalFromUp(ve, n);

    // Stretch the view direction
    vec3 vh = normalize(vec3(alpha_u * v.x, alpha_v * v.y, v.z));

    // Orthonormal basis (special case for axis-aligned view)
    float len2 = vh.x * vh.x + vh.y * vh.y;
    vec3 t1 = len2 > 0.0f ? vec3(-vh.y, vh.x, 0.0f) * inversesqrt(len2)
                          : vec3(1.0f, 0.0f, 0.0f);
    vec3 t2 = cross(vh, t1);

    // Parameterize projected area
    float u = sample_xy.x;
    float v_sample = sample_xy.y;
    float r = sqrt(u);
    float phi = v_sample * 2.0f * PI;
    float t1_coord = r * cos(phi);
    float t2_coord = r * sin(phi);

    float s = 0.5f * (1.0f + vh.z);
    t2_coord = (1.0f - s) * sqrt(1.0f - t1_coord * t1_coord) + s * t2_coord;

    // Reproject onto hemisphere
    vec3 nh = t1_coord * t1 + t2_coord * t2 +
              sqrt(max(0.0f, 1.0f - t1_coord * t1_coord - t2_coord * t2_coord)) * vh;

    // Transform back to ellipsoid configuration
    vec3 local_h = normalize(vec3(alpha_u * nh.x,
                                  alpha_v * nh.y,
                                  max(0.0f, nh.z)));

    return local_h;
}

#endif // MICROFACET_GLSL