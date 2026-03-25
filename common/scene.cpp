#include <scene.h>
#include <tiny_bvh.h>
#include <stb_image.h>
#include <utils.h>

NAMESPACE_BEGIN(dream)

std::unique_ptr<SceneManager> SceneManager::instance_ = nullptr;

SceneManager& SceneManager::Instance() {
	static std::once_flag init_flag;
	std::call_once(init_flag, []() {
		instance_ = std::unique_ptr<SceneManager>(new SceneManager());
		});
	return *instance_;
}

void SceneManager::Release() {
	if (instance_) {
		instance_->ReleaseInstance();
		instance_.reset();
	}
}

void SceneManager::ReleaseInstance() {
	// Clear regular geometry data
	triangles_encoded_.clear();
	bvh_nodes_encoded_.clear();
	triangle_tbo_.reset();
	bvh_node_tbo_.reset();

	// Clear light geometry data
	triangles_light_encoded_.clear();
	bvh_nodes_light_encoded_.clear();
	triangle_light_tbo_.reset();
	bvh_node_light_tbo_.reset();

	// Clear alias table data
	light_triangle_weights_.clear();
	mesh_light_alias_table_tbo_.reset();

	// Clear texture data
	textures_.clear();
	texture_name_to_id_.clear();
	if (texture_array_ != 0) {
		glDeleteTextures(1, &texture_array_);
		texture_array_ = 0;
	}
}

int SceneManager::LoadTexture(const std::string& file_path, TextureType type) {
	INFO("[info] Loading texture: {}", file_path);

	// Check if texture already loaded
	auto it = texture_name_to_id_.find(file_path);
	if (it != texture_name_to_id_.end()) {
		INFO("[info] Texture already loaded: {} (ID: {})", file_path, it->second);

		return it->second;
	}

	stbi_set_flip_vertically_on_load(true);
	int width, height, channels;

	// Load texture as float data
	float* data = stbi_loadf(file_path.c_str(), &width, &height, &channels, 4);
	if (!data) {
		ERROR("[error] Failed to load texture: {}", file_path);

		return -1;
	}

	INFO("[info] Original texture: {}x{}, {} channels", width, height, channels);

	const int TARGET_SIZE = 2048;
	const int TARGET_CHANNELS = 4;  // RGBA

	// Resize texture to 2048x2048
	std::vector<float> resized_data(TARGET_SIZE * TARGET_SIZE * TARGET_CHANNELS, 1.0f);

	// Use bilinear interpolation for resizing
	for (int y = 0; y < TARGET_SIZE; ++y) {
		for (int x = 0; x < TARGET_SIZE; ++x) {
			float u = static_cast<float>(x) / (TARGET_SIZE - 1);
			float v = static_cast<float>(y) / (TARGET_SIZE - 1);

			Vector4f color = BilinearSample(data, width, height, channels, u, v);

			int idx = (y * TARGET_SIZE + x) * TARGET_CHANNELS;
			resized_data[idx] = color.r;
			resized_data[idx + 1] = color.g;
			resized_data[idx + 2] = color.b;
			resized_data[idx + 3] = color.a;
		}
	}

	stbi_image_free(data);

	// Create texture object
	Texture texture;
	texture.path = file_path;
	texture.width = TARGET_SIZE;
	texture.height = TARGET_SIZE;
	texture.channels = TARGET_CHANNELS;
	texture.data = std::move(resized_data);
	texture.texture_array_layer = -1;  // Will be set when creating texture array

	int texture_id = textures_.size();
	textures_.push_back(std::move(texture));
	texture_name_to_id_[file_path] = texture_id;

	INFO("[info] Texture loaded: {} ({}x{}, ID: {})", file_path, TARGET_SIZE, TARGET_SIZE, texture_id);

	return texture_id;
}

glm::vec4 SceneManager::BilinearSample(const float* data, int width, int height, int channels, float u, float v) const {
	float x = u * (width - 1);
	float y = v * (height - 1);

	int x0 = static_cast<int>(std::floor(x));
	int y0 = static_cast<int>(std::floor(y));
	int x1 = std::min(x0 + 1, width - 1);
	int y1 = std::min(y0 + 1, height - 1);

	float wx = x - x0;
	float wy = y - y0;

	glm::vec4 p00(0.0f), p10(0.0f), p01(0.0f), p11(0.0f);

	for (int c = 0; c < std::min(channels, 4); ++c) {
		p00[c] = data[(y0 * width + x0) * channels + c];
		p10[c] = data[(y0 * width + x1) * channels + c];
		p01[c] = data[(y1 * width + x0) * channels + c];
		p11[c] = data[(y1 * width + x1) * channels + c];
	}

	glm::vec4 top = p00 * (1.0f - wx) + p10 * wx;
	glm::vec4 bottom = p01 * (1.0f - wx) + p11 * wx;

	return top * (1.0f - wy) + bottom * wy;
}

