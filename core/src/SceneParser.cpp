#include <SceneParser.h>

shared_ptr<Material> SceneParser::SearchMaterial(string name) {
	for (const auto& material : materials) {
		if (name == material.first) {
			//			cout << "找到" << endl;
			return material.second;
		}
	}
	cout << "Material loss!" << endl;

	return NULL;
}

void SceneParser::LoadFromJson(const string& fn, Scene& scene, bool& is_succeed) {
	json data = CreateJsonFromFile(fn, is_succeed);
	if (!is_succeed) {
		return;
	}
	Parse(data, scene);
}

void SceneParser::Parse(const json& data, Scene& scene) {
	env = NULL;

	int width = data.value("width", 1024);
	int height = data.value("height", 1024);
	use_denoise = data.value("denoise", false);
	cout << "Settings Information: " << endl;
	cout << "width: " << width << endl;
	cout << "height: " << height << endl;
	cout << "denoise: " << use_denoise << endl << endl;

	json inteData = data.value("integrator", json());
	inte_info = ParseIntegrator(inteData);

	json filterData = data.value("filter", json());
	auto filter = ParseFilter(data);
	if (filter == NULL) {
		cout << "Filter Error!" << endl;
	}

	json cameraData = data.value("camera", json());
	auto camera = ParseCamera(cameraData, static_cast<float>(width) / static_cast<float>(height));
	if (camera == NULL) {
		cout << "Camera Error!" << endl;
	}

	json materialData = data.value("material", json());
	ParseMaterials(materialData);

	json lightData = data.value("light", json());
	ParseLights(lightData);

	json shapeData = data.value("shape", json());
	ParseShapes(shapeData);

	float light_size = lights.size();
	if (env != NULL) {
		light_size++;
	}
	cout << "light_size: " << light_size << endl;
	cout << "shape_size: " << shapes.size() << endl;
	cout << "material_size: " << materials.size() << endl;

	scene.width = width;
	scene.height = height;
	scene.SetCamera(camera);
	scene.SetFilter(filter);
	scene.SetHDR(env);

	for (auto light : lights) {
		scene.AddLight(light, light->shape);
	}

	for (auto shape : shapes) {
		scene.AddShape(shape);
	}

	scene.Commit();
}

IntegratorInfo SceneParser::ParseIntegrator(const json& data) {
	cout << "Integrator Information: " << endl;
	string type = data.value("type", "path_tracing");
	int depth = data.value("depth", 15);
	string light_strategy = data.value("light_strategy", "random");
	string sampler_type = data.value("sampler_type", "sobol");

	cout << "type: " << type << endl;
	cout << "depth: " << depth << endl;
	cout << "light strategy: " << light_strategy << endl;
	cout << "sampler type: " << sampler_type << endl << endl;

	TraceLightType ls_type;
	if (light_strategy == "random") {
		ls_type = TraceLightType::RANDOM;
	}
	else if (light_strategy == "all") {
		ls_type = TraceLightType::ALL;
	}

	SamplerType sp_type;
	if (sampler_type == "sobol") {
		sp_type = SamplerType::SimpleSobol;
	}
	else if (sampler_type == "independent") {
		sp_type = SamplerType::Independent;
	}

	return { type, depth, ls_type, sp_type };
}

shared_ptr<Filter> SceneParser::ParseFilter(const json& data) {
	cout << "Filter Information: " << endl;
	string filterType = data.value("type", "gaussian");

	cout << "type: " << filterType << endl << endl;

	if (filterType == "gaussian") {
		return make_shared<FilterGaussian>();
	}
	else if (filterType == "triangle") {
		return make_shared<FilterTriangle>();
	}
	else if (filterType == "tent") {
		return make_shared<FilterTent>();
	}
	else if (filterType == "box") {
		return make_shared<FilterBox>();
	}

	return NULL;
}

shared_ptr<Camera> SceneParser::ParseCamera(const json& data, float aspect) {
	cout << "Camera Information: " << endl;
	string cameraType = data.value("type", "pinhole");
	cout << "type: " << cameraType << endl;

	json look_from_j = data.value("look_from",json());
	vec3 look_from(look_from_j.at(0), look_from_j.at(1), look_from_j.at(2));
	cout << "look_from: " << to_string(look_from) << endl;

	json look_at_j = data.value("look_at", json());
	vec3 look_at(look_at_j.at(0), look_at_j.at(1), look_at_j.at(2));
	cout << "look_at: " << to_string(look_at) << endl;

	json up_j = data.value("up", json());
	vec3 up(up_j.at(0), up_j.at(1), up_j.at(2));
	cout << "up: " << to_string(up) << endl;

	float znear = data.value("znear", 1.0f);
	cout << "znear: " << znear << endl;

	float fov = data.value("fov", 60.0f);
	cout << "fov: " << fov << endl;

	if (cameraType == "pinhole") {
		cout << "aspect: " << aspect << endl << endl;

		return make_shared<PinholeCamera>(look_from, look_at, up, znear, fov, aspect);
	}
	else if (cameraType == "thinlens") {
		cout << "aspect: " << aspect << endl;

		float aperture = data.value("aperture", 2.0f);
		cout << "aperture: " << aperture << endl;

		float focus_dist = length(look_from - look_at);
		cout << "focus_dist: " << focus_dist << endl << endl;
		
		return make_shared<ThinlensCamera>(look_from, look_at, up, znear, fov, aspect, aperture, focus_dist);
	}

	return NULL;
}

