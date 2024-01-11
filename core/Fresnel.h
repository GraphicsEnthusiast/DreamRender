#pragma once

#include "Utils.h"
#include "Spectrum.h"

namespace Fresnel {
	float FresnelSchlick(float f0, float VdotH);

	Spectrum FresnelSchlick(const Spectrum& f0, float VdotH);

	Spectrum FresnelConductor(const Vector3f& V, const Vector3f& H, const Spectrum& eta_r, const Spectrum& eta_i);

	Spectrum AverageFresnelConductor(const Spectrum& eta, const Spectrum& k);

	float FresnelDielectric(const Vector3f& V, const Vector3f& H, float eta_inv);

	float AverageFresnelDielectric(float eta);
}