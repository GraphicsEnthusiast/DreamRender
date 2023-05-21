#include <Utils.h>
#include <SceneParser.h>
#include <Render.h>

namespace PathTracingScene {
	Scene MitsubaKnob() {
		RTCDevice rtc_device = rtcNewDevice(NULL);
		Scene scene(rtc_device);

		shared_ptr<KullaConty> kulla_conty = make_shared<KullaConty>();
		auto diff = make_shared<SmoothDiffuse>(make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/default.png"));
		auto diff2 = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.5f)));
		auto diff3 = make_shared<SmoothDiffuse>(make_shared<CheckerTexture>(make_shared<ConstantTexture>(vec3(0.4f)), make_shared<ConstantTexture>(vec3(0.8f))));

		//-------------------------全部材质-------------------------
		auto material1 = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.1f, 0.8f, 0.5f)));
		auto material2 = make_shared<RoughDiffuse>(make_shared<ConstantTexture>(vec3(0.1f, 0.5f, 0.8f)), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/roughness.png"));
		auto material3 = make_shared<SmoothConductor>(make_shared<ConstantTexture>(vec3(0.8f, 0.85f, 0.88f)),
			vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
		auto material4 = make_shared<RoughConductor>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.8f, 0.85f, 0.88f)), make_shared<ConstantTexture>(vec3(0.2f)), make_shared<ConstantTexture>(vec3(0.0f)),
			vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
		auto material5 = make_shared<RoughConductor>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.8f, 0.85f, 0.88f)), make_shared<ConstantTexture>(vec3(0.4f)), make_shared<ConstantTexture>(vec3(0.8f)),
			vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
		auto material6 = make_shared<ThinDielectric>(make_shared<ConstantTexture>(vec3(1.0f)), 1.5f, 1.0f);
		auto material7 = make_shared<SmoothDielectric>(make_shared<ConstantTexture>(vec3(1.0f)), 1.5f, 1.0f);
		auto material8 = make_shared<RoughDielectric>(kulla_conty,
			make_shared<ConstantTexture>(vec3(1.0f)), make_shared<ConstantTexture>(vec3(0.2f)),
			make_shared<ConstantTexture>(vec3(0.0f)), 1.5f, 1.0f);
		auto material9 = make_shared<RoughDielectric>(kulla_conty,
			make_shared<ConstantTexture>(vec3(1.0f)), make_shared<ConstantTexture>(vec3(0.2f)),
			make_shared<ConstantTexture>(vec3(0.8f)), 1.5f, 1.0f);
		auto material10 = make_shared<SmoothPlastic>(make_shared<ConstantTexture>(vec3(0.1f, 0.27f, 0.36f)), make_shared<ConstantTexture>(vec3(1.0f)), 1.9f, 1.0f, true);
		auto material11 = make_shared<RoughPlastic>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.1f, 0.27f, 0.36f)), make_shared<ConstantTexture>(vec3(1.0f)),
			make_shared<ConstantTexture>(vec3(0.33f)), make_shared<ConstantTexture>(vec3(0.0f)),
			1.9f, 1.0f, true);
		auto material12 = make_shared<RoughPlastic>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.1f, 0.27f, 0.36f)), make_shared<ConstantTexture>(vec3(1.0f)),
			make_shared<ConstantTexture>(vec3(0.33f)), make_shared<ConstantTexture>(vec3(0.8f)),
			1.9f, 1.0f, true);
		auto material13 = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/albedo.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/roughness.png"),
			make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/metallic.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/normal.png"));
		auto material14 = make_shared<ClearcoatedConductor>(material4, make_shared<ConstantTexture>(vec3(0.2f)), make_shared<ConstantTexture>(vec3(0.4f)), 1.0f);
		auto material15 = make_shared<DisneyDiffuse>(make_shared<ConstantTexture>(vec3(0.82f, 0.67f, 0.16f)), make_shared<ConstantTexture>(vec3(0.5f)),
			make_shared<ConstantTexture>(vec3(0.5f)));
		auto material16 = make_shared<DisneyMetal>(make_shared<ConstantTexture>(vec3(0.82f, 0.67f, 0.16f)), make_shared<ConstantTexture>(vec3(0.1f)),
			make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)),
			make_shared<ConstantTexture>(vec3(0.5f)));
		auto material17 = make_shared<DisneyClearcoat>(make_shared<ConstantTexture>(vec3(0.5f)));
		auto material18 = make_shared<DisneyGlass>(make_shared<ConstantTexture>(vec3(0.82f, 0.67f, 0.16f)), make_shared<ConstantTexture>(vec3(0.1f)),
			make_shared<ConstantTexture>(vec3(0.5f)), 1.5f, 1.0f);
		auto material19 = make_shared<DisneySheen>(make_shared<ConstantTexture>(vec3(0.82f, 0.67f, 0.16f)), make_shared<ConstantTexture>(vec3(0.5f)));
		auto material20 = make_shared<DisneyPrinciple>(material15, material16, material17, material18, material19, make_shared<ConstantTexture>(vec3(0.5f)),
			make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)));

		string model1 = "../TestScene/MitsubaKnob/Model/in.obj";
		string model2 = "../TestScene/MitsubaKnob/Model/out.obj";
