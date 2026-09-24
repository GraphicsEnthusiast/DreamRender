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

#endif // FRESNEL_GLSL