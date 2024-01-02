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

	for (int bounce = 0; bounce < maxBounce; bounce++) {
		RTCRay rtc_ray = RayToRTCRay(ray);
		RTCRayHit rtc_rayhit = MakeRayHit(rtc_ray);
		scene->TraceRay(rtc_rayhit, info);

		if (info.t == Infinity) {
			float t = 0.5f * (ray.GetDir().y + 1.0f);
			float a[3] = { 0.5f, 0.7f, 1.0f };
			float b[3] = { 1.0f, 1.0f, 1.0f };
			RGBSpectrum color1 = RGBSpectrum::FromRGB(b);
			RGBSpectrum color2 = RGBSpectrum::FromRGB(a);
			RGBSpectrum color = Lerp(t, color1, color2);

			radiance += color * history;

			break;
		}

// 		if (info.material->GetType() == MaterialType::DiffuseLightMaterial) {
// 			float misWeight = 1.0f;
// 			RGBSpectrum light_radiance = info.material->Emit(info.uv);
// 			if (bounce != 0) {
// 
// 			}
// 			radiance += misWeight * history * light_radiance;
// 
// 			break;
// 		}

		float bsdf_pdf = 0.0f;
		RGBSpectrum bsdf = info.material->Sample(V, L, bsdf_pdf, info, sampler);
		float costheta = std::abs(glm::dot(info.Ns, L));
		if (std::isnan(bsdf_pdf) || bsdf_pdf == 0.0f || bsdf.IsBlack()) {
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

RGBSpectrum* VolumetricPathTracing::RenderImage(const PostProcessing& post, RGBSpectrum* image) {
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
			image[j * width + i] = radiance;
		}
	}
	sampler->NextSample();

	return image;
}