void SceneManager::CreateTextureArray() {
	if (textures_.empty()) {
		INFO("[info] No textures to create texture array.");

		return;
	}

	INFO("[info] Creating texture array with {} textures (2048x2048 each).", textures_.size());

	// Create 2D texture array
	glGenTextures(1, &texture_array_);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array_);

	// Calculate mipmap levels
	int mip_levels = 1;
	int size = 2048;  // Target size
	while (size > 1) {
		size >>= 1;
		mip_levels++;
	}

	// Allocate storage for texture array
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mip_levels, GL_RGBA32F, 2048, 2048, static_cast<GLsizei>(textures_.size()));

	// Upload each texture
	for (unsigned int i = 0; i < textures_.size(); ++i) {
		textures_[i].texture_array_layer = static_cast<int>(i);

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
			0, 0, static_cast<GLint>(i),  // layer
			2048, 2048, 1,
			GL_RGBA, GL_FLOAT,
			textures_[i].data.data());
	}

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Generate mipmaps
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	INFO("[info] Texture array created: ID={}, {} textures", texture_array_, textures_.size());
}

void SceneManager::EncodeTriangles(const std::vector<TriangleMesh>& meshes, bool is_light) {
	unsigned int total_count = 0;

	// Single pass: count total triangles for the current batch
	for (const auto& mesh : meshes) {
		total_count += mesh.GetNumTriangles();
	}

	// Reset weight data for light triangles
	if (is_light) {
		light_triangle_weights_.clear();
		light_triangle_weights_.reserve(total_count);
	}

	// Resize appropriate container based on the is_light parameter
	if (is_light) {
		triangles_light_encoded_.resize(total_count);
	}
	else {
		triangles_encoded_.resize(total_count);
	}

	unsigned int index = 0;

	// Encode triangles into the appropriate container
	for (unsigned int mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) {
		const auto& mesh = meshes[mesh_idx];
		auto material = mesh.GetMaterial();

		const unsigned int mesh_triangle_count = mesh.GetNumTriangles();
		const auto& vertices = mesh.GetVertices();
		const auto& normals = mesh.GetNormals();
		const auto& texcoords = mesh.GetTexCoords();
		const auto& indices = mesh.GetIndices();

		int diffuse_tex_id = material.GetTextureID(TextureType::DIFFUSE);
		int roughness_tex_id = material.GetTextureID(TextureType::ROUGHNESS);

		for (unsigned int i = 0; i < mesh_triangle_count; ++i) {
			const unsigned int idx0 = indices[i * 3];
			const unsigned int idx1 = indices[i * 3 + 1];
			const unsigned int idx2 = indices[i * 3 + 2];

			TriangleEncoded encoded_tri;

			// Extract vertex positions using indices and pack uv.x in w component
			encoded_tri.p1 = Point4f(
				vertices[idx0 * 3],
				vertices[idx0 * 3 + 1],
				vertices[idx0 * 3 + 2],
				texcoords[idx0 * 2]);

			encoded_tri.p2 = Point4f(
				vertices[idx1 * 3],
				vertices[idx1 * 3 + 1],
				vertices[idx1 * 3 + 2],
				texcoords[idx1 * 2]);

			encoded_tri.p3 = Point4f(
				vertices[idx2 * 3],
				vertices[idx2 * 3 + 1],
				vertices[idx2 * 3 + 2],
				texcoords[idx2 * 2]);

			// Extract normals using indices and pack uv.y in w component
			encoded_tri.n1 = Vector4f(
				normals[idx0 * 3],
				normals[idx0 * 3 + 1],
				normals[idx0 * 3 + 2],
				texcoords[idx0 * 2 + 1]);

			encoded_tri.n2 = Vector4f(
				normals[idx1 * 3],
				normals[idx1 * 3 + 1],
				normals[idx1 * 3 + 2],
				texcoords[idx1 * 2 + 1]);

			encoded_tri.n3 = Vector4f(
				normals[idx2 * 3],
				normals[idx2 * 3 + 1],
				normals[idx2 * 3 + 2],
				texcoords[idx2 * 2 + 1]);

			// Set material parameters
			encoded_tri.material_type.x = static_cast<float>(material.type);

			encoded_tri.emission = Vector4f(
				material.emission.x,
				material.emission.y,
				material.emission.z,
				1.0f
			);

			encoded_tri.diffuse = Vector4f(
				material.diffuse.r,          // x: diffuse color R
				material.diffuse.g,          // y: diffuse color G
				material.diffuse.b,          // z: diffuse color B
				static_cast<float>(diffuse_tex_id)  // w: diffuse texture ID
			);

			encoded_tri.roughness = Vector4f(
				material.roughness,
				0.0f,
				0.0f,
				static_cast<float>(roughness_tex_id)  // w: roughness texture ID
			);

			// Store in appropriate container based on the is_light parameter
			if (is_light) {
				// Calculate triangle area for importance sampling
				Vector3f e1 = Point3f(encoded_tri.p2) - Point3f(encoded_tri.p1);
				Vector3f e2 = Point3f(encoded_tri.p3) - Point3f(encoded_tri.p1);

				float area = 0.5f * glm::length(glm::cross(e1, e2));
				const float PI = 3.1415926535897932385f;
				auto Luminance = [](const Vector3f& rgb) -> float {
					return 0.2126f * rgb.r + 0.7152f * rgb.g + 0.0722f * rgb.b;
				};

				// Calculate weight: power = area * luminance * pi for importance sampling
				float weight = area * Luminance(material.emission) * PI;

				light_triangle_weights_.push_back(weight);
				triangles_light_encoded_[index++] = encoded_tri;
			}
			else {
				triangles_encoded_[index++] = encoded_tri;
			}
		}
	}
}

