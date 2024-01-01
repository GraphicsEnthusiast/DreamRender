#include "Integrator.h"

Integrator::~Integrator() {
	if (image != NULL) {
		delete[] image;
		image = NULL;
	}

	if (sampler != NULL) {
		delete sampler;
		sampler = NULL;
	}
}

float Integrator::PowerHeuristic(float pdf1, float pdf2, int beta) {
	float p1 = pow(pdf1, beta);
	float p2 = pow(pdf2, beta);

	return p1 / (p1 + p2);
}

RGBSpectrum VolumetricPathTracing::SolvingIntegrator(RTCRayHit& rayhit, IntersectionInfo& info) {
	return RGBSpectrum();
}

RGBSpectrum* VolumetricPathTracing::RenderImage() {
	omp_set_num_threads(32);
#pragma omp parallel for
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			Point2f jitter = filter->FilterPoint2f(sampler->Get2());
			float pixelX = ((float)i + 0.5f + jitter.x) / width;
			float pixelY = ((float)j + 0.5f + jitter.y) / height;

			sampler->SetPixel(i, j);

			Ray ray = scene->GetCamera()->GenerateRay(sampler, pixelX, pixelY);
			RTCRay rtc_ray = RayToRTCRay(ray);

			IntersectionInfo info;
			scene->TraceRay(MakeRayHit(rtc_ray), info);

			if (info.t == Infinity) {
				float t = 0.5f * (ray.GetDir().y + 1.0f);
				float a[3] = { 0.5f, 0.7f, 1.0f };
				float b[3] = { 1.0f, 1.0f, 1.0f };
				RGBSpectrum color1 = RGBSpectrum::FromRGB(b);
				RGBSpectrum color2 = RGBSpectrum::FromRGB(a);
				RGBSpectrum color = Lerp(t, color1, color2);
				image[j * width + i] = color;
			}
			else {
				float a[3] = { info.Ns[0], info.Ns[1], info.Ns[2] };
				RGBSpectrum color = (RGBSpectrum::FromRGB(a) + 1.0f) * 0.5f;
				image[j * width + i] = color;
			}

			//nowTexture[j * Width + i] = post.GetScreenColor(color);
		}
	}
	sampler->NextSample();

	return image;
}
