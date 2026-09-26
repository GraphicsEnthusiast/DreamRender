#ifndef FRESNEL_GLSL
#define FRESNEL_GLSL

#include "spectrum/spectrum.glsl"

/**
 * @brief Computes Fresnel reflectance for conductors using complex refractive indices.
 * @param v View direction vector.
 * @param h Half-vector (or surface normal for normal incidence).
 * @param eta_r Real part of complex index of refraction (n) per wavelength.
 * @param eta_i Imaginary part (k) per wavelength.
 * @return Sampled Fresnel reflectance spectrum.
 */
SampledSpectrum FresnelConductor(vec3 v, vec3 h, SampledSpectrum eta_r, SampledSpectrum eta_i) {
    float v_dot_h = dot(v, h);
    float v_dot_h_2 = v_dot_h * v_dot_h;
    float sin_theta_2 = 1.0f - v_dot_h_2;
    float sin_theta_4 = sin_theta_2 * sin_theta_2;

    // eta_r^2 and eta_i^2 (component-wise)
    SampledSpectrum eta_r_2 = Mul(eta_r, eta_r);
    SampledSpectrum eta_i_2 = Mul(eta_i, eta_i);

    // temp_1 = eta_r^2 - eta_i^2 - sin^2(theta)
    SampledSpectrum temp_1 = Sub(Sub(eta_r_2, eta_i_2),
                                 SampledSpectrumNewFloat(sin_theta_2));

    // a_2_pb_2 = sqrt(temp_1^2 + 4 * eta_i^2 * eta_r^2)
    SampledSpectrum temp_1_2 = Mul(temp_1, temp_1);
    SampledSpectrum cross_term = MulFloat(Mul(eta_i_2, eta_r_2), 4.0f);
    SampledSpectrum a_2_pb_2 = Add(temp_1_2, cross_term);
    a_2_pb_2 = Sqrt(GEFloat(a_2_pb_2, 0.0f)); // Numerically safe, though always >= 0

    // a = sqrt(max(0.5 * (a_2_pb_2 + temp_1), 0))
    SampledSpectrum a = Add(a_2_pb_2, temp_1);
    a = MulFloat(a, 0.5f);
    a = Sqrt(GEFloat(a, 0.0f));

    // Common terms for s and p reflectance
    SampledSpectrum term_1 = Add(a_2_pb_2, SampledSpectrumNewFloat(sin_theta_2));
    SampledSpectrum term_2 = MulFloat(a, 2.0f * v_dot_h);
    SampledSpectrum term_3 = Add(MulFloat(a_2_pb_2, v_dot_h_2),
                                 SampledSpectrumNewFloat(sin_theta_4));
    SampledSpectrum term_4 = MulFloat(term_2, sin_theta_2);

    // Per-polarization reflectances
    SampledSpectrum r_s = Div(Sub(term_1, term_2),
                              Add(term_1, term_2));
    SampledSpectrum r_p = Mul(r_s,
                              Div(Sub(term_3, term_4),
                                  Add(term_3, term_4)));

    // Unpolarized average
    return MulFloat(Add(r_s, r_p), 0.5f);
}

/**
 * @brief Computes the average Fresnel reflectance for conductors using complex refractive indices.
 * @param eta_r Real part of complex index of refraction (n) per wavelength.
 * @param eta_i Imaginary part (k) per wavelength.
 * @return Sampled average Fresnel reflectance spectrum.
 */
