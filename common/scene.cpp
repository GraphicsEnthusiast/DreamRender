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
	triangles_encoded_.clear();
	bvh_nodes_encoded_.clear();
	triangle_tbo_.reset();
	bvh_node_tbo_.reset();
}

void SceneManager::EncodeTriangles(const std::vector<TriangleMesh>& meshes/*, const std::vector<Material>& materials*/) {
	unsigned int total_triangle_count = 0;

	for (const auto& mesh : meshes) {
		total_triangle_count += mesh.GetNumTriangles();
	}

	triangles_encoded_.resize(total_triangle_count);

	unsigned int triangle_index = 0;

	for (unsigned int mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) {
		const auto& mesh = meshes[mesh_idx];
		//const auto& material = materials[meshIdx];

		const unsigned int mesh_triangle_count = mesh.GetNumTriangles();
		const auto& vertices = mesh.GetVertices();
		const auto& normals = mesh.GetNormals();
		const auto& texcoords = mesh.GetTexCoords();
		const auto& indices = mesh.GetIndices();

		for (unsigned int i = 0; i < mesh_triangle_count; ++i) {
			const unsigned int idx0 = indices[i * 3];
			const unsigned int idx1 = indices[i * 3 + 1];
			const unsigned int idx2 = indices[i * 3 + 2];

			// Extract vertex positions using indices and pack uv.x in w component
			triangles_encoded_[triangle_index].p1 = Point4f(
				vertices[idx0 * 3],
				vertices[idx0 * 3 + 1],
				vertices[idx0 * 3 + 2],
				texcoords[idx0 * 2]);  // uv1.x in w

			triangles_encoded_[triangle_index].p2 = Point4f(
				vertices[idx1 * 3],
				vertices[idx1 * 3 + 1],
				vertices[idx1 * 3 + 2],
				texcoords[idx1 * 2]);  // uv2.x in w

			triangles_encoded_[triangle_index].p3 = Point4f(
				vertices[idx2 * 3],
				vertices[idx2 * 3 + 1],
				vertices[idx2 * 3 + 2],
				texcoords[idx2 * 2]);  // uv3.x in w

			// Extract normals using indices and pack uv.y in w component
			triangles_encoded_[triangle_index].n1 = Vector4f(
				normals[idx0 * 3],
				normals[idx0 * 3 + 1],
				normals[idx0 * 3 + 2],
				texcoords[idx0 * 2 + 1]);  // uv1.y in w

			triangles_encoded_[triangle_index].n2 = Vector4f(
				normals[idx1 * 3],
				normals[idx1 * 3 + 1],
				normals[idx1 * 3 + 2],
				texcoords[idx1 * 2 + 1]);  // uv2.y in w

			triangles_encoded_[triangle_index].n3 = Vector4f(
				normals[idx2 * 3],
				normals[idx2 * 3 + 1],
				normals[idx2 * 3 + 2],
				texcoords[idx2 * 2 + 1]);  // uv3.y in w

			triangle_index++;
		}
	}
}

void SceneManager::BuildBVH() {
	if (triangles_encoded_.empty()) {
		ERROR("[error] No triangles to build BVH!");

		return;
	}

	unsigned int trangles_size = triangles_encoded_.size();
	std::vector<tinybvh::bvhvec4> vertices;
	vertices.reserve(trangles_size * 3);

	for (const auto& tri : triangles_encoded_) {
		vertices.push_back(tinybvh::bvhvec4{ tri.p1.x, tri.p1.y, tri.p1.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p2.x, tri.p2.y, tri.p2.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p3.x, tri.p3.y, tri.p3.z, 0.0f });
	}

	tinybvh::BVH_GPU bvh;
	bvh.Build(vertices.data(), trangles_size);

	unsigned int used_nodes = bvh.usedNodes;
	bvh_nodes_encoded_.resize(used_nodes);

	const tinybvh::BVH_GPU::BVHNode* src_nodes = bvh.bvhNode;

	for (unsigned int i = 0; i < used_nodes; ++i) {
		const auto& src_node = src_nodes[i];
		auto& dst_node = bvh_nodes_encoded_[i];

		dst_node.lmin = Point4f(src_node.lmin.x, src_node.lmin.y, src_node.lmin.z, static_cast<float>(src_node.left));
		dst_node.lmax = Point4f(src_node.lmax.x, src_node.lmax.y, src_node.lmax.z, static_cast<float>(src_node.right));
		dst_node.rmin = Point4f(src_node.rmin.x, src_node.rmin.y, src_node.rmin.z, static_cast<float>(src_node.triCount));
		dst_node.rmax = Point4f(src_node.rmax.x, src_node.rmax.y, src_node.rmax.z, static_cast<float>(src_node.firstTri));
	}

	const unsigned int* indices = bvh.bvh.primIdx;
	std::vector<TriangleEncoded> sorted_triangles(trangles_size);
	for (unsigned int i = 0; i < trangles_size; ++i) {
		sorted_triangles[i] = triangles_encoded_[indices[i]];
	}
	triangles_encoded_ = std::move(sorted_triangles);
}

void SceneManager::CreateGPUBuffers() {
	triangle_tbo_ = std::make_unique<TBO>(
		triangles_encoded_.data(),
		triangles_encoded_.size() * sizeof(TriangleEncoded),
		GL_RGBA32F,
		GL_STATIC_DRAW
	);

	bvh_node_tbo_ = std::make_unique<TBO>(
		bvh_nodes_encoded_.data(),
		bvh_nodes_encoded_.size() * sizeof(BVHNodeEncoded),
		GL_RGBA32F,
		GL_STATIC_DRAW
	);
}

const TBO& SceneManager::GetTriangleTBO() const noexcept {
	return *triangle_tbo_;
}

const TBO& SceneManager::GetBVHNodeTBO() const noexcept {
	return *bvh_node_tbo_;
}

NAMESPACE_END(dream)