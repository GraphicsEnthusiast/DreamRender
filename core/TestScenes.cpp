#include "TestScenes.h"

RTCDevice rtc_device = rtcNewDevice(NULL);

std::shared_ptr<Renderer> TestScenes::Diningroom_MeshLight() {
	int Width = 1200;
	int Height = 1000;

	// Material
	float yellow[3] = { 0.75f, 0.8f, 0.7f };
	float radiance[3] = { 16.0f, 8.0f, 4.0f };
	float diffuse[3] = { 0.88f, 0.85f, 0.8f };
	float diffuse2[3] = { 0.4f, 0.4f, 0.4f };
	float albedo[3] = { 0.85f, 0.85f, 0.75f };
	float albedo2[3] = { 1.0f, 1.0f, 1.0f };
	float specular[3] = { 1.0f, 1.0f, 1.0f };
	float roughness[3] = { 0.1f };
	float roughness2[3] = { 0.3f };
	auto light_material = std::make_shared<DiffuseLight>(Spectrum::FromRGB(radiance));
	auto floor_material = std::make_shared<Diffuse>(std::make_shared<Image>("scenes/diningroom/textures/Tiles.jpg"), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto wall_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto light2_window_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto wall_picture_material = std::make_shared<Diffuse>(std::make_shared<Image>("scenes/diningroom/textures/picture.jpg"), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto table1_chair_spoon_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, true);
	auto table2_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), 1.5f, 1.0f, true);
	auto chair2_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness2)));
	auto chair3_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto cup_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(yellow)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.4f, 1.0f, true);
	auto plate_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, false);
	auto teapot_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.45f, 1.0f, true);

	// Light
	auto light = std::make_shared<TriangleMeshArea>(new TriangleMesh(light_material, "scenes/diningroom/models/light.obj", Transform()));

	// Shape
 	auto floor = new TriangleMesh(floor_material, "scenes/diningroom/models/floor.obj", Transform());
 	auto wall = new TriangleMesh(wall_material, "scenes/diningroom/models/wall.obj", Transform());
	auto light2 = new TriangleMesh(light2_window_material, "scenes/diningroom/models/light2.obj", Transform());
	auto window = new TriangleMesh(light2_window_material, "scenes/diningroom/models/window.obj", Transform());
	auto wall_picture = new TriangleMesh(wall_picture_material, "scenes/diningroom/models/wall_picture.obj", Transform());
	auto table1 = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/table1.obj", Transform());
	auto table2 = new TriangleMesh(table2_material, "scenes/diningroom/models/table2.obj", Transform());
	auto chair1 = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/chair1.obj", Transform());
	auto chair2 = new TriangleMesh(chair2_material, "scenes/diningroom/models/chair2.obj", Transform());
	auto chair3 = new TriangleMesh(chair3_material, "scenes/diningroom/models/chair3.obj", Transform());
	auto cup = new TriangleMesh(cup_material, "scenes/diningroom/models/cup.obj", Transform());
	auto plate = new TriangleMesh(plate_material, "scenes/diningroom/models/plate.obj", Transform());
	auto spoon = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/spoon.obj", Transform());
	auto pot = new TriangleMesh(teapot_material, "scenes/diningroom/models/pot.obj", Transform());
	auto teapot = new TriangleMesh(teapot_material, "scenes/diningroom/models/teapot.obj", Transform());

	// Camera
	auto camera = std::make_shared<Pinhole>(Point3f(-3.5f, 3.0f, 6.0f), Point3f(-1.0f, 1.7f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height);

	// Filter
	auto filter = std::make_shared<Gaussian>();

	// Sampler
	auto sampler = std::make_shared<Independent>();

	// Scene
	auto scene = std::make_shared<Scene>(rtc_device);
	scene->AddLight(light);
	scene->AddShape(floor);
	scene->AddShape(wall);
	scene->AddShape(light2);
	scene->AddShape(window);
	scene->AddShape(wall_picture);
	scene->AddShape(table1);
	scene->AddShape(table2);
	scene->AddShape(chair1);
	scene->AddShape(chair2);
	scene->AddShape(chair3);
	scene->AddShape(cup);
	scene->AddShape(plate);
	scene->AddShape(spoon);
	scene->AddShape(pot);
	scene->AddShape(teapot);
	scene->SetCamera(camera);
	scene->Commit();

	// Integrator
	auto integrator = std::make_shared<VolumetricPathTracing>(scene, sampler, filter, Width, Height, 64);

	// ToneMapper
	auto tone = std::make_shared<Reinhard>();

	// PostProcessing
	auto post = std::make_shared<PostProcessing>(tone, 1.0f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}

std::shared_ptr<Renderer> TestScenes::Diningroom_SphereLight() {
	int Width = 1200;
	int Height = 1000;

	// Material
	float radiance[3] = { 16.0f, 8.0f, 4.0f };
	float red[3] = { 0.8f, 0.3f, 0.3f };
	float yellow[3] = { 0.75f, 0.8f, 0.7f };
	float diffuse[3] = { 0.88f, 0.85f, 0.8f };
	float diffuse2[3] = { 0.4f, 0.4f, 0.4f };
	float albedo[3] = { 0.85f, 0.85f, 0.75f };
	float albedo2[3] = { 1.0f, 1.0f, 1.0f };
	float specular[3] = { 1.0f, 1.0f, 1.0f };
	float roughness[3] = { 0.1f };
	float roughness2[3] = { 0.3f };
	auto quadlight_material = std::make_shared<DiffuseLight>(Spectrum::FromRGB(radiance));
	auto light_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(red)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, true);
	auto floor_material = std::make_shared<Diffuse>(std::make_shared<Image>("scenes/diningroom/textures/Tiles.jpg"), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto wall_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto light2_window_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto wall_picture_material = std::make_shared<Diffuse>(std::make_shared<Image>("scenes/diningroom/textures/Teacup.png"), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto table1_chair_spoon_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, true);
	auto table2_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), 1.5f, 1.0f, true);
	auto chair2_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness2)));
	auto chair3_material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto cup_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(yellow)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.4f, 1.0f, true);
	auto plate_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, false);
	auto teapot_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.45f, 1.0f, true);

	// Light
	auto spherelight = std::make_shared<SphereArea>(new Sphere(quadlight_material, Point3f(15.0f, 5.0f, 0.0f), 5.0f));

	// Shape
	Transform tran;
	auto light = new TriangleMesh(light_material, "scenes/diningroom/models/light.obj", tran);
	auto floor = new TriangleMesh(floor_material, "scenes/diningroom/models/floor.obj", tran);
	auto wall = new TriangleMesh(wall_material, "scenes/diningroom/models/wall.obj", tran);
	auto light2 = new TriangleMesh(light2_window_material, "scenes/diningroom/models/light2.obj", tran);
	auto window = new TriangleMesh(light2_window_material, "scenes/diningroom/models/window.obj", tran);
	auto wall_picture = new TriangleMesh(wall_picture_material, "scenes/diningroom/models/wall_picture.obj", tran);
	auto table1 = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/table1.obj", tran);
	auto table2 = new TriangleMesh(table2_material, "scenes/diningroom/models/table2.obj", tran);
	auto chair1 = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/chair1.obj", tran);
	auto chair2 = new TriangleMesh(chair2_material, "scenes/diningroom/models/chair2.obj", tran);
	auto chair3 = new TriangleMesh(chair3_material, "scenes/diningroom/models/chair3.obj", tran);
	auto cup = new TriangleMesh(cup_material, "scenes/diningroom/models/cup.obj", tran);
	auto plate = new TriangleMesh(plate_material, "scenes/diningroom/models/plate.obj", tran);
	auto spoon = new TriangleMesh(table1_chair_spoon_material, "scenes/diningroom/models/spoon.obj", tran);
	auto pot = new TriangleMesh(teapot_material, "scenes/diningroom/models/pot.obj", tran);
	auto teapot = new TriangleMesh(teapot_material, "scenes/diningroom/models/teapot.obj", tran);

	// Camera
	auto camera = std::make_shared<Pinhole>(Point3f(-1.0f, 2.0f, 8.5f), Point3f(-1.0f, 2.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 45.0f, (float)Width / (float)Height);

	// Filter
	auto filter = std::make_shared<Gaussian>();

	// Sampler
	auto sampler = std::make_shared<Independent>();

	// Scene
	auto scene = std::make_shared<Scene>(rtc_device);
	scene->AddLight(spherelight);
	scene->AddShape(light);
	scene->AddShape(floor);
	scene->AddShape(wall);
	scene->AddShape(light2);
	scene->AddShape(window);
	scene->AddShape(wall_picture);
	scene->AddShape(table1);
	scene->AddShape(table2);
	scene->AddShape(chair1);
	scene->AddShape(chair2);
	scene->AddShape(chair3);
	scene->AddShape(cup);
	scene->AddShape(plate);
	scene->AddShape(spoon);
	scene->AddShape(pot);
	scene->AddShape(teapot);
	scene->SetCamera(camera);
	scene->Commit();

	// Integrator
	auto integrator = std::make_shared<VolumetricPathTracing>(scene, sampler, filter, Width, Height, 64);

	// ToneMapper
	auto tone = std::make_shared<Reinhard>();

	// PostProcessing
	auto post = std::make_shared<PostProcessing>(tone, 1.0f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}