SampledSpectrum FresnelAverageConductor(SampledSpectrum eta_r, SampledSpectrum eta_i) {
    // Compute reflectivity and edge tint per wavelength
    SampledSpectrum reflectivity = SampledSpectrumNewFloat(0.0f);
    SampledSpectrum edgetint = SampledSpectrumNewFloat(0.0f);
    
    // temp1, temp2, temp3 are scalar temporaries per wavelength
    // Using SampledSpectrum operations for component-wise computation
    SampledSpectrum eta_minus_1 = Sub(eta_r, SampledSpectrumNewFloat(1.0f));
    SampledSpectrum eta_plus_1 = Add(eta_r, SampledSpectrumNewFloat(1.0f));
    
    // reflectivity = ((eta-1)^2 + k^2) / ((eta+1)^2 + k^2)
    SampledSpectrum num = Add(Mul(eta_minus_1, eta_minus_1), Mul(eta_i, eta_i));
    SampledSpectrum den = Add(Mul(eta_plus_1, eta_plus_1), Mul(eta_i, eta_i));
    reflectivity = Div(num, den);
    
    // temp1 = 1 + sqrt(reflectivity)
    SampledSpectrum temp1 = Add(SampledSpectrumNewFloat(1.0f), Sqrt(reflectivity));
    
    // temp2 = 1 - sqrt(reflectivity)
    SampledSpectrum temp2 = Sub(SampledSpectrumNewFloat(1.0f), Sqrt(reflectivity));
    
    // temp3 = (1 - reflectivity) / (1 + reflectivity)
    SampledSpectrum temp3 = Div(Sub(SampledSpectrumNewFloat(1.0f), reflectivity),
                                Add(SampledSpectrumNewFloat(1.0f), reflectivity));
    
    // edgetint = (temp1 - eta * temp2) / (temp1 - temp3 * temp2)
    SampledSpectrum num_et = Sub(temp1, Mul(eta_r, temp2));
    SampledSpectrum den_et = Sub(temp1, Mul(temp3, temp2));
    edgetint = Div(num_et, den_et);
    
    // Polynomial fit for average Fresnel reflectance
    SampledSpectrum result = SampledSpectrumNewFloat(0.087237f);
    result = Add(result, MulFloat(edgetint, 0.0230685f));
    result = Sub(result, MulFloat(Mul(edgetint, edgetint), 0.0864902f));
    result = Add(result, MulFloat(Mul(Mul(edgetint, edgetint), edgetint), 0.0774594f));
    result = Add(result, MulFloat(reflectivity, 0.782654f));
    result = Sub(result, MulFloat(Mul(reflectivity, reflectivity), 0.136432f));
    result = Add(result, MulFloat(Mul(Mul(reflectivity, reflectivity), reflectivity), 0.278708f));
    result = Add(result, MulFloat(Mul(edgetint, reflectivity), 0.19744f));
    result = Add(result, MulFloat(Mul(Mul(edgetint, edgetint), reflectivity), 0.0360605f));
    result = Sub(result, MulFloat(Mul(edgetint, Mul(reflectivity, reflectivity)), 0.2586f));
    
    return result;
}

/**
 * @brief Computes Fresnel reflectance for dielectrics.
 * @param v View direction vector.
 * @param h Half-vector (or surface normal).
 * @param eta_inv Inverse of relative refractive index (eta_inv = eta_out / eta_in).
 * @return Fresnel reflectance value.
 */
float FresnelDielectric(vec3 v, vec3 h, float eta_inv) {
    float cos_theta_i = abs(dot(v, h));
    float cos_theta_t_2 = 1.0f - eta_inv * eta_inv * (1.0f - cos_theta_i * cos_theta_i);
    
    if (cos_theta_t_2 <= 0.0f) {
        // Total internal reflection
        return 1.0f;
    }
    else {
        float cos_theta_t = sqrt(cos_theta_t_2);
        float Rs_sqrt = (eta_inv * cos_theta_i - cos_theta_t) / (eta_inv * cos_theta_i + cos_theta_t);
        float Rp_sqrt = (cos_theta_i - eta_inv * cos_theta_t) / (cos_theta_i + eta_inv * cos_theta_t);
        
        // Unpolarized average
        return (Rs_sqrt * Rs_sqrt + Rp_sqrt * Rp_sqrt) / 2.0f;
    }
}

/**
 * @brief Computes the average Fresnel reflectance for dielectrics using polynomial fits.
 * @param eta Relative refractive index.
 * @return Average Fresnel reflectance value.
 */
float FresnelAverageDielectric(float eta) {
    if (eta < 1.0f) {
        // Fit by Egan and Hilgeman (1973). Works reasonably well for
        // "normal" IOR values (<2).
        // Max rel. error in 1.0 - 1.5 : 0.1%
        // Max rel. error in 1.5 - 2   : 0.6%
        // Max rel. error in 2.0 - 5   : 9.5%
        return -1.4399f * (eta * eta) + 0.7099f * eta + 0.6681f + 0.0636f / eta;
    }
    else {
        // Fit by d'Eon and Irving (2011)
        // Maintains good accuracy even for unusual IOR values.
        // Max rel. error in 1.0 - 2.0   : 0.1%
        // Max rel. error in 2.0 - 10.0  : 0.2%
        float inv_eta = 1.0f / eta;
        float inv_eta_2 = inv_eta * inv_eta;
        float inv_eta_3 = inv_eta_2 * inv_eta;
        float inv_eta_4 = inv_eta_3 * inv_eta;
        float inv_eta_5 = inv_eta_4 * inv_eta;
        
        return 0.919317f - 3.4793f * inv_eta + 6.75335f * inv_eta_2 - 
               7.80989f * inv_eta_3 + 4.98554f * inv_eta_4 - 1.36881f * inv_eta_5;
    }
}

#endif // FRESNEL_GLSL