//		string model3 = "../TestScene/MitsubaKnob/Model/back.obj";
		mat4 model_trans1 = GetTransformMatrix(vec3(0.0f, -1.0f, 0.0f), vec3(0.0f), vec3(0.9f));
		mat4 model_trans2 = GetTransformMatrix(vec3(0.0f, -1.0f, 0.0f), vec3(0.0f), vec3(1.0f));

		scene.AddShape(new TriangleMesh(diff2, model1, model_trans1));
		scene.AddShape(new TriangleMesh(material20, model2, model_trans2));
		scene.AddShape(new Quad(diff3, vec3(-10.0f, -1.0f, -10.0f), vec3(20.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 20.0f)));

		scene.width = 1200;
		scene.height = 1000;
		scene.depth = 50;
		scene.SetCamera(make_shared<PinholeCamera>(vec3(0.0f, 1.5f, 3.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), 1.0f, 60.0f,
			static_cast<float>(scene.width) / static_cast<float>(scene.height)));
		scene.SetFilter(make_shared<FilterGaussian>());
		scene.SetHDR(make_shared<InfiniteAreaLight>(make_shared<HdrTexture>("../TestScene/MitsubaKnob/HDR/078.hdr")));

		scene.Commit();

		return scene;
	}
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
		auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(7.0f)));
		
		auto red = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.63f, 0.065f, 0.05f)));
		auto green = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.14f, 0.45f, 0.091f)));
		auto grey = make_shared<SmoothDiffuse>(make_shared<ConstantTexture>(vec3(0.725f, 0.710f, 0.680f)));
		auto diff = make_shared<SmoothDiffuse>(make_shared<CheckerTexture>(make_shared<ConstantTexture>(vec3(0.1f, 0.8f, 0.5f)), make_shared<ConstantTexture>(vec3(0.8f))));
		auto con = make_shared<SmoothConductor>(make_shared<ConstantTexture>(vec3(0.8f, 0.85f, 0.88f)),
			vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
		auto die = make_shared<SmoothDielectric>(make_shared<ConstantTexture>(vec3(1.0f)), 1.5f, 1.0f);
		auto spl = make_shared<SmoothPlastic>(make_shared<ConstantTexture>(vec3(0.2f, 0.54f, 0.72f)), make_shared<ConstantTexture>(vec3(1.0f)), 1.9f, 1.0f, true);
		auto met = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/albedo.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/roughness.png"),
			make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/metallic.png"), make_shared<ImageTexture>("../TestScene/MitsubaKnob/Texture/normal.png"));

		auto quad = new Quad(lightmat, vec3(-1.0f, 3.45f, -1.0f), vec3(2.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f));
		auto quadlight = make_shared<QuadLight>(quad);

		//灯必须在其他物体前加入————————————————————————————————————————————
		scene.AddLight(quadlight, quadlight->quad);

		scene.AddShape(new TriangleMesh(grey, back, model_box));
		scene.AddShape(new TriangleMesh(red, left, model_box));
		scene.AddShape(new TriangleMesh(green, right, model_box));
		scene.AddShape(new TriangleMesh(diff, bottom, model_box));
		scene.AddShape(new TriangleMesh(grey, top, model_box));
		scene.AddShape(new TriangleMesh(spl, ajax, model_ajax));

		scene.width = 1200;
		scene.height = 1000;
		scene.depth = 50;
		scene.SetCamera(make_shared<PinholeCamera>(vec3(0.0f, 1.0f, 7.5f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), 1.0f, 60.0f,
			static_cast<float>(scene.width) / static_cast<float>(scene.height)));
		scene.SetFilter(make_shared<FilterGaussian>());

		scene.Commit();

		return scene;
	}
	Scene Teapot() {
		RTCDevice rtc_device = rtcNewDevice(NULL);
		Scene scene(rtc_device);

		string teapot = "../TestScene/Teapot/Model/teapot.obj";
		string quad = "../TestScene/Teapot/Model/quad.obj";

		auto material1 = make_shared<DisneyDiffuse>(make_shared<ConstantTexture>(vec3(0.0f, 0.29f, 0.88f)), make_shared<ConstantTexture>(vec3(0.1f)),
			make_shared<ConstantTexture>(vec3(0.5f)));
		auto material2 = make_shared<DisneyMetal>(make_shared<ConstantTexture>(vec3(0.0f, 0.29f, 0.88f)), make_shared<ConstantTexture>(vec3(0.1f)),
			make_shared<ConstantTexture>(vec3(0.0f)), make_shared<ConstantTexture>(vec3(0.0f)), make_shared<ConstantTexture>(vec3(0.5f)),
			make_shared<ConstantTexture>(vec3(0.5f)));
		auto material3 = make_shared<DisneyClearcoat>(make_shared<ConstantTexture>(vec3(0.5f)));
		auto material4 = make_shared<DisneyGlass>(make_shared<ConstantTexture>(vec3(0.0f, 0.29f, 0.88f)), make_shared<ConstantTexture>(vec3(0.1f)),
			make_shared<ConstantTexture>(vec3(0.0f)), 1.5f, 1.0f);
		auto material5 = make_shared<DisneySheen>(make_shared<ConstantTexture>(vec3(0.0f, 0.29f, 0.88f)), make_shared<ConstantTexture>(vec3(0.5f)));
		auto material6 = make_shared<DisneyPrinciple>(material1, material2, material3, material4, material5, make_shared<ConstantTexture>(vec3(0.0f)),
			make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)), make_shared<ConstantTexture>(vec3(0.5f)));
		auto diff = make_shared<SmoothDiffuse>(make_shared<ImageTexture>("../TestScene/Teapot/Texture/checker.png"));
		auto diff2 = make_shared<SmoothDiffuse>(make_shared<CheckerTexture>(make_shared<ConstantTexture>(vec3(0.4f)), make_shared<ConstantTexture>(vec3(0.8f))));

		scene.AddShape(new TriangleMesh(material6, teapot, mat4(1.0f)));
		scene.AddShape(new Quad(diff, vec3(-20.0f, 0.0f, -20.0f), vec3(40.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 40.0f)));

		scene.width = 1200;
		scene.height = 1000;
		scene.depth = 5;
		scene.SetCamera(make_shared<ThinlensCamera>(vec3(20.0f, 12.0f, 0.0f), vec3(-0.953633f, 2.17253f, -0.0972613), vec3(0.0f, 1.0f, 0.0f), 1.0f, 60.0f,
			static_cast<float>(scene.width) / static_cast<float>(scene.height), 2.0f, length(vec3(20.0f, 12.0f, 0.0f) - vec3(-0.953633f, 2.17253f, -0.0972613))));
		scene.SetFilter(make_shared<FilterGaussian>());
		scene.SetHDR(make_shared<InfiniteAreaLight>(make_shared<HdrTexture>("../TestScene/Teapot/HDR/sunset.hdr"), 5.0f));

		scene.Commit();

		return scene;
	}
	Scene BoyHDR() {
		RTCDevice rtc_device = rtcNewDevice(NULL);
		Scene scene(rtc_device);

		string head = "../TestScene/Boy/Model/head.obj";
		string body = "../TestScene/Boy/Model/body.obj";
		string base = "../TestScene/Boy/Model/base.obj";

		mat4 model_trans = GetTransformMatrix(vec3(0.0f), vec3(0.0f, 90.0f, 0.0f), vec3(30.0f));

		shared_ptr<KullaConty> kulla_conty = make_shared<KullaConty>();
		auto diff = make_shared<SmoothDiffuse>(make_shared<ImageTexture>("../TestScene/Boy/Texture/grid.jpg"));
		auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(4.0f)));
		auto die = make_shared<RoughDielectric>(kulla_conty,
			make_shared<ConstantTexture>(vec3(1.0f)), make_shared<ConstantTexture>(vec3(0.2f)),
			make_shared<ConstantTexture>(vec3(0.0f)), 1.5f, 1.0f);
		auto con = make_shared<RoughConductor>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.8f, 0.85f, 0.88f)), make_shared<ConstantTexture>(vec3(0.2f)), make_shared<ConstantTexture>(vec3(0.0f)),
			vec3(0.14282f, 0.37414f, 1.43944f), vec3(3.97472f, 2.38066f, 1.59981f));
		auto spl = make_shared<RoughPlastic>(kulla_conty,
			make_shared<ConstantTexture>(vec3(0.63f, 0.065f, 0.05f)), make_shared<ConstantTexture>(vec3(1.0f)),
			make_shared<ConstantTexture>(vec3(0.2f)), make_shared<ConstantTexture>(vec3(0.0f)),
			1.5f, 1.0f, true);
		auto spl2 = make_shared<SmoothPlastic>(make_shared<ConstantTexture>(vec3(0.1f, 0.27f, 0.36f)), make_shared<ConstantTexture>(vec3(1.0f)), 1.9f, 1.0f, true);

		scene.AddShape(new TriangleMesh(con, head, model_trans));
		scene.AddShape(new TriangleMesh(die, body, model_trans));
		scene.AddShape(new TriangleMesh(spl, base, model_trans));
		scene.AddShape(new Quad(diff, vec3(-5.0f, 0.0f, -5.0f), vec3(10.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 10.0f)));

		scene.width = 1200;
		scene.height = 1000;
		scene.depth = 50;
		scene.SetCamera(make_shared<PinholeCamera>(vec3(3.0f, 6.5f, -11.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), 1.0f, 40.0f,
			static_cast<float>(scene.width) / static_cast<float>(scene.height)));
		scene.SetFilter(make_shared<FilterGaussian>());
		scene.SetHDR(make_shared<InfiniteAreaLight>(make_shared<HdrTexture>("../TestScene/Boy/HDR/spaichingen_hill_4k.hdr")));

		scene.Commit();

		return scene;
	}
	Scene BoyQuadLight() {
		RTCDevice rtc_device = rtcNewDevice(NULL);
		Scene scene(rtc_device);

		string head = "../TestScene/Boy/Model/head.obj";
		string body = "../TestScene/Boy/Model/body.obj";
		string base = "../TestScene/Boy/Model/base.obj";
		string background = "../TestScene/Boy/Model/background.obj";

		mat4 model_trans = GetTransformMatrix(vec3(0.0f), vec3(0.0f), vec3(30.0f));

		shared_ptr<KullaConty> kulla_conty = make_shared<KullaConty>();
		auto diff = make_shared<SmoothDiffuse>(make_shared<ImageTexture>("../TestScene/Boy/Texture/grid.jpg"));
		auto lightmat = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(3.0f)));
		auto lightmat2 = make_shared<DiffuseLight>(make_shared<ConstantTexture>(vec3(2.0f)));
		auto head_mat = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/Boy/Texture/01_Head_Base_Color.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/01_Head_MetallicRoughness.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/01_Head_Normal_DirectX.png"));
		auto body_mat = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/Boy/Texture/02_Body_Base_Color.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/02_Body_MetallicRoughness.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/02_Body_Normal_DirectX.png"));
		auto base_mat = make_shared<MetalWorkflow>(make_shared<ImageTexture>("../TestScene/Boy/Texture/03_Base_Base_Color.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/03_Base_MetallicRoughness.png"),
			make_shared<ImageTexture>("../TestScene/Boy/Texture/03_Base_Normal_DirectX.png"));

		scene.AddShape(new TriangleMesh(diff, background, model_trans));
		scene.AddShape(new TriangleMesh(head_mat, head, model_trans));
		scene.AddShape(new TriangleMesh(body_mat, body, model_trans));
		scene.AddShape(new TriangleMesh(base_mat, base, model_trans));

		auto quad1 = new Quad(lightmat, vec3(-3.0f, 6.0f, -3.0f), vec3(6.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 6.0f));
		auto quad2 = new Quad(lightmat2, vec3(-5.0f, 0.0f, 10.0f), vec3(0.0f, 10.0f, 0.0f), vec3(10.0f, 0.0f, 0.0f));
		auto quad3 = new Quad(lightmat2, vec3(-5.0f, 0.0f, -10.0f), vec3(10.0f, 0.0f, 0.0f), vec3(0.0f, 10.0f, 0.0f));

		auto light1 = make_shared<QuadLight>(quad1);
		auto light2 = make_shared<QuadLight>(quad2);
		auto light3 = make_shared<QuadLight>(quad3);

		scene.AddLight(light1, light1->shape);
		scene.AddLight(light2, light2->shape);
		scene.AddLight(light3, light3->shape);

		scene.width = 1200;
		scene.height = 1000;
		scene.depth = 50;
		scene.SetCamera(make_shared<PinholeCamera>(vec3(9.0f, 4.0f, 0.0f), vec3(0.0f, 2.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), 1.0f, 40.0f,
			static_cast<float>(scene.width) / static_cast<float>(scene.height)));
		scene.SetFilter(make_shared<FilterGaussian>());

		scene.Commit();

		return scene;
	}
}

