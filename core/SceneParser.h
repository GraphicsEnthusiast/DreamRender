#pragma once

#include "Utils.h"
#include "Scene.h"

typedef nlohmann::json json;

inline bool HasExtension(const std::string& value, const std::string& ending) {
	if (ending.size() > value.size()) {
		return false;
	}

	return std::equal(
		ending.rbegin(), ending.rend(), value.rbegin(),
		[](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

inline json CreateJsonFromFile(const std::string& fn, bool& is_succeed) {
	std::ifstream fst;
	fst.open(fn.c_str());
	is_succeed = false;
	fst.exceptions(std::ifstream::failbit || std::ifstream::badbit);
	if (!fst.is_open()) {
		printf("Open scene json failed! \n");

		return json();
	}
	else {
		is_succeed = true;
		std::stringstream buffer;
		buffer << fst.rdbuf();
		std::string str = buffer.str();
		if (HasExtension(fn, "bson")) {
			return json::from_bson(str);
		}
		else {
			return json::parse(str);
		}
	}
}

class SceneParser {
public:
	SceneParser() = default;

	void LoadFromJson(const std::string& fn, bool& is_succeed);

	std::shared_ptr<Renderer> CreateRenderer();

private:
	void Parse(const json& data);

	void ParseCamera(const json& data);

	void ParseLights(const json& data);

	void ParseShapes(const json& data);

	void ParseMaterials(const json& data);

	void ParseMediums(const json& data);

	void ParseSampler(const json& data);

	void ParseFilter(const json& data);

	void ParseIntegrator(const json& data);

private:
	float width, height;
	std::vector<std::shared_ptr<Light>> lights;
	std::unordered_map<std::string, std::shared_ptr<Material>> materials;
	std::unordered_map<std::string, std::shared_ptr<Medium>> mediums;
	std::vector<Shape*> shapes;
	std::shared_ptr<Camera> camera;
	std::shared_ptr<Scene> scene;
	std::shared_ptr<Sampler> sampler;
	std::shared_ptr<Filter> filter;
	std::shared_ptr<Integrator> integrator;
	std::shared_ptr<PostProcessing> post;
};
