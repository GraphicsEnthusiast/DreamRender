#include <Utils.h>
#include <SceneParser.h>
#include <Render.h>

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

	shared_ptr<Integrator> inte = NULL;
	if (scene_parser.inte_info.type == "path_tracing") {
		inte = make_shared<PathTracing>(make_shared<Scene>(scene), scene_parser.inte_info.sampler_type, scene_parser.inte_info.light_strategy);
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
