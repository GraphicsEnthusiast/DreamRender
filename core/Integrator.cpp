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
	float bp_pdf = 0.0f;// bsdf or phase pdf

	for (int bounce = 0; bounce < maxBounce; bounce++) {
		RTCRayHit rtc_rayhit = MakeRayHit(ray.GetOrg(), ray.GetDir());
		scene->TraceRay(rtc_rayhit, info);

		// Hit nothing
		if (info.t == Infinity) {
			float misWeight = 1.0f;
			float light_pdf = 0.0f;
			RGBSpectrum back_radiance = scene->EvaluateEnvironment(L, light_pdf);
			if (bounce != 0) {
				if (std::isnan(light_pdf) || light_pdf == 0.0f) {
					break;
				}
				misWeight = PowerHeuristic(bp_pdf, light_pdf, 2);
			}

			radiance += misWeight * history * back_radiance;

			break;
		}

		// Hit light
		if (info.material->GetType() == MaterialType::DiffuseLightMaterial) {
			float misWeight = 1.0f;
			float light_pdf = 0.0f;
			RGBSpectrum light_radiance = scene->EvaluateLight(info.geomID, L, light_pdf, info);
			if (bounce != 0) {
				if (std::isnan(light_pdf) || light_pdf == 0.0f) {
					break;
				}
				misWeight = PowerHeuristic(bp_pdf, light_pdf, 2);
			}
			radiance += misWeight * history * light_radiance;

			break;
		}

		float light_pdf = 0.0f, bsdf_pdf = 0.0f;
		// Sample light
		Vector3f lightL;
		RGBSpectrum light_radiance = scene->SampleLightEnvironment(lightL, light_pdf, info, sampler);
		RGBSpectrum bsdf = info.material->Evaluate(V, lightL, bsdf_pdf, info);
		float costheta = std::max(glm::dot(info.Ns, lightL), 0.0f);
		float misWeight = PowerHeuristic(light_pdf, bsdf_pdf, 2);
		if (!(std::isnan(bsdf_pdf) || std::isnan(light_pdf) || bsdf_pdf == 0.0f || light_pdf == 0.0f)) {
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

		float prr = std::min((history[0] + history[1] + history[2]) / 3.0f, 0.95f);
		if (sampler->Get1() > prr) {
			break;
		}
		history /= prr;

		if (history.HasNaNs()) {
			break;
		}

		V = -L;
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
			//image[j * width + i] = radiance;
		}
	}
	sampler->NextSample();
}
