#include <Utils.h>
#include <SceneParser.h>
#include <Render.h>
#include <Medium.h>

Scene CornellBox() {
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);

	string back = "../TestScene/CornellBox/Model/back.obj";
	string bottom = "../TestScene/CornellBox/Model/bottom.obj";
	string left = "../TestScene/CornellBox/Model/left.obj";
	string right = "../TestScene/CornellBox/Model/right.obj";
	string top = "../TestScene/CornellBox/Model/top.obj";
	string ajax = "../TestScene/CornellBox/Model/ajax.obj";

	mat4 model_ajax = GetTransformMatrix(vec3(0.0f, -2.5f, 0.0f), vec3(0.0f, 180.0f, 0.0f), vec3(4.0f));
	mat4 model_box = GetTransformMatrix(vec3(0.0f, -2.5f, 0.0f), vec3(0.0f, 0.0f, 0.0f), vec3(3.0f));

	shared_ptr<KullaConty> kulla_conty = make_shared<KullaConty>();
//	auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(8.0f)));
	auto red = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.8f, 0.2f, 0.3f)));
	auto green = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.14f, 0.45f, 0.091f)));
	auto grey = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.725f, 0.710f, 0.680f)));
	auto con = make_shared<SmoothConductor>(make_shared<ConstantTexture>(vec3(1.0f)),
		vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
	auto die = make_shared<SmoothDielectric>(make_shared<ConstantTexture>(vec3(1.0f)), 1.33f, 1.0f);
	auto spl = make_shared<SmoothPlastic>(make_shared<ConstantTexture>(vec3(0.2f, 0.54f, 0.72f)), make_shared<ConstantTexture>(vec3(1.0f)), 1.9f, 1.0f, true);
// 	auto met = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/albedo.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/roughness.png"),
// 		make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/metallic.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/normal.png"));

	auto medium = make_shared<HomogeneousMedium>(vec3(0.1486, 0.321, 0.736) * 10.0f, vec3(0.0f), new IsotropicPhaseFunction());
	auto medium2 = make_shared<HomogeneousMedium>(vec3(0.05f, 0.025f, 0.02f), vec3(0.05f, 0.03f, 0.06f), new IsotropicPhaseFunction());

// 	auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(8.0f, 46.4f, 64.0f)));
// 	auto lightmat2 = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(100.0f)));
// 	
// 	auto nullmat = make_shared<NullMaterial>();
// 	auto sphere = new Sphere(nullmat, vec3(0.7f, 0.0f, 0.0f), 0.5f, medium2, medium);
// 	scene.AddShape(sphere);
// 
// 	auto sphere2 = new Sphere(red, vec3(-0.7f, 0.0f, 0.0f), 0.5f, medium2);
// 	scene.AddShape(sphere2);
// 
// 	auto sphere3 = new Sphere(lightmat2, vec3(1.0f, 1.0f, -1.0f), 0.25f, medium2);
// 	auto sphere_light3 = make_shared<SphereLight>(sphere3);
// 	scene.AddLight(sphere_light3, sphere_light3->shape);

// 	auto quad = new Quad(lightmat, vec3(-1.0f, 3.45f, -1.0f), vec3(2.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f));
// 	auto quadlight = make_shared<QuadLight>(quad);
// 
// 	scene.AddLight(quadlight, quadlight->quad);
// 	auto nullmat = make_shared<NullMaterial>();
// 	scene.AddShape(new TriangleMesh(grey, back, model_box));
// 	scene.AddShape(new TriangleMesh(red, left, model_box));
// 	scene.AddShape(new TriangleMesh(green, right, model_box));
// 	scene.AddShape(new TriangleMesh(grey, bottom, model_box));
// 	scene.AddShape(new TriangleMesh(grey, top, model_box));
// 	scene.AddShape(new TriangleMesh(nullmat, ajax, model_ajax, NULL, medium));

// 	auto sphere = new Sphere(lightmat2, vec3(1.5f, 1.5f, 2.0f), 0.25f, medium);
// 	auto sphere_light = make_shared<SphereLight>(sphere);
// 	scene.AddLight(sphere_light, sphere_light->shape);
// 
// 	auto sphere2 = new Sphere(lightmat, vec3(-1.5f, 1.5f, 2.0f), 2.0f, medium);
// 	auto sphere_light2 = make_shared<SphereLight>(sphere2);
// 	scene.AddLight(sphere_light2, sphere_light2->shape);

// 	auto sphere2 = new Sphere(red, vec3(-0.7f, 0.0f, 0.0f), 0.5f);
// 	scene.AddShape(sphere2);

// 	auto nullmat = make_shared<NullMaterial>();
// 	auto sphere3 = new Sphere(nullmat, vec3(0.0f, 0.0f, 0.0f), 0.75f, medium2, medium);
// 	scene.AddShape(sphere3);

	auto nullmat = make_shared<NullMaterial>();
	auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(8.0f, 46.4f, 64.0f)));
	auto sphere = new Sphere(lightmat, vec3(1.0f, 1.0f, -1.0f), 0.25f);
	auto sphere_light = make_shared<SphereLight>(sphere);
