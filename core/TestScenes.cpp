#include "TestScenes.h"

RTCDevice rtc_device = rtcNewDevice(NULL);

std::shared_ptr<Renderer> TestScenes::Diningroom_MeshLight() {
	int Width = 1200;
	int Height = 1000;

	// Material
	float yellow[3] = { 0.75f, 0.8f, 0.7f };
	float radiance[3] = { 17.0f, 12.0f, 8.0f };
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
	auto tone = std::make_shared<ACES>();

	// PostProcessing
	auto post = std::make_shared<PostProcessing>(tone, 0.0f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}

std::shared_ptr<Renderer> TestScenes::Diningroom_EnvironmentLight() {
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
	auto envlight = std::make_shared<InfiniteArea>(std::make_shared<Hdr>("scenes/diningroom/textures/spaichingen_hill_4k.hdr"));

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
	auto camera = std::make_shared<Pinhole>(Point3f(-1.0f, 2.0f, 9.0f), Point3f(-1.0f, 2.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 45.0f, (float)Width / (float)Height);

	// Filter
	auto filter = std::make_shared<Gaussian>();

	// Sampler
	auto sampler = std::make_shared<Independent>();

	// Scene
	auto scene = std::make_shared<Scene>(rtc_device);
	scene->AddLight(envlight);
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
	auto post = std::make_shared<PostProcessing>(tone, 0.5f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}

std::shared_ptr<Renderer> TestScenes::Subsurface() {
	int Width = 800;
	int Height = 800;

	// Medium
	float sigma_a[3] = { 0.0030f, 0.0034f, 0.046f };
	float sigma_s[3] = { 2.29f, 2.39f, 1.97f };
	float scale = 100.0f;
	auto medium = std::make_shared<Homogeneous>(std::make_shared<Isotropic>(), Spectrum::FromRGB(sigma_s), Spectrum::FromRGB(sigma_a), scale);

	// Material
	float diffuse_white[3] = { 0.725f, 0.71f, 0.68f };
	float diffuse_red[3] = { 0.63f, 0.065f, 0.05f };
	float diffuse_green[3] = { 0.14f, 0.45f, 0.091f };
	float albedo[3] = { 0.8f, 0.9f, 0.82f };
	float roughness[3] = { 0.1f };

	auto buddha_material = std::make_shared<ThinDielectric>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f);
	auto white = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse_white)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto red = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse_red)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
	auto green = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse_green)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));

	// Light
	auto envlight = std::make_shared<InfiniteArea>(std::make_shared<Hdr>("scenes/subsurface/textures/spruit_sunrise_4k.hdr"));

	// Shape
	Transform tran;
	Transform tran_b = Transform::Rotate(0.0f, 180.0f, 0.0f);
	auto buddha = new TriangleMesh(buddha_material, "scenes/subsurface/models/buddha.obj", tran_b, NULL, medium);
	auto cbox_back = new TriangleMesh(white, "scenes/subsurface/models/cbox_back.obj", tran_b);
	auto cbox_floor = new TriangleMesh(white, "scenes/subsurface/models/cbox_floor.obj", tran);
	auto cbox_greenwall = new TriangleMesh(red, "scenes/subsurface/models/cbox_greenwall.obj", tran);
	
	// Camera
	auto camera = std::make_shared<Pinhole>(Point3f(0.0f, 0.0f, 55.0f), Point3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, 
		(float)Width / (float)Height);

	// Filter
	auto filter = std::make_shared<Gaussian>();

	// Sampler
	auto sampler = std::make_shared<Independent>();

	// Scene
	auto scene = std::make_shared<Scene>(rtc_device);
	scene->AddLight(envlight);
	scene->AddShape(buddha);
	scene->AddShape(cbox_back);
	scene->AddShape(cbox_floor);
	scene->AddShape(cbox_greenwall);
	scene->SetCamera(camera);
	scene->Commit();

	// Integrator
	auto integrator = std::make_shared<VolumetricPathTracing>(scene, sampler, filter, Width, Height, 2048);

	// ToneMapper
	auto tone = std::make_shared<ACES>();

	// PostProcessing
	auto post = std::make_shared<PostProcessing>(tone, 0.0f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}

std::shared_ptr<Renderer> TestScenes::Surface() {
	int Width = 1280;
	int Height = 720;

	// Material
	float albedo[3] = { 1.0f, 1.0f, 1.0f };
	float diffuse[3] = { 0.4f, 0.4f, 0.4f };
	float specular[3] = { 1.0f, 1.0f, 1.0f };
	float roughness[3] = { 0.1f };
	float roughness2[3] = { 0.5f };
	float eta[3] = { 0.14282f, 0.37414f, 1.43944f };
 	float k[3] = { 3.97472f, 2.38066f, 1.59981f };

	auto conductor = std::make_shared<Conductor>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), Spectrum::FromRGB(eta), Spectrum::FromRGB(k));
	auto dragon_material = std::make_shared<ClearcoatedConductor>(conductor, std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.0f);
	auto clock_material = std::make_shared<MetalWorkflow>(std::make_shared<Image>("scenes/surface/textures/clock_albedo.bmp"), 
		std::make_shared<Image>("scenes/surface/textures/clock_roughness.bmp"), std::make_shared<Image>("scenes/surface/textures/clock_roughness.bmp"),
		std::make_shared<Image>("scenes/surface/textures/clock_metallic.bmp"), std::make_shared<Image>("scenes/surface/textures/clock_normal.bmp"));
	auto dielctric = std::make_shared<Dielectric>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f);
	auto dielctric2 = std::make_shared<Dielectric>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness2)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness2)), 1.5f, 1.0f);
	auto teapot_material = std::make_shared<Mixture>(dielctric, dielctric2, 0.5f);
	auto plane_material = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(diffuse)), std::make_shared<Constant>(Spectrum::FromRGB(specular)),
		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, true);

	// Light
	auto envlight = std::make_shared<InfiniteArea>(std::make_shared<Hdr>("scenes/surface/textures/spruit_sunrise_4k.hdr"));

	// Shape
	Transform tran;
	auto clock = new TriangleMesh(clock_material, "scenes/surface/models/clock.obj", tran);
	auto dragon = new TriangleMesh(dragon_material, "scenes/surface/models/dragon.obj", tran);
	auto teapot = new TriangleMesh(teapot_material, "scenes/surface/models/teapot.obj", tran);
	auto plane = new TriangleMesh(plane_material, "scenes/surface/models/plane.obj", tran);

	// Camera
	auto camera = std::make_shared<Pinhole>(Point3f(15.0f, 7.5f, 0.0f), Point3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f,
		(float)Width / (float)Height);

	// Filter
	auto filter = std::make_shared<Gaussian>();

	// Sampler
	auto sampler = std::make_shared<SimpleSobol>(0);

	// Scene
	auto scene = std::make_shared<Scene>(rtc_device);
	scene->AddLight(envlight);
	scene->AddShape(clock);
	scene->AddShape(dragon);
	scene->AddShape(teapot);
	scene->AddShape(plane);
	scene->SetCamera(camera);
	scene->Commit();

	// Integrator
	auto integrator = std::make_shared<VolumetricPathTracing>(scene, sampler, filter, Width, Height, 64);

	// ToneMapper
	auto tone = std::make_shared<Uncharted2>();

	// PostProcessing
	auto post = std::make_shared<PostProcessing>(tone, 0.0f);

	// Renderer
	auto renderer = std::make_shared<Renderer>(integrator, post);

	return renderer;
}
