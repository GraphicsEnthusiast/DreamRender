#include "Renderer.h"

int main() {
	int Width = 800;
	int Height = 800;

	Transform tran;
	Pinhole camera(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height);
	//Thinlens camera2(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height, 2.0f);
	Sampler* sampler = new SimpleSobol(0);
	PostProcessing post(std::make_shared<Uncharted2>());
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);
	//scene.AddShape(new Sphere(Point3f(0.0f, 0.0f, 3.0f), 0.5f));
	scene.AddShape(new TriangleMesh(NULL, "teapot.obj", tran));
	scene.SetCamera(std::make_shared<Pinhole>(camera));
	scene.Commit();

	VolumetricPathTracing vpt(std::make_shared<Scene>(scene), sampler, std::make_shared<Gaussian>(), Width, Height, 5);
	Renderer renderer(std::make_shared<VolumetricPathTracing>(vpt), post);

	renderer.Run();

	return 0;
}