void SceneManager::BuildLightAliasTable() {
	// Construct alias table using precomputed triangle weights
	// This enables O(1) time complexity for light triangle sampling on GPU
	mesh_light_alias_table_ = AliasTable1D(light_triangle_weights_);
}

std::vector<TriangleEncoded> SceneManager::BuildBVHForTriangles(const std::vector<TriangleEncoded>& triangles,
	std::vector<BVHNodeEncoded>& bvh_nodes, bool is_light) {

	if (triangles.empty()) {
		return std::vector<TriangleEncoded>();
	}

	unsigned int triangles_size = static_cast<unsigned int>(triangles.size());
	std::vector<tinybvh::bvhvec4> vertices;
	vertices.reserve(triangles_size * 3);

	// Prepare vertex data for BVH construction
	for (const auto& tri : triangles) {
		vertices.push_back(tinybvh::bvhvec4{ tri.p1.x, tri.p1.y, tri.p1.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p2.x, tri.p2.y, tri.p2.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p3.x, tri.p3.y, tri.p3.z, 0.0f });
	}

	// Build BVH using external library
	tinybvh::BVH_GPU bvh;
	bvh.Build(vertices.data(), triangles_size);

	// Encode BVH nodes
	unsigned int used_nodes = bvh.usedNodes;
	bvh_nodes.resize(used_nodes);

	const tinybvh::BVH_GPU::BVHNode* src_nodes = bvh.bvhNode;
	for (unsigned int i = 0; i < used_nodes; ++i) {
		const auto& src_node = src_nodes[i];
		auto& dst_node = bvh_nodes[i];

		dst_node.lmin = Point4f(src_node.lmin.x, src_node.lmin.y, src_node.lmin.z,
			static_cast<float>(src_node.left));
		dst_node.lmax = Point4f(src_node.lmax.x, src_node.lmax.y, src_node.lmax.z,
			static_cast<float>(src_node.right));
		dst_node.rmin = Point4f(src_node.rmin.x, src_node.rmin.y, src_node.rmin.z,
			static_cast<float>(src_node.triCount));
		dst_node.rmax = Point4f(src_node.rmax.x, src_node.rmax.y, src_node.rmax.z,
			static_cast<float>(src_node.firstTri));
	}

	// Sort triangles based on BVH primitive indices
	const unsigned int* indices = bvh.bvh.primIdx;
	std::vector<float> sorted_light_triangle_weights(light_triangle_weights_.size());
	std::vector<TriangleEncoded> sorted_triangles(triangles_size);
	for (unsigned int i = 0; i < triangles_size; ++i) {
		sorted_triangles[i] = triangles[indices[i]];
		if (is_light) {
			sorted_light_triangle_weights[i] = light_triangle_weights_[indices[i]];
		}
	}
	if (is_light) {
		light_triangle_weights_ = sorted_light_triangle_weights;
	}

	return sorted_triangles;
}