int main(int argc, char** argv) {
	cout << "Hello, DreamRender!" << endl << endl;

	SceneParser scene_parser;
	RTCDevice rtc_device = rtcNewDevice(NULL);
	Scene scene(rtc_device);
	bool is_succeed;
	if (argc > 1) {
		string fileName(argv[1]);
		cout << "Scene path: " << fileName << endl << endl;
		scene_parser.LoadFromJson(fileName, scene, is_succeed);
		if (!is_succeed) {
			cout << endl;
			cout << "Scene path error!" << endl;

			return 0;
		}
	}
	else {
		cout << "Scene path error!" << endl;

		return 0;
	}

//	auto scene = PathTracingScene::MitsubaKnob();
//	auto scene = PathTracingScene::CornellBox();
//	auto scene = PathTracingScene::Teapot();
//	auto scene = PathTracingScene::BoyHDR();
//	auto scene = PathTracingScene::BoyQuadLight();
//	auto inte = make_shared<PathTracing>(make_shared<Scene>(scene), scene_parser.inte_info.light_strategy);

	shared_ptr<Integrator> inte = NULL;
	if (scene_parser.inte_info.type == "path_tracing") {
		inte = make_shared<PathTracing>(make_shared<Scene>(scene), scene_parser.inte_info.light_strategy);
	}
	else {
		cout << "Integrator error!" << endl;

		return 0;
	}
	auto render = make_shared<CPURender>(inte, scene_parser.use_denoise);
	render->Init();
	render->Run();
	render->Destory();

	return 0;
}
