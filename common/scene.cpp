#include <scene.h>
#include <tiny_bvh.h>

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

		const unsigned int mesh_triangle_count = mesh.GetNumTriangles();
		const auto& vertices = mesh.GetVertices();
		const auto& normals = mesh.GetNormals();
		const auto& texcoords = mesh.GetTexCoords();
		const auto& indices = mesh.GetIndices();

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

			encoded_tri.material_type = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
			encoded_tri.diffuse = Vector4f(0.8f, 0.8f, 0.8f, 0.0f);

			// Store in appropriate container based on the is_light parameter
			if (is_light) {
				// Calculate triangle area for importance sampling
				Vector3f e1 = Point3f(encoded_tri.p2) - Point3f(encoded_tri.p1);
				Vector3f e2 = Point3f(encoded_tri.p3) - Point3f(encoded_tri.p1);
				float area = 0.5f * glm::length(glm::cross(e1, e2));

				// Calculate weight: area + luminance for importance sampling
				float weight = area;

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

NAMESPACE_END(dream)