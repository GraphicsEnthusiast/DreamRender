#include "Renderer.h"

int main() {
	int Width = 800;
	int Height = 800;

	Transform tran;
	Pinhole camera(Point3f(10.0f, 10.0f, -10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height);
	//Thinlens camera2(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height, 2.0f);
	PostProcessing post(std::make_shared<Reinhard>());
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);

	float albedo[3] = { 0.1f, 0.7f, 0.8f };
	float roughness[3] = { 0.1f, 0.1f, 0.1f };
	float radiance[3] = { 5.0f, 5.0f, 5.0f };
	float radiance2[3] = { 5.0f, 0.0f, 0.0f };
	auto material = std::make_shared<Diffuse>(std::make_shared<Constant>(RGBSpectrum::FromRGB(albedo)), std::make_shared<Constant>(RGBSpectrum::FromRGB(roughness)));
	auto material2 = std::make_shared<DiffuseLight>(RGBSpectrum::FromRGB(radiance));
	auto material3 = std::make_shared<DiffuseLight>(RGBSpectrum::FromRGB(radiance2));
	//scene.AddShape(new Sphere(material, Point3f(0.0f, 0.0f, 0.0f), 3.0f));
	scene.AddShape(new TriangleMesh(material, "teapot.obj", tran));
	scene.AddLight(std::make_shared<SphereArea>(new Sphere(material2, Point3f(0.0f, 8.0f, 10.0f), 2.0f)));
	scene.AddLight(std::make_shared<SphereArea>(new Sphere(material3, Point3f(10.0f, 8.0f, 0.0f), 2.0f)));
//	scene.AddLight(std::make_shared<InfiniteArea>(std::make_shared<Hdr>("spruit_sunrise_4k.hdr"), 1.0f));
	scene.SetCamera(std::make_shared<Pinhole>(camera));
	scene.Commit();

	VolumetricPathTracing vpt(std::make_shared<Scene>(scene), std::make_shared<Independent>(), std::make_shared<Gaussian>(), Width, Height, 5);
	Renderer renderer(std::make_shared<VolumetricPathTracing>(vpt), post);

	renderer.Run();

	return 0;
}
