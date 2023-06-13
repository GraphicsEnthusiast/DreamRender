#pragma once

#include <Utils.h>
#include <Camera.h>
#include <Integrator.h>
#include <nlohmann/json.hpp>

using namespace nlohmann;

inline bool HasExtension(const string& value, const string& ending) {
	if (ending.size() > value.size()) {
		return false;
	}

	return std::equal(
		ending.rbegin(), ending.rend(), value.rbegin(),
		[](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

inline json CreateJsonFromFile(const string& fn, bool& is_succeed) {
	std::ifstream fst;
	fst.open(fn.c_str());
	is_succeed = false;
	fst.exceptions(ifstream::failbit || ifstream::badbit);
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

struct IntegratorInfo {
	string type;
	int depth;
	TraceLightType light_strategy;
	SamplerType sampler_type;
};

class SceneParser {
public:
	SceneParser() = default;

	void LoadFromJson(const string& fn, Scene& scene, bool& is_succeed);
	void Parse(const json& data, Scene& scene);
	IntegratorInfo ParseIntegrator(const json& data);
	shared_ptr<Filter> ParseFilter(const json& data);
	shared_ptr<Camera> ParseCamera(const json& data, float aspect);
	void ParseLights(const json& data);
	void ParseShapes(const json& data);
	void ParseMaterials(const json& data);
	void ParseMediums(const json& data);

private:
	shared_ptr<Material> SearchMaterial(string name);
	shared_ptr<Medium> SearchMedium(string name);

public:
	IntegratorInfo inte_info;
	vector<shared_ptr<Light>> lights;
	vector<pair<string, shared_ptr<Material>>> materials;
	vector<pair<string, shared_ptr<Medium>>> mediums;
	vector<Shape*> shapes;
	shared_ptr<InfiniteAreaLight> env;
	bool use_denoise;
};
