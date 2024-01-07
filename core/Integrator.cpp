#include "Integrator.h"

float Integrator::PowerHeuristic(float pdf1, float pdf2, int beta) {
	float p1 = pow(pdf1, beta);
	float p2 = pow(pdf2, beta);

	return p1 / (p1 + p2);
}

RGBSpectrum VolumetricPathTracing::SolvingIntegrator(Ray& ray, IntersectionInfo& info) {
	RGBSpectrum radiance(0.0f);
	RGBSpectrum history(1.0f);
	Vector3f V = -ray.GetDir();
	Vector3f L = ray.GetDir();
	Point3f pre_position = ray.GetOrg();
	float bp_pdf = 0.0f;// bsdf or phase pdf
	float mult_trans_pdf = 1.0f;

	auto HitNothing = [](const IntersectionInfo& info) ->bool {
		return info.t == Infinity;
	};

	auto HitLight = [](const IntersectionInfo& info) ->bool {
		if (info.material == NULL) {
			return false;
		}

		return info.material->GetType() == MaterialType::DiffuseLightMaterial;
	};

	auto HitMediumBoundary = [](const IntersectionInfo& info) -> bool {
		if (info.material == NULL) {
			return false;
		}

		return info.material->GetType() == MaterialType::MediumBoundaryMaterial;
	};

	auto UpdateMediumInfo = [](IntersectionInfo& info, float actual_distance, const Point3f& pre_position, const Vector3f& L) -> void {
		info.position = pre_position + actual_distance * L;
		info.t = actual_distance;
		info.frontFace = true;
		info.Ng = Vector3f(0.0f);
		info.Ns = Vector3f(0.0f);
		info.uv = Point2f(0.0f);
		info.material = NULL;
		info.geomID = -1;
		info.primID = -1;
	};

	for (int bounce = 0; bounce < maxBounce; bounce++) {
		RTCRayHit rtc_rayhit = MakeRayHit(ray.GetOrg(), ray.GetDir());
		scene->TraceRay(rtc_rayhit, info);

		auto medium = info.mi.GetMedium(HitLight(info) ? true : info.frontFace);
		bool scattered = false;
		float trans_pdf = 0.0f;
		float actual_distance = 0.0f;
		RGBSpectrum transmittance(0.0f);

		if (medium != NULL) {
			// Sample medium distance
			transmittance = medium->SampleDistance(info.t, actual_distance, trans_pdf, scattered, sampler);

			if (std::isnan(trans_pdf) || trans_pdf == 0.0f) {
				break;
			}

			history *= (transmittance / trans_pdf);
			mult_trans_pdf *= trans_pdf;
		
			if (scattered) {
				UpdateMediumInfo(info, actual_distance, pre_position, L);
		
				// Sample light
				Vector3f lightL;
				float light_pdf = 0.0f;
				float phase_pdf = 0.0f;
				float mult_trans_pdf_nee = 1.0f;
				RGBSpectrum light_radiance = scene->SampleLightEnvironment(lightL, light_pdf, mult_trans_pdf_nee, info, sampler);
				RGBSpectrum attenuation = medium->GetPhaseFunction()->Evaluate(V, lightL, phase_pdf, info);
				phase_pdf *= mult_trans_pdf_nee;

				if (!(std::isnan(phase_pdf) || std::isnan(light_pdf) || phase_pdf == 0.0f || light_pdf == 0.0f)) {
					float misWeight = PowerHeuristic(light_pdf, phase_pdf, 2);
		
					radiance += misWeight * history * attenuation * light_radiance / light_pdf;
				}
		
				// Sample phase
				attenuation = medium->GetPhaseFunction()->Sample(V, L, phase_pdf, info, sampler);
		
				if (std::isnan(phase_pdf) || phase_pdf == 0.0f) {
					break;
				}
		
				bp_pdf = phase_pdf;
				history *= (attenuation / phase_pdf);
			}
		}

		if (!scattered) {
			if (HitLight(info)) {// Hit light
				float misWeight = 1.0f;
				float light_pdf = 0.0f;
				RGBSpectrum light_radiance = scene->EvaluateLight(info.geomID, L, light_pdf, info);
				bp_pdf *= mult_trans_pdf;

				if (bounce != 0) {
					if (std::isnan(light_pdf) || light_pdf == 0.0f) {
						break;
					}

					misWeight = PowerHeuristic(bp_pdf, light_pdf, 2);
				}
				radiance += misWeight * history * light_radiance;

				break;
			}
			else if (HitNothing(info)) {// Hit nothing
				float misWeight = 1.0f;
				float light_pdf = 0.0f;
				RGBSpectrum back_radiance = scene->EvaluateEnvironment(L, light_pdf);
				bp_pdf *= mult_trans_pdf;

				if (bounce != 0) {
					if (std::isnan(light_pdf) || light_pdf == 0.0f) {
						break;
					}

					misWeight = PowerHeuristic(bp_pdf, light_pdf, 2);
				}

				radiance += misWeight * history * back_radiance;

				break;
			}
			else if (HitMediumBoundary(info)) {// Hit medium boundary
				V = -L;
				pre_position = info.position;
				ray = Ray::SpawnRay(pre_position, L, info.Ng);
				bounce--;

				continue;
			}
			else {
				// Sample light
				float light_pdf = 0.0f, bsdf_pdf = 0.0f;
				Vector3f lightL;
				float mult_trans_pdf_nee = 1.0f;
				RGBSpectrum light_radiance = scene->SampleLightEnvironment(lightL, light_pdf, mult_trans_pdf_nee, info, sampler);
				RGBSpectrum bsdf = info.material->Evaluate(V, lightL, bsdf_pdf, info);
				bsdf_pdf *= mult_trans_pdf_nee;
				float costheta = std::max(glm::dot(info.Ns, lightL), 0.0f);

				if (!(std::isnan(bsdf_pdf) || std::isnan(light_pdf) || bsdf_pdf == 0.0f || light_pdf == 0.0f)) {
					float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);

					radiance += misWeight * history * bsdf * costheta * light_radiance / light_pdf;
				}

				// Sample surface
				bsdf = info.material->Sample(V, L, bsdf_pdf, info, sampler);
				bp_pdf = bsdf_pdf;
				costheta = std::abs(glm::dot(info.Ns, L));

				if (std::isnan(bsdf_pdf) || bsdf_pdf == 0.0f) {
					break;
				}

				history *= (bsdf * costheta / bsdf_pdf);
			}
		}

		// Russian roulette
		if (bounce > 3 && history.MaxComponentValue() < 0.1f) {
			auto continueProperbility = std::max(0.05f, 1.0f - history.MaxComponentValue());

			if (sampler->Get1() < continueProperbility) {
				break;
			}

			history /= (1.0f - continueProperbility);
		}

		if (history.HasNaNs()) {
			break;
		}

		// Update information
		V = -L;
		mult_trans_pdf = 1.0f;
		pre_position = info.position;
		ray = Ray::SpawnRay(info.position, L, info.Ng);
	}

	return radiance;
}

void VolumetricPathTracing::RenderImage(const PostProcessing& post, RGBSpectrum* image) {
	omp_set_num_threads(32);
#pragma omp parallel for
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			sampler->SetPixel(i, j);

			Point2f jitter = filter->FilterPoint2f(sampler->Get2());
			float pixelX = ((float)i + 0.5f + jitter.x) / width;
			float pixelY = ((float)j + 0.5f + jitter.y) / height;

			IntersectionInfo info;
			Ray ray = scene->GetCamera()->GenerateRay(sampler, pixelX, pixelY);
			RGBSpectrum radiance = SolvingIntegrator(ray, info);

			if (radiance.HasNaNs()) {
				assert(0);
			}

			image[j * width + i] = const_cast<PostProcessing&>(post).GetScreenColor(radiance);
		}
	}
	sampler->NextSample();
}
