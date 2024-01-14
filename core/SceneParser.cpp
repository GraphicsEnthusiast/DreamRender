#include "SceneParser.h"

void SceneParser::LoadFromJson(const std::string& fn, bool& is_succeed) {
	json data = CreateJsonFromFile(fn, is_succeed);
	if (!is_succeed) {
		scene = NULL;

		return;
	}

	RTCDevice rtc_device = rtcNewDevice(NULL);
	scene = std::make_shared<Scene>(rtc_device);
	Parse(data);
}