//	scene.AddLight(sphere_light, sphere_light->shape);

	auto sphere2 = new Sphere(die, vec3(0.0f), 0.75f, NULL, medium);
	scene.AddShape(sphere2);

	auto lightmat2 = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(2.4f, 1.0f, 2.4f)));
	
	auto sphere3 = new Sphere(lightmat2, vec3(-1.5f, 1.5f, -1.0f), 1.0f);
	auto sphere_light3 = make_shared<SphereLight>(sphere3);
//	scene.AddLight(sphere_light3, sphere_light3->shape);

// 	auto medium = make_shared<HomogeneousMedium>(vec3(200.0f), vec3(100.0f), new IsotropicPhaseFunction());
// 	auto medium2 = make_shared<HomogeneousMedium>(vec3(0.05f, 0.025f, 0.02f), vec3(0.05f, 0.03f, 0.06f), new IsotropicPhaseFunction());
// 
// 	auto rdie = make_shared<RoughDielectric>(make_shared<KullaConty>(),
// 		make_shared<ConstantTexture>(vec3(1.0f)), make_shared<ConstantTexture>(vec3(0.1f)), make_shared<ConstantTexture>(vec3(0.0f)), 1.33f, 1.0f);
// 	auto sphere = new Sphere(rdie, vec3(0.0f), 1.0f, NULL, medium);
// 	scene.AddShape(sphere);
// 	auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(10.0f)));
// 	auto sphere2 = new Sphere(lightmat, vec3(0.0f, 0.0f, -10.0f), 5.0f);
// 	auto sphere_light2 = make_shared<SphereLight>(sphere2);
// 	scene.AddLight(sphere_light2, sphere_light2->shape);

	scene.width = 1200;
	scene.height = 1000;
	scene.SetCamera(make_shared<PinholeCamera>(vec3(0.0f, 0.0f, -4.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), 1.0f, 45.0f,
		static_cast<float>(scene.width) / static_cast<float>(scene.height)));
	scene.SetFilter(make_shared<FilterGaussian>());
	scene.SetHDR(make_shared<InfiniteAreaLight>(make_shared<HdrTexture>("../TestScene/Boy/HDR/spaichingen_hill_4k.hdr")));

	scene.Commit();

	return scene;
}


int main(int argc, char** argv) {
// 	cout << "Hello, DreamRender!" << endl << endl;
// 
// 	SceneParser scene_parser;
// 	RTCDevice rtc_device = rtcNewDevice(NULL);
// 	Scene scene(rtc_device);
// 	bool is_succeed;
// 	if (argc > 1) {
// 		string fileName(argv[1]);
// 		cout << "Scene path: " << fileName << endl << endl;
// 		scene_parser.LoadFromJson(fileName, scene, is_succeed);
// 		if (!is_succeed) {
// 			cout << endl;
// 			cout << "Scene path error!" << endl;
// 
// 			return 0;
// 		}
// 	}
// 	else {
// 		cout << "Scene path error!" << endl;
// 
// 		return 0;
// 	}
// 
// 	shared_ptr<Integrator> inte = NULL;
// 	if (scene_parser.inte_info.type == "path_tracing") {
// 		inte = make_shared<PathTracing>(make_shared<Scene>(scene), scene_parser.inte_info.depth,
// 			scene_parser.inte_info.sampler_type, scene_parser.inte_info.light_strategy);
// 	}
// 	else {
// 		cout << "Integrator error!" << endl;
// 
// 		return 0;
// 	}
// 
// 	auto render = make_shared<CPURender>(inte, scene_parser.use_denoise);
// 	render->Init();
// 	render->Run();
// 	render->Destory();

	auto scene = CornellBox();
	auto inte = make_shared<VolumetricPathTracing>(make_shared<Scene>(scene), 100, SamplerType::Independent, TraceLightType::ALL);

	auto render = make_shared<CPURender>(inte, 0);
	render->Init();
	render->Run();
	render->Destory();

	return 0;
}
