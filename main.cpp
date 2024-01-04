#include "Renderer.h"

int main() {
	int Width = 800;
	int Height = 800;

	Transform tran = Transform::Rotate(0.0f, 45.0f, 0.0f);
	Pinhole camera(Point3f(10.0f, 8.0f, -10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height);
	//Thinlens camera2(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height, 2.0f);
	PostProcessing post(std::make_shared<ACES>());
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);

	float specular[3] = { 1.0f, 1.0f, 1.0f };
	float albedo[3] = { 1.0f, 1.0f, 1.0f };
	float albedo2[3] = { 0.1f, 0.5f, 0.6f };
	float roughness[3] = { 0.1f };
	float roughness2[3] = { 0.4f };
	float radiance[3] = { 5.0f, 5.0f, 5.0f };
	float radiance2[3] = { 5.0f, 0.0f, 0.0f };
	float eta[3] = { 2.76404, 1.95417, 1.62766 };
	float k[3] = { 3.83077, 2.73841, 2.31812 };
	auto material = std::make_shared<Diffuse>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)));
	auto material2 = std::make_shared<DiffuseLight>(RGBSpectrum::FromRGB(radiance));
	auto material3 = std::make_shared<DiffuseLight>(RGBSpectrum::FromRGB(radiance2));
	auto material4 = std::make_shared<Diffuse>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo2)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)));
	auto material5 = std::make_shared<Conductor>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness2)),
		std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)), RGBSpectrum::FromRGB(eta), RGBSpectrum::FromRGB(k));
	auto material6 = std::make_shared<Dielectric>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)),
		std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)), 1.5f, 1.0f);
	auto material7 = std::make_shared<Plastic>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo2)), std::make_shared<Constant>(RGBSpectrum::FromRGB(specular)), 
		std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)), 1.5f, 1.0f, true);
	auto material8 = std::make_shared<ThinDielectric>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)),
		std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)), 1.5f, 1.0f);
	scene.AddShape(new Quad(material, Point3f(-10.0f, -0.05f, -10.0f), Vector3f(20.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 20.0f)));
//	scene.AddShape(new Sphere(material8, Point3f(0.0f, 3.0f, 0.0f), 3.0f));
	scene.AddShape(new TriangleMesh(material8, "teapot.obj", tran));
//	scene.AddLight(std::make_shared<QuadArea>(new Quad(material2, Point3f(3.0f, 7.0f, 3.0f), Vector3f(-3.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, -3.0f))));
// 	scene.AddLight(std::make_shared<SphereArea>(new Sphere(material2, Point3f(4.0f, 10.0f, 4.0f), 3.0f)));
// 	scene.AddLight(std::make_shared<SphereArea>(new Sphere(material3, Point3f(14.0f, 8.0f, -14.0f), 3.0f)));
	scene.AddLight(std::make_shared<InfiniteArea>(std::make_shared<Hdr>("spruit_sunrise_4k.hdr"), 1.0f));
	scene.SetCamera(std::make_shared<Pinhole>(camera));
	scene.Commit();

	VolumetricPathTracing vpt(std::make_shared<Scene>(scene), std::make_shared<Independent>(), std::make_shared<Gaussian>(), Width, Height, 50);
	Renderer renderer(std::make_shared<VolumetricPathTracing>(vpt), post);

	renderer.Run();

	return 0;
}
