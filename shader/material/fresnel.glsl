#ifndef FRESNEL_GLSL
#define FRESNEL_GLSL

#include "spectrum/spectrum.glsl"

/**
 * @brief Vectorized Fresnel reflectance for conductors.
 * @param world_in View direction vector.
 * @param h Half-vector (normalized halfway vector between view and light).
 * @param eta_r Real part of the complex refractive index (n).
 * @param eta_i Imaginary part of the complex refractive index (k).
 * @return Sampled reflectance spectrum.
 */
SampledSpectrum FresnelConductor(vec3 world_in, vec3 h, SampledSpectrum eta_r, SampledSpectrum eta_i) {
    float cos_v_n = dot(world_in, h);
    float cos_v_n_2 = cos_v_n * cos_v_n;
    float sin_v_n_2 = 1.0f - cos_v_n_2;
    float sin_v_n_4 = sin_v_n_2 * sin_v_n_2;

    SampledSpectrum eta_r_2 = Mul(eta_r, eta_r);
    SampledSpectrum eta_i_2 = Mul(eta_i, eta_i);

    // temp_1 = eta_r^2 - eta_i^2 - sin_v_n_2
    SampledSpectrum temp_1 = Sub(Sub(eta_r_2, eta_i_2),
                                 SampledSpectrumNewFloat(sin_v_n_2));

    // a_2_pb_2 = sqrt(temp_1^2 + 4 * eta_i^2 * eta_r^2)
    SampledSpectrum temp_1_2 = Mul(temp_1, temp_1);
    SampledSpectrum cross_term = MulFloat(Mul(eta_i_2, eta_r_2), 4.0f);
    SampledSpectrum a_2_pb_2 = Add(temp_1_2, cross_term);

    // a_2_pb_2 = sqrt(max(a_2_pb_2, 0)) component-wise
    a_2_pb_2 = Sqrt(GEFloat(a_2_pb_2, 0.0f));

    // a = sqrt(max(0.5 * (a_2_pb_2 + temp_1), 0))
    SampledSpectrum a = Add(a_2_pb_2, temp_1);
    a = MulFloat(a, 0.5f);
    a = Sqrt(GEFloat(a, 0.0f));

    SampledSpectrum term_1 = Add(a_2_pb_2, SampledSpectrumNewFloat(sin_v_n_2));
    SampledSpectrum term_2 = MulFloat(a, 2.0f * cos_v_n);
    SampledSpectrum term_3 = Add(
        MulFloat(a_2_pb_2, cos_v_n_2),
        SampledSpectrumNewFloat(sin_v_n_4)
    );
    SampledSpectrum term_4 = MulFloat(term_2, sin_v_n_2);

    // r_s = (term_1 - term_2) / (term_1 + term_2)
    SampledSpectrum r_s = Div(
        Sub(term_1, term_2),
        Add(term_1, term_2)
    );

    // r_p = r_s * (term_3 - term_4) / (term_3 + term_4)
    SampledSpectrum r_p = Mul(
        r_s,
        Div(
            Sub(term_3, term_4),
            Add(term_3, term_4)
        )
    );

    // Average s and p polarization reflectances
    return MulFloat(Add(r_s, r_p), 0.5f);
}

/**
 * @brief Vectorized average Fresnel reflectance for conductors.
 * @param eta Real part of the complex refractive index (n).
 * @param k Imaginary part of the complex refractive index (k).
 * @return Sampled average reflectance spectrum.
 */
