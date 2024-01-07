#pragma once

#include "Utils.h"
#include "Spectrum.h"

namespace Fresnel {
	float FresnelSchlick(float f0, float VdotH);

	RGBSpectrum FresnelSchlick(const RGBSpectrum& f0, float VdotH);

	RGBSpectrum FresnelConductor(const Vector3f& V, const Vector3f& H, const RGBSpectrum& eta_r, const RGBSpectrum& eta_i);

	RGBSpectrum AverageFresnelConductor(const RGBSpectrum& eta, const RGBSpectrum& k);

	float FresnelDielectric(const Vector3f& V, const Vector3f& H, float eta_inv);

	float AverageFresnelDielectric(float eta);
}