void SceneParser::ParseLights(const json& data) {
	cout << "Lights Information: " << endl;
	int i = 0;
	for (auto iter = data.cbegin(); iter != data.cend(); ++iter) {
		json light = data.at(i);
		string type = light.value("type", "");
		cout << "type: " << type << endl;
		if (type == "infinite") {
			string hdr_path = light.value("hdr_path", "");
			float scale = light.value("scale", 1.0f);

			cout << "hdr_path: " << hdr_path << endl;
			cout << "scale: " << scale << endl;

			env = make_shared<InfiniteAreaLight>(make_shared<HdrTexture>(hdr_path.c_str()), scale);
		}
		else if (type == "quad") {
			json pos_j = light.value("pos", json());
			vec3 pos(pos_j.at(0), pos_j.at(1), pos_j.at(2));
			cout << "pos: " << to_string(pos) << endl;

			json u_j = light.value("u", json());
			vec3 u(u_j.at(0), u_j.at(1), u_j.at(2));
			cout << "u: " << to_string(u) << endl;

			json v_j = light.value("v", json());
			vec3 v(v_j.at(0), v_j.at(1), v_j.at(2));
			cout << "v: " << to_string(v) << endl;

			string material_name = light.value("material", "");
			cout << "material: " << material_name << endl;

			auto mat = SearchMaterial(material_name);
			auto quad = new Quad(mat, pos, u, v);
			auto quad_light = make_shared<QuadLight>(quad);

			lights.push_back(quad_light);
		}
		else if (type == "sphere") {
			json center_j = light.value("center", json());
			vec3 center(center_j.at(0), center_j.at(1), center_j.at(2));
			cout << "center: " << to_string(center) << endl;

			float radius = light.value("radius", 0.0f);
			cout << "radius: " << radius << endl;

			string material_name = light.value("material", "");
			cout << "material: " << material_name << endl;

			auto mat = SearchMaterial(material_name);
			auto sphere = new Sphere(mat, center, radius);
			auto sphere_light = make_shared<SphereLight>(sphere);

			lights.push_back(sphere_light);
		}
		else if (type == "point") {
			json pos_j = light.value("pos", json());
			vec3 pos(pos_j.at(0), pos_j.at(1), pos_j.at(2));
			cout << "pos: " << to_string(pos) << endl;

			json intensity_j = light.value("intensity", json());
			vec3 intensity(intensity_j.at(0), intensity_j.at(1), intensity_j.at(2));
			cout << "intensity: " << to_string(intensity) << endl;

			auto point_light = make_shared<PointLight>(pos, intensity);

			lights.push_back(point_light);
		}
		else if (type == "direction") {
			json dir_j = light.value("dir", json());
			vec3 dir(dir_j.at(0), dir_j.at(1), dir_j.at(2));
			cout << "dir: " << to_string(dir) << endl;

			json radiance_j = light.value("radiance", json());
			vec3 radiance(radiance_j.at(0), radiance_j.at(1), radiance_j.at(2));
			cout << "radiance: " << to_string(radiance) << endl;

			auto direction_light = make_shared<DirectionLight>(dir, radiance);

			lights.push_back(direction_light);
		}

		cout << endl;
		i++;
	}
}

void SceneParser::ParseShapes(const json& data) {
	cout << "Shapes Information: " << endl;
	int i = 0;
	for (auto iter = data.cbegin(); iter != data.cend(); ++iter) {
		json shape = data.at(i);
		string type = shape.value("type", "");
		cout << "type: " << type << endl;
		if (type == "mesh") {
			string mesh_path = shape.value("mesh_path", "");
			cout << "mesh_path: " << mesh_path << endl;

			json translate_j = shape.value("translate", json());
			vec3 translate(translate_j.at(0), translate_j.at(1), translate_j.at(2));
			cout << "translate: " << to_string(translate) << endl;

			json rotate_j = shape.value("rotate", json());
			vec3 rotate(rotate_j.at(0), rotate_j.at(1), rotate_j.at(2));
			cout << "rotate: " << to_string(rotate) << endl;

			json scale_j = shape.value("scale", json());
			vec3 scale(scale_j.at(0), scale_j.at(1), scale_j.at(2));
			cout << "scale: " << to_string(scale) << endl;

			string material_name = shape.value("material", "");
			cout << "material: " << material_name << endl;

			mat4 model_tran = GetTransformMatrix(translate, rotate, scale);
			auto mat = SearchMaterial(material_name);

			auto mesh = new TriangleMesh(mat, mesh_path, model_tran);

			shapes.push_back(mesh);
		}
		else if (type == "quad") {
			json pos_j = shape.value("pos", json());
			vec3 pos(pos_j.at(0), pos_j.at(1), pos_j.at(2));
			cout << "pos: " << to_string(pos) << endl;

			json u_j = shape.value("u", json());
			vec3 u(u_j.at(0), u_j.at(1), u_j.at(2));
			cout << "u: " << to_string(u) << endl;

			json v_j = shape.value("v", json());
			vec3 v(v_j.at(0), v_j.at(1), v_j.at(2));
			cout << "v: " << to_string(v) << endl;

			string material_name = shape.value("material", "");
			cout << "material: " << material_name << endl;

			auto mat = SearchMaterial(material_name);
			auto quad = new Quad(mat, pos, u, v);

			shapes.push_back(quad);
		}
		else if (type == "sphere") {
			json center_j = shape.value("center", json());
			vec3 center(center_j.at(0), center_j.at(1), center_j.at(2));
			cout << "center: " << to_string(center) << endl;

			float radius = shape.value("radius", 0.0f);
			cout << "radius: " << radius << endl;

			string material_name = shape.value("material", "");
			cout << "material: " << material_name << endl;

			auto mat = SearchMaterial(material_name);
			auto sphere = new Sphere(mat, center, radius);

			shapes.push_back(sphere);
		}

		cout << endl;
		i++;
	}
}