void SceneManager::BuildBVH() {
	// Build BVH for regular geometry
	if (!triangles_encoded_.empty()) {
		triangles_encoded_ = BuildBVHForTriangles(triangles_encoded_, bvh_nodes_encoded_, false);
	}

	// Build BVH for light geometry
	if (!triangles_light_encoded_.empty()) {
		triangles_light_encoded_ = BuildBVHForTriangles(triangles_light_encoded_, bvh_nodes_light_encoded_, true);
		if (!light_triangle_weights_.empty()) {
			BuildLightAliasTable();
		}
	}

	// Log build information
	if (triangles_encoded_.empty() && triangles_light_encoded_.empty()) {
		ERROR("[error] No triangles to build BVH!");
	}
}

void SceneManager::CreateGPUBuffers() {
	CreateTextureArray();

	// Create GPU buffers for regular geometry
	if (!triangles_encoded_.empty()) {
		triangle_tbo_ = std::make_unique<TBO>(
			triangles_encoded_.data(),
			triangles_encoded_.size() * sizeof(TriangleEncoded),
			GL_RGBA32F,
			GL_STATIC_DRAW
			);
	}

	if (!bvh_nodes_encoded_.empty()) {
		bvh_node_tbo_ = std::make_unique<TBO>(
			bvh_nodes_encoded_.data(),
			bvh_nodes_encoded_.size() * sizeof(BVHNodeEncoded),
			GL_RGBA32F,
			GL_STATIC_DRAW
			);
	}

	// Create GPU buffers for light geometry
	if (!triangles_light_encoded_.empty()) {
		triangle_light_tbo_ = std::make_unique<TBO>(
			triangles_light_encoded_.data(),
			triangles_light_encoded_.size() * sizeof(TriangleEncoded),
			GL_RGBA32F,
			GL_STATIC_DRAW
			);
	}

	if (!bvh_nodes_light_encoded_.empty()) {
		bvh_node_light_tbo_ = std::make_unique<TBO>(
			bvh_nodes_light_encoded_.data(),
			bvh_nodes_light_encoded_.size() * sizeof(BVHNodeEncoded),
			GL_RGBA32F,
			GL_STATIC_DRAW
			);
	}

	// Create GPU buffer for alias table data
	if (!mesh_light_alias_table_.GetGPUData().empty()) {
		mesh_light_alias_table_tbo_ = std::make_unique<TBO>(
			mesh_light_alias_table_.GetGPUData().data(),
			mesh_light_alias_table_.GetGPUData().size() * sizeof(AliasTableData),
			GL_RG32F,  // Each element contains alias index and probability
			GL_STATIC_DRAW
			);
	}
}

const TBO& SceneManager::GetTriangleTBO() const noexcept {
	return *triangle_tbo_;
}

const TBO& SceneManager::GetBVHNodeTBO() const noexcept {
	return *bvh_node_tbo_;
}

const TBO& SceneManager::GetTriangleLightTBO() const noexcept {
	return *triangle_light_tbo_;
}

const TBO& SceneManager::GetMeshLightAliasTableTBO() const noexcept {
	return *mesh_light_alias_table_tbo_;
}

const TBO& SceneManager::GetBVHNodeLightTBO() const noexcept {
	return *bvh_node_light_tbo_;
}

float SceneManager::GetMeshLightTableSum() const noexcept {
	return mesh_light_alias_table_.Sum();
}

float SceneManager::GetMeshLightTableMax() const noexcept {
	return mesh_light_alias_table_.Max();
}

unsigned int SceneManager::GetMeshLightTableSize() const noexcept {
	return static_cast<unsigned int>(light_triangle_weights_.size());
}

GLuint SceneManager::GetTextureArray() const noexcept {
	return texture_array_;
}

int SceneManager::GetTextureCount() const noexcept {
	return static_cast<int>(textures_.size());
}

NAMESPACE_END(dream)