SampledSpectrum AverageFresnelConductor(SampledSpectrum eta, SampledSpectrum k) {
    SampledSpectrum eta_minus_1 = Sub(eta, SampledSpectrumNewFloat(1.0f));
    SampledSpectrum eta_plus_1 = Add(eta, SampledSpectrumNewFloat(1.0f));
    SampledSpectrum k_sq = Mul(k, k);

    // Normal-incidence reflectivity
    SampledSpectrum reflectivity = Div(
        Add(Mul(eta_minus_1, eta_minus_1), k_sq),
        Add(Mul(eta_plus_1, eta_plus_1), k_sq)
    );

    // Edge tint term
    SampledSpectrum sqrt_reflectivity = Sqrt(reflectivity);
    SampledSpectrum temp1 = Add(SampledSpectrumNewFloat(1.0f), sqrt_reflectivity);
    SampledSpectrum temp2 = Sub(SampledSpectrumNewFloat(1.0f), sqrt_reflectivity);
    SampledSpectrum temp3 = Div(
        Sub(SampledSpectrumNewFloat(1.0f), reflectivity),
        Add(SampledSpectrumNewFloat(1.0f), reflectivity)
    );

    SampledSpectrum numerator = Sub(temp1, Mul(eta, temp2));
    SampledSpectrum denominator = Sub(temp1, Mul(temp3, temp2));
    SampledSpectrum edgetint = Div(numerator, denominator);

    // Polynomial combination
    SampledSpectrum constant = SampledSpectrumNewFloat(0.087237f);
    SampledSpectrum term1 = MulFloat(edgetint, 0.0230685f);
    SampledSpectrum term2 = MulFloat(Mul(edgetint, edgetint), -0.0864902f);
    SampledSpectrum term3 = MulFloat(Mul(Mul(edgetint, edgetint), edgetint), 0.0774594f);
    SampledSpectrum term4 = MulFloat(reflectivity, 0.782654f);
    SampledSpectrum term5 = MulFloat(Mul(reflectivity, reflectivity), -0.136432f);
    SampledSpectrum term6 = MulFloat(Mul(Mul(reflectivity, reflectivity), reflectivity), 0.278708f);
    SampledSpectrum term7 = Mul(MulFloat(edgetint, 0.19744f), reflectivity);
    SampledSpectrum term8 = Mul(Mul(MulFloat(edgetint, 0.0360605f), edgetint), reflectivity);
    SampledSpectrum term9 = Mul(Mul(MulFloat(edgetint, -0.2586f), reflectivity), reflectivity);

    SampledSpectrum result = Add(constant, term1);
    result = Add(result, term2);
    result = Add(result, term3);
    result = Add(result, term4);
    result = Add(result, term5);
    result = Add(result, term6);
    result = Add(result, term7);
    result = Add(result, term8);
    result = Add(result, term9);

    return result;
}

/**
 * @brief Exact Fresnel reflectance for dielectric materials.
 * @param world_in View direction vector.
 * @param h Surface normal or half-vector depending on the use case.
 * @param eta_inv Reciprocal of the relative index of refraction,
 * @return Fresnel reflectance in [0, 1].
 */
float FresnelDielectric(vec3 world_in, vec3 h, float eta_inv) {
    float cos_theta_i = abs(dot(world_in, h));
    float sin_theta_i_2 = 1.0f - cos_theta_i * cos_theta_i;
    float cos_theta_t_2 = 1.0f - eta_inv * eta_inv * sin_theta_i_2;

    if (cos_theta_t_2 <= 0.0f) {
        return 1.0f; // Total internal reflection
    }

    float cos_theta_t = sqrt(cos_theta_t_2);
    float Rs_sqrt = (eta_inv * cos_theta_i - cos_theta_t) /
                    (eta_inv * cos_theta_i + cos_theta_t);
    float Rp_sqrt = (cos_theta_i - eta_inv * cos_theta_t) /
                    (cos_theta_i + eta_inv * cos_theta_t);

    return (Rs_sqrt * Rs_sqrt + Rp_sqrt * Rp_sqrt) / 2.0f;
}

/**
 * @brief Average Fresnel reflectance for dielectric materials.
 * @param eta Relative index of refraction (eta_transmitted / eta_incident).
 * @return Average Fresnel reflectance in [0, 1].
 */
float AverageFresnelDielectric(float eta) {
    if (eta < 1.0f) {
        // Egan and Hilgeman (1973) fit.
        // Maximum relative error:
        //   0.1% in [1.0, 1.5]
        //   0.6% in [1.5, 2.0]
        //   9.5% in [2.0, 5.0]
        return -1.4399f * (eta * eta) + 0.7099f * eta + 0.6681f + 0.0636f / eta;
    }
    else {
        // d'Eon and Irving (2011) fit.
        // Maximum relative error:
        //   0.1% in [1.0, 2.0]
        //   0.2% in [2.0, 10.0]
        float inv_eta = 1.0f / eta;
        float inv_eta_2 = inv_eta * inv_eta;
        float inv_eta_3 = inv_eta_2 * inv_eta;
        float inv_eta_4 = inv_eta_3 * inv_eta;
        float inv_eta_5 = inv_eta_4 * inv_eta;

        return 0.919317f - 3.4793f * inv_eta +
               6.75335f * inv_eta_2 - 7.80989f * inv_eta_3 +
               4.98554f * inv_eta_4 - 1.36881f * inv_eta_5;
    }
}

#endif // FRESNEL_GLSL