void SceneParser::ParseMaterials(const json& data) {
	cout << "Materials Information: " << endl;
	int i = 0;
	for (auto iter = data.cbegin(); iter != data.cend(); ++iter) {
		json material = data.at(i);
		string type = material.value("type", "");
		cout << "type: " << type << endl;

		shared_ptr<Material> mat;
		shared_ptr<KullaConty> kulla_conty = make_shared<KullaConty>();
		shared_ptr<Texture> emitted_texture;
		shared_ptr<Texture> albedo_texture;
		shared_ptr<Texture> diffuse_texture;
		shared_ptr<Texture> specular_texture;
		shared_ptr<Texture> roughness_texture;
		shared_ptr<Texture> anisotropy_texture;
		shared_ptr<Texture> normal_texture;
		shared_ptr<Texture> metallic_texture;
		shared_ptr<Texture> metallicRoughness_texture;
		vec3 eta;
		vec3 k;
		shared_ptr<Texture> subsurface_texture;
		shared_ptr<Texture> specularTint_texture;
		shared_ptr<Texture> clearcoatGloss_texture;
		shared_ptr<Texture> sheenTint_texture;
		shared_ptr<Texture> sheen_texture;
		shared_ptr<Texture> clearcoat_texture;
		shared_ptr<Texture> specularTransmission_texture;

		if (type == "diffuse_light") {
			string name = material.value("name", "");
			cout << "name: " << name << endl;

			string emitted_texture_type = material.value("emitted_texture_type", "");
			cout << "emitted_texture_type: " << emitted_texture_type << endl;

			if (emitted_texture_type == "constant") {
				json emitted_j = material.value("emitted", json());
				vec3 emitted(emitted_j.at(0), emitted_j.at(1), emitted_j.at(2));
				cout << "emitted: " << to_string(emitted) << endl;

				emitted_texture = make_shared<ConstantTexture>(emitted);
				mat = make_shared<DiffuseLight>(emitted_texture);

				materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
			}
		}
		else if (type == "smooth_diffuse") {
			 string name = material.value("name", "");
			 cout << "name: " << name << endl;
			 
			 string albedo_texture_type = material.value("albedo_texture_type", "");
			 cout << "albedo_texture_type: " << albedo_texture_type << endl;
			 
			 if (albedo_texture_type == "constant") {
			 	json albedo_j = material.value("albedo", json());
			 	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
			 	cout << "albedo: " << to_string(albedo) << endl;

				albedo_texture = make_shared<ConstantTexture>(albedo);
			 }
			 else if (albedo_texture_type == "image") {
				 string albedo_path = material.value("albedo_path", "");
				 cout << "albedo_path: " << albedo_path << endl;

				 albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
			 }
			 mat = make_shared<SmoothDiffuse>(albedo_texture);
			 materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "rough_diffuse") {
			string name = material.value("name", "");
			cout << "name: " << name << endl;

			string albedo_texture_type = material.value("albedo_texture_type", "");
			cout << "albedo_texture_type: " << albedo_texture_type << endl;

			if (albedo_texture_type == "constant") {
				json albedo_j = material.value("albedo", json());
				vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
				cout << "albedo: " << to_string(albedo) << endl;

				albedo_texture = make_shared<ConstantTexture>(albedo);
			}
			else if (albedo_texture_type == "image") {
				string albedo_path = material.value("albedo_path", "");
				cout << "albedo_path: " << albedo_path << endl;

				albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
			}

			string roughness_texture_type = material.value("roughness_texture_type", "");
			cout << "roughness_texture_type: " << roughness_texture_type << endl;

			if (roughness_texture_type == "constant") {
				float roughness = material.value("roughness", 0.1f);
				cout << "roughness: " << roughness << endl;

				roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
			}
			else if (roughness_texture_type == "image") {
				string roughness_path = material.value("roughness_path", "");
				cout << "roughness_path: " << roughness_path << endl;

				roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
			}
			auto mat = make_shared<RoughDiffuse>(albedo_texture, roughness_texture);
			materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "smooth_conductor") {
			string name = material.value("name", "");
			cout << "name: " << name << endl;

			string albedo_texture_type = material.value("albedo_texture_type", "");
			cout << "albedo_texture_type: " << albedo_texture_type << endl;

			if (albedo_texture_type == "constant") {
				json albedo_j = material.value("albedo", json());
				vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
				cout << "albedo: " << to_string(albedo) << endl;

				albedo_texture = make_shared<ConstantTexture>(albedo);
			}
			else if (albedo_texture_type == "image") {
				string albedo_path = material.value("albedo_path", "");
				cout << "albedo_path: " << albedo_path << endl;

				albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
			}

			normal_texture = NULL;
			string normal_texture_type = material.value("normal_texture_type", "");
			cout << "normal_texture_type: " << normal_texture_type << endl;
			if (normal_texture_type != "" && normal_texture_type == "image") {
				string normal_path = material.value("normal_path", "");
				cout << "normal_path: " << normal_path << endl;

				normal_texture = make_shared<ImageTexture>(normal_path.c_str());
			}

			json eta_j = material.value("eta", json());
			vec3 eta(eta_j.at(0), eta_j.at(1), eta_j.at(2));
			cout << "eta: " << to_string(eta) << endl;

			json k_j = material.value("k", json());
			vec3 k(k_j.at(0), k_j.at(1), k_j.at(2));
			cout << "k: " << to_string(k) << endl;

			mat = make_shared<SmoothConductor>(albedo_texture, eta, k, normal_texture);
			materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "rough_conductor") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }
		    
		    string roughness_texture_type = material.value("roughness_texture_type", "");
		    cout << "roughness_texture_type: " << roughness_texture_type << endl;
		    
		    if (roughness_texture_type == "constant") {
		    	float roughness = material.value("roughness", 0.1f);
		    	cout << "roughness: " << roughness << endl;
		    
		    	roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
		    }
		    else if (roughness_texture_type == "image") {
		    	string roughness_path = material.value("roughness_path", "");
		    	cout << "roughness_path: " << roughness_path << endl;
		    
		    	roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
		    }

			string anisotropy_texture_type = material.value("anisotropy_texture_type", "");
			cout << "anisotropy_texture_type: " << anisotropy_texture_type << endl;

			if (anisotropy_texture_type == "constant") {
				float anisotropy = material.value("anisotropy", 0.1f);
				cout << "anisotropy: " << anisotropy << endl;

				anisotropy_texture = make_shared<ConstantTexture>(vec3(anisotropy));
			}
			else if (anisotropy_texture_type == "image") {
				string anisotropy_path = material.value("anisotropy_path", "");
				cout << "anisotropy_path: " << anisotropy_path << endl;

				anisotropy_texture = make_shared<ImageTexture>(anisotropy_path.c_str());
			}
		    
		    json eta_j = material.value("eta", json());
		    vec3 eta(eta_j.at(0), eta_j.at(1), eta_j.at(2));
		    cout << "eta: " << to_string(eta) << endl;
		    
		    json k_j = material.value("k", json());
		    vec3 k(k_j.at(0), k_j.at(1), k_j.at(2));
		    cout << "k: " << to_string(k) << endl;
		    
		    mat = make_shared<RoughConductor>(kulla_conty, albedo_texture, roughness_texture, anisotropy_texture, eta, k);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "smooth_dielectric") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }

			normal_texture = NULL;
			string normal_texture_type = material.value("normal_texture_type", "");
			cout << "normal_texture_type: " << normal_texture_type << endl;
			if (normal_texture_type != "" && normal_texture_type == "image") {
				string normal_path = material.value("normal_path", "");
				cout << "normal_path: " << normal_path << endl;

				normal_texture = make_shared<ImageTexture>(normal_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;
		    
		    mat = make_shared<SmoothDielectric>(albedo_texture, int_ior, ext_ior, normal_texture);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "rough_dielectric") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }

			string roughness_texture_type = material.value("roughness_texture_type", "");
			cout << "roughness_texture_type: " << roughness_texture_type << endl;

			if (roughness_texture_type == "constant") {
				float roughness = material.value("roughness", 0.1f);
				cout << "roughness: " << roughness << endl;

				roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
			}
			else if (roughness_texture_type == "image") {
				string roughness_path = material.value("roughness_path", "");
				cout << "roughness_path: " << roughness_path << endl;

				roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
			}

			string anisotropy_texture_type = material.value("anisotropy_texture_type", "");
			cout << "anisotropy_texture_type: " << anisotropy_texture_type << endl;

			if (anisotropy_texture_type == "constant") {
				float anisotropy = material.value("anisotropy", 0.1f);
				cout << "anisotropy: " << anisotropy << endl;

				anisotropy_texture = make_shared<ConstantTexture>(vec3(anisotropy));
			}
			else if (anisotropy_texture_type == "image") {
				string anisotropy_path = material.value("anisotropy_path", "");
				cout << "anisotropy_path: " << anisotropy_path << endl;

				anisotropy_texture = make_shared<ImageTexture>(anisotropy_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;
		    
		    mat = make_shared<RoughDielectric>(kulla_conty, albedo_texture, roughness_texture, anisotropy_texture, int_ior, ext_ior);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "thin_dielectric") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }

			normal_texture = NULL;
			string normal_texture_type = material.value("normal_texture_type", "");
			cout << "normal_texture_type: " << normal_texture_type << endl;
			if (normal_texture_type != "" && normal_texture_type == "image") {
				string normal_path = material.value("normal_path", "");
				cout << "normal_path: " << normal_path << endl;

				normal_texture = make_shared<ImageTexture>(normal_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;
		    
		    mat = make_shared<ThinDielectric>(albedo_texture, int_ior, ext_ior, normal_texture);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "smooth_plastic") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string diffuse_texture_type = material.value("diffuse_texture_type", "");
		    cout << "diffuse_texture_type: " << diffuse_texture_type << endl;
		    
		    if (diffuse_texture_type == "constant") {
		    	json diffuse_j = material.value("diffuse", json());
		    	vec3 diffuse(diffuse_j.at(0), diffuse_j.at(1), diffuse_j.at(2));
		    	cout << "diffuse: " << to_string(diffuse) << endl;
		    
		    	diffuse_texture = make_shared<ConstantTexture>(diffuse);
		    }
		    else if (diffuse_texture_type == "image") {
		    	string diffuse_path = material.value("diffuse_path", "");
		    	cout << "diffuse_path: " << diffuse_path << endl;
		    
		    	diffuse_texture = make_shared<ImageTexture>(diffuse_path.c_str());
		    }

			string specular_texture_type = material.value("specular_texture_type", "");
			cout << "specular_texture_type: " << specular_texture_type << endl;

			if (specular_texture_type == "constant") {
				json specular_j = material.value("specular", json());
				vec3 specular(specular_j.at(0), specular_j.at(1), specular_j.at(2));
				cout << "specular: " << to_string(specular) << endl;

				specular_texture = make_shared<ConstantTexture>(specular);
			}
			else if (specular_texture_type == "image") {
				string specular_path = material.value("specular_path", "");
				cout << "specular_path: " << specular_path << endl;

				specular_texture = make_shared<ImageTexture>(specular_path.c_str());
			}

			normal_texture = NULL;
			string normal_texture_type = material.value("normal_texture_type", "");
			cout << "normal_texture_type: " << normal_texture_type << endl;
			if (normal_texture_type != "" && normal_texture_type == "image") {
				string normal_path = material.value("normal_path", "");
				cout << "normal_path: " << normal_path << endl;

				normal_texture = make_shared<ImageTexture>(normal_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;

			float nonlinear = material.value("nonlinear", true);
			cout << "nonlinear: " << nonlinear << endl;
		    
		    mat = make_shared<SmoothPlastic>(diffuse_texture, specular_texture, int_ior, ext_ior, nonlinear, normal_texture);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "rough_plastic") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string diffuse_texture_type = material.value("diffuse_texture_type", "");
		    cout << "diffuse_texture_type: " << diffuse_texture_type << endl;
		    
		    if (diffuse_texture_type == "constant") {
		    	json diffuse_j = material.value("diffuse", json());
		    	vec3 diffuse(diffuse_j.at(0), diffuse_j.at(1), diffuse_j.at(2));
		    	cout << "diffuse: " << to_string(diffuse) << endl;
		    
		    	diffuse_texture = make_shared<ConstantTexture>(diffuse);
		    }
		    else if (diffuse_texture_type == "image") {
		    	string diffuse_path = material.value("diffuse_path", "");
		    	cout << "diffuse_path: " << diffuse_path << endl;
		    
		    	diffuse_texture = make_shared<ImageTexture>(diffuse_path.c_str());
		    }
		    
		    string specular_texture_type = material.value("specular_texture_type", "");
		    cout << "specular_texture_type: " << specular_texture_type << endl;
		    
		    if (specular_texture_type == "constant") {
		    	json specular_j = material.value("specular", json());
		    	vec3 specular(specular_j.at(0), specular_j.at(1), specular_j.at(2));
		    	cout << "specular: " << to_string(specular) << endl;
		    
		    	specular_texture = make_shared<ConstantTexture>(specular);
		    }
		    else if (specular_texture_type == "image") {
		    	string specular_path = material.value("specular_path", "");
		    	cout << "specular_path: " << specular_path << endl;
		    
		    	specular_texture = make_shared<ImageTexture>(specular_path.c_str());
		    }

			string roughness_texture_type = material.value("roughness_texture_type", "");
			cout << "roughness_texture_type: " << roughness_texture_type << endl;

			if (roughness_texture_type == "constant") {
				float roughness = material.value("roughness", 0.1f);
				cout << "roughness: " << roughness << endl;

				roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
			}
			else if (roughness_texture_type == "image") {
				string roughness_path = material.value("roughness_path", "");
				cout << "roughness_path: " << roughness_path << endl;

				roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
			}

			string anisotropy_texture_type = material.value("anisotropy_texture_type", "");
			cout << "anisotropy_texture_type: " << anisotropy_texture_type << endl;

			if (anisotropy_texture_type == "constant") {
				float anisotropy = material.value("anisotropy", 0.1f);
				cout << "anisotropy: " << anisotropy << endl;

				anisotropy_texture = make_shared<ConstantTexture>(vec3(anisotropy));
			}
			else if (anisotropy_texture_type == "image") {
				string anisotropy_path = material.value("anisotropy_path", "");
				cout << "anisotropy_path: " << anisotropy_path << endl;

				anisotropy_texture = make_shared<ImageTexture>(anisotropy_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;
		    
		    float nonlinear = material.value("nonlinear", true);
		    cout << "nonlinear: " << nonlinear << endl;
		    
		    mat = make_shared<RoughPlastic>(kulla_conty, diffuse_texture, specular_texture, roughness_texture, anisotropy_texture,
				int_ior, ext_ior, nonlinear);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "clearcoated_conductor") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }
		    
		    string roughness_texture_type = material.value("roughness_texture_type", "");
		    cout << "roughness_texture_type: " << roughness_texture_type << endl;
		    
		    if (roughness_texture_type == "constant") {
		    	float roughness = material.value("roughness", 0.1f);
		    	cout << "roughness: " << roughness << endl;
		    
		    	roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
		    }
		    else if (roughness_texture_type == "image") {
		    	string roughness_path = material.value("roughness_path", "");
		    	cout << "roughness_path: " << roughness_path << endl;
		    
		    	roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
		    }
		    
		    string anisotropy_texture_type = material.value("anisotropy_texture_type", "");
		    cout << "anisotropy_texture_type: " << anisotropy_texture_type << endl;
		    
		    if (anisotropy_texture_type == "constant") {
		    	float anisotropy = material.value("anisotropy", 0.1f);
		    	cout << "anisotropy: " << anisotropy << endl;
		    
		    	anisotropy_texture = make_shared<ConstantTexture>(vec3(anisotropy));
		    }
		    else if (anisotropy_texture_type == "image") {
		    	string anisotropy_path = material.value("anisotropy_path", "");
		    	cout << "anisotropy_path: " << anisotropy_path << endl;
		    
		    	anisotropy_texture = make_shared<ImageTexture>(anisotropy_path.c_str());
		    }
		    
		    json eta_j = material.value("eta", json());
		    vec3 eta(eta_j.at(0), eta_j.at(1), eta_j.at(2));
		    cout << "eta: " << to_string(eta) << endl;
		    
		    json k_j = material.value("k", json());
		    vec3 k(k_j.at(0), k_j.at(1), k_j.at(2));
		    cout << "k: " << to_string(k) << endl;
		    
		    string roughness_u_texture_type = material.value("roughness_u_texture_type", "");
		    shared_ptr<Texture> roughness_u_texture;
		    cout << "roughness_u_texture_type: " << roughness_u_texture_type << endl;
		    
		    if (roughness_u_texture_type == "constant") {
		    	float roughness_u = material.value("roughness_u", 0.1f);
		    	cout << "roughness_u: " << roughness_u << endl;
		    
		    	roughness_u_texture = make_shared<ConstantTexture>(vec3(roughness_u));
		    }
		    else if (roughness_u_texture_type == "image") {
		    	string roughness_u_path = material.value("roughness_u_path", "");
		    	cout << "roughness_u_path: " << roughness_u_path << endl;
		    
		    	roughness_u_texture = make_shared<ImageTexture>(roughness_u_path.c_str());
		    }
		    
		    string roughness_v_texture_type = material.value("roughness_v_texture_type", "");
		    shared_ptr<Texture> roughness_v_texture;
		    cout << "roughness_v_texture_type: " << roughness_v_texture_type << endl;
		    
		    if (roughness_v_texture_type == "constant") {
		    	float roughness_v = material.value("roughness_v", 0.1f);
		    	cout << "roughness_v: " << roughness_v << endl;
		    
		    	roughness_v_texture = make_shared<ConstantTexture>(vec3(roughness_v));
		    }
		    else if (roughness_v_texture_type == "image") {
		    	string roughness_v_path = material.value("roughness_v_path", "");
		    	cout << "roughness_v_path: " << roughness_v_path << endl;
		    
		    	roughness_v_texture = make_shared<ImageTexture>(roughness_v_path.c_str());
		    }

			float clear_coat = material.value("clear_coat", 1.0f);
		    
		    auto con_mat = make_shared<RoughConductor>(kulla_conty, albedo_texture, roughness_texture, anisotropy_texture, eta, k);
		    
		    mat = make_shared<ClearcoatedConductor>(con_mat, roughness_u_texture, roughness_v_texture, clear_coat);
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "metal_workflow") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }

			string metallicRoughness_texture_type = material.value("metallicRoughness_texture_type", "");
			cout << "metallicRoughness_texture_type: " << metallicRoughness_texture_type << endl;

			if (metallicRoughness_texture_type == "") {
				string roughness_texture_type = material.value("roughness_texture_type", "");
				cout << "roughness_texture_type: " << roughness_texture_type << endl;

				if (roughness_texture_type == "constant") {
					float roughness = material.value("roughness", 0.1f);
					cout << "roughness: " << roughness << endl;

					roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
				}
				else if (roughness_texture_type == "image") {
					string roughness_path = material.value("roughness_path", "");
					cout << "roughness_path: " << roughness_path << endl;

					roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
				}

				string metallic_texture_type = material.value("metallic_texture_type", "");
				cout << "metallic_texture_type: " << metallic_texture_type << endl;

				if (metallic_texture_type == "constant") {
					float metallic = material.value("metallic", 0.1f);
					cout << "metallic: " << metallic << endl;

					metallic_texture = make_shared<ConstantTexture>(vec3(metallic));
				}
				else if (metallic_texture_type == "image") {
					string metallic_path = material.value("metallic_path", "");
					cout << "metallic_path: " << metallic_path << endl;

					metallic_texture = make_shared<ImageTexture>(metallic_path.c_str());
				}
			}
			else if (metallicRoughness_texture_type == "image") {
				string metallicRoughness_path = material.value("metallicRoughness_path", "");
				cout << "metallicRoughness_path: " << metallicRoughness_path << endl;

				metallicRoughness_texture = make_shared<ImageTexture>(metallicRoughness_path.c_str());
			}
		    
			normal_texture = NULL;
			string normal_texture_type = material.value("normal_texture_type", "");
			cout << "normal_texture_type: " << normal_texture_type << endl;
			if (normal_texture_type != "" && normal_texture_type == "image") {
				string normal_path = material.value("normal_path", "");
				cout << "normal_path: " << normal_path << endl;

				normal_texture = make_shared<ImageTexture>(normal_path.c_str());
			}
			if (metallicRoughness_texture_type != "image") {
				mat = make_shared<MetalWorkflow>(albedo_texture, roughness_texture, metallic_texture, normal_texture);
			}
			else {
				mat = make_shared<MetalWorkflow>(albedo_texture, metallicRoughness_texture, normal_texture);
			}
		    
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}
		else if (type == "disney_principle") {
		    string name = material.value("name", "");
		    cout << "name: " << name << endl;
		    
		    string albedo_texture_type = material.value("albedo_texture_type", "");
		    cout << "albedo_texture_type: " << albedo_texture_type << endl;
		    
		    if (albedo_texture_type == "constant") {
		    	json albedo_j = material.value("albedo", json());
		    	vec3 albedo(albedo_j.at(0), albedo_j.at(1), albedo_j.at(2));
		    	cout << "albedo: " << to_string(albedo) << endl;
		    
		    	albedo_texture = make_shared<ConstantTexture>(albedo);
		    }
		    else if (albedo_texture_type == "image") {
		    	string albedo_path = material.value("albedo_path", "");
		    	cout << "albedo_path: " << albedo_path << endl;
		    
		    	albedo_texture = make_shared<ImageTexture>(albedo_path.c_str());
		    }

			string sheen_texture_type = material.value("sheen_texture_type", "");
			cout << "sheen_texture_type: " << sheen_texture_type << endl;

			if (sheen_texture_type == "constant") {
				json sheen_j = material.value("sheen", json());
				vec3 sheen(sheen_j.at(0), sheen_j.at(1), sheen_j.at(2));
				cout << "sheen: " << to_string(sheen) << endl;

				sheen_texture = make_shared<ConstantTexture>(sheen);
			}
			else if (sheen_texture_type == "image") {
				string sheen_path = material.value("sheen_path", "");
				cout << "sheen_path: " << sheen_path << endl;

				sheen_texture = make_shared<ImageTexture>(sheen_path.c_str());
			}

			string specular_texture_type = material.value("specular_texture_type", "");
			cout << "specular_texture_type: " << specular_texture_type << endl;

			if (specular_texture_type == "constant") {
				json specular_j = material.value("specular", json());
				vec3 specular(specular_j.at(0), specular_j.at(1), specular_j.at(2));
				cout << "specular: " << to_string(specular) << endl;

				specular_texture = make_shared<ConstantTexture>(specular);
			}
			else if (specular_texture_type == "image") {
				string specular_path = material.value("specular_path", "");
				cout << "specular_path: " << specular_path << endl;

				specular_texture = make_shared<ImageTexture>(specular_path.c_str());
			}

			string specularTint_texture_type = material.value("specularTint_texture_type", "");
			cout << "specularTint_texture_type: " << specularTint_texture_type << endl;

			if (specularTint_texture_type == "constant") {
		    	json specularTint_j = material.value("specularTint", json());
		    	vec3 specularTint(specularTint_j.at(0), specularTint_j.at(1), specularTint_j.at(2));
		    	cout << "specularTint: " << to_string(specularTint) << endl;
		    
		    	specularTint_texture = make_shared<ConstantTexture>(specularTint);
		    }
		    else if (specularTint_texture_type == "image") {
		    	string specularTint_path = material.value("specularTint_path", "");
		    	cout << "specularTint_path: " << specularTint_path << endl;
		    
		    	specularTint_texture = make_shared<ImageTexture>(specularTint_path.c_str());
		    }
		    
		    string roughness_texture_type = material.value("roughness_texture_type", "");
		    cout << "roughness_texture_type: " << roughness_texture_type << endl;
		    
		    if (roughness_texture_type == "constant") {
		    	float roughness = material.value("roughness", 0.1f);
		    	cout << "roughness: " << roughness << endl;
		    
		    	roughness_texture = make_shared<ConstantTexture>(vec3(roughness));
		    }
		    else if (roughness_texture_type == "image") {
		    	string roughness_path = material.value("roughness_path", "");
		    	cout << "roughness_path: " << roughness_path << endl;
		    
		    	roughness_texture = make_shared<ImageTexture>(roughness_path.c_str());
		    }
		    
		    string anisotropy_texture_type = material.value("anisotropy_texture_type", "");
		    cout << "anisotropy_texture_type: " << anisotropy_texture_type << endl;
		    
		    if (anisotropy_texture_type == "constant") {
		    	float anisotropy = material.value("anisotropy", 0.1f);
		    	cout << "anisotropy: " << anisotropy << endl;
		    
		    	anisotropy_texture = make_shared<ConstantTexture>(vec3(anisotropy));
		    }
		    else if (anisotropy_texture_type == "image") {
		    	string anisotropy_path = material.value("anisotropy_path", "");
		    	cout << "anisotropy_path: " << anisotropy_path << endl;
		    
		    	anisotropy_texture = make_shared<ImageTexture>(anisotropy_path.c_str());
		    }

			string subsurface_texture_type = material.value("subsurface_texture_type", "");
			cout << "subsurface_texture_type: " << subsurface_texture_type << endl;

			if (subsurface_texture_type == "constant") {
				float subsurface = material.value("subsurface", 0.1f);
				cout << "subsurface: " << subsurface << endl;

				subsurface_texture = make_shared<ConstantTexture>(vec3(subsurface));
			}
			else if (subsurface_texture_type == "image") {
				string subsurface_path = material.value("subsurface_path", "");
				cout << "subsurface_path: " << subsurface_path << endl;

				subsurface_texture = make_shared<ImageTexture>(subsurface_path.c_str());
			}

			string metallic_texture_type = material.value("metallic_texture_type", "");
			cout << "metallic_texture_type: " << metallic_texture_type << endl;

			if (metallic_texture_type == "constant") {
				float metallic = material.value("metallic", 0.1f);
				cout << "metallic: " << metallic << endl;

				metallic_texture = make_shared<ConstantTexture>(vec3(metallic));
			}
			else if (metallic_texture_type == "image") {
				string metallic_path = material.value("metallic_path", "");
				cout << "metallic_path: " << metallic_path << endl;

				metallic_texture = make_shared<ImageTexture>(metallic_path.c_str());
			}

			string clearcoat_texture_type = material.value("clearcoat_texture_type", "");
			cout << "clearcoat_texture_type: " << clearcoat_texture_type << endl;

			if (clearcoat_texture_type == "constant") {
				float clearcoat = material.value("clearcoat", 0.1f);
				cout << "clearcoat: " << clearcoat << endl;

				clearcoat_texture = make_shared<ConstantTexture>(vec3(clearcoat));
			}
			else if (clearcoat_texture_type == "image") {
				string clearcoat_path = material.value("clearcoat_path", "");
				cout << "clearcoat_path: " << clearcoat_path << endl;

				clearcoat_texture = make_shared<ImageTexture>(clearcoat_path.c_str());
			}

			string specularTransmission_texture_type = material.value("specularTransmission_texture_type", "");
			cout << "specularTransmission_texture_type: " << specularTransmission_texture_type << endl;

			if (specularTransmission_texture_type == "constant") {
				float specularTransmission = material.value("specularTransmission", 0.1f);
				cout << "specularTransmission: " << specularTransmission << endl;

				specularTransmission_texture = make_shared<ConstantTexture>(vec3(specularTransmission));
			}
			else if (specularTransmission_texture_type == "image") {
				string specularTransmission_path = material.value("specularTransmission_path", "");
				cout << "specularTransmission_path: " << specularTransmission_path << endl;

				specularTransmission_texture = make_shared<ImageTexture>(specularTransmission_path.c_str());
			}

			string clearcoatGloss_texture_type = material.value("clearcoatGloss_texture_type", "");
			cout << "clearcoatGloss_texture_type: " << clearcoatGloss_texture_type << endl;

			if (clearcoatGloss_texture_type == "constant") {
				float clearcoatGloss = material.value("clearcoatGloss", 0.1f);
				cout << "clearcoatGloss: " << clearcoatGloss << endl;

				clearcoatGloss_texture = make_shared<ConstantTexture>(vec3(clearcoatGloss));
			}
			else if (clearcoatGloss_texture_type == "image") {
				string clearcoatGloss_path = material.value("clearcoatGloss_path", "");
				cout << "clearcoatGloss_path: " << clearcoatGloss_path << endl;

				clearcoatGloss_texture = make_shared<ImageTexture>(clearcoatGloss_path.c_str());
			}

			string sheenTint_texture_type = material.value("sheenTint_texture_type", "");
			cout << "sheenTint_texture_type: " << sheenTint_texture_type << endl;

			if (sheenTint_texture_type == "constant") {
				float sheenTint = material.value("sheenTint", 0.1f);
				cout << "sheenTint: " << sheenTint << endl;

				sheenTint_texture = make_shared<ConstantTexture>(vec3(sheenTint));
			}
			else if (sheenTint_texture_type == "image") {
				string sheenTint_path = material.value("sheenTint_path", "");
				cout << "sheenTint_path: " << sheenTint_path << endl;

				sheenTint_texture = make_shared<ImageTexture>(sheenTint_path.c_str());
			}
		    
		    float int_ior = material.value("int_ior", 1.5f);
		    cout << "int_ior: " << int_ior << endl;
		    
		    float ext_ior = material.value("ext_ior", 1.0f);
		    cout << "ext_ior: " << ext_ior << endl;

			auto material1 = make_shared<DisneyDiffuse>(albedo_texture, roughness_texture, subsurface_texture);
			auto material2 = make_shared<DisneyMetal>(albedo_texture, roughness_texture,
				anisotropy_texture, metallic_texture, specular_texture, specularTint_texture);
			auto material3 = make_shared<DisneyClearcoat>(clearcoatGloss_texture);
			auto material4 = make_shared<DisneyGlass>(albedo_texture, roughness_texture,
				anisotropy_texture, int_ior, ext_ior);
			auto material5 = make_shared<DisneySheen>(albedo_texture, sheenTint_texture);
			mat = make_shared<DisneyPrinciple>(material1, material2, material3, material4, material5, metallic_texture,
				specularTransmission_texture, sheen_texture, clearcoat_texture);
		    
		    materials.push_back(pair<string, shared_ptr<Material>>(name, mat));
		}

		cout << endl;
		i++;
	}
}