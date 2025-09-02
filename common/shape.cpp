#include <shape.h>
#include <tiny_bvh.h>
#include <tiny_obj_loader.h>

NAMESPACE_BEGIN(dream)

TriangleMesh::TriangleMesh(const std::string& file, const Transform& trans) : transform_(trans) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str()) || 0 == shapes.size()) {
		ERROR("[error] Load from obj {} failed!", file.c_str());
	}

	// loop over shapes
	for (unsigned int s = 0; s < shapes.size(); ++s) {
		unsigned int index_offset = 0;
		// loop over faces
		for (unsigned int f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			const unsigned int fv = static_cast<unsigned int>(shapes[s].mesh.num_face_vertices[f]);

			std::vector<Point3f> vertices;
			std::vector<Vector3f> normals;
			std::vector<Point2f> texcoords;

			// loop over vertices
			// get vertices, normals, texcoords of a triangle
			for (unsigned int v = 0; v < fv; ++v) {
				const tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				const tinyobj::real_t vx =
					attrib.vertices[3 * static_cast<unsigned int>(idx.vertex_index) + 0];
				const tinyobj::real_t vy =
					attrib.vertices[3 * static_cast<unsigned int>(idx.vertex_index) + 1];
				const tinyobj::real_t vz =
					attrib.vertices[3 * static_cast<unsigned int>(idx.vertex_index) + 2];

				Point3f v_world = transform_.TransformPoint(Point3f(vx, vy, vz));
				vertices.push_back(Point3f(v_world.x, v_world.y, v_world.z));

				if (idx.normal_index >= 0) {
					const tinyobj::real_t nx =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 0];
					const tinyobj::real_t ny =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 1];
					const tinyobj::real_t nz =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 2];

					Vector3f n_world = transform_.TransformVector(Vector3f(nx, ny, nz));
					normals.push_back(glm::normalize(Vector3f(n_world.x, n_world.y, n_world.z)));
				}

				if (idx.texcoord_index >= 0) {
					const tinyobj::real_t tx =
						attrib
						.texcoords[2 * static_cast<unsigned int>(idx.texcoord_index) + 0];
					const tinyobj::real_t ty =
						attrib
						.texcoords[2 * static_cast<unsigned int>(idx.texcoord_index) + 1];
					texcoords.push_back(Point2f(tx, ty));
				}
			}

			// if normals is empty, add geometric normal
			if (0 == normals.size()) {
				const Point3f v1 = glm::normalize(vertices[1] - vertices[0]);
				const Point3f v2 = glm::normalize(vertices[2] - vertices[0]);
				const Vector3f n = glm::normalize(glm::cross(v1, v2));
				normals.push_back(n);
				normals.push_back(n);
				normals.push_back(n);
			}

			// if texcoords is empty, add barycentric coords
			if (0 == texcoords.size()) {
				texcoords.push_back(Point2f(0.0f));
				texcoords.push_back(Point2f(1.0f, 0.0f));
				texcoords.push_back(Point2f(0.0f, 1.0f));
			}

			for (int i = 0; i < 3; ++i) {
				vertices_.push_back(vertices[i][0]);
				vertices_.push_back(vertices[i][1]);
				vertices_.push_back(vertices[i][2]);

				normals_.push_back(normals[i][0]);
				normals_.push_back(normals[i][1]);
				normals_.push_back(normals[i][2]);

				texcoords_.push_back(texcoords[i][0]);
				texcoords_.push_back(texcoords[i][1]);

				indices_.push_back(indices_.size());
			}

			index_offset += fv;
		}
	}
}

unsigned int TriangleMesh::GetNumTriangles() const noexcept {
	return static_cast<unsigned int>(indices_.size() / 3);
}

unsigned int TriangleMesh::GetNumVertices() const noexcept {
	return static_cast<unsigned int>(vertices_.size() / 3);
}

const std::vector<float>& TriangleMesh::GetVertices() const noexcept {
	return vertices_;
}

const std::vector<unsigned int>& TriangleMesh::GetIndices() const noexcept {
	return indices_;
}

const std::vector<float>& TriangleMesh::GetNormals() const noexcept {
	return normals_;
}

const std::vector<float>& TriangleMesh::GetTexCoords() const noexcept {
	return texcoords_;
}

std::unique_ptr<TriangleMeshManager> TriangleMeshManager::instance_ = nullptr;

TriangleMeshManager& TriangleMeshManager::Instance() {
	static std::once_flag init_flag;
	std::call_once(init_flag, []() {
		instance_ = std::unique_ptr<TriangleMeshManager>(new TriangleMeshManager());
		});

	return *instance_;
}

void TriangleMeshManager::Release() {
	if (instance_) {
		instance_->ReleaseInstance();
		instance_.reset();
	}
}

void TriangleMeshManager::ReleaseInstance() {
	triangles_encoded_.clear();
	bvh_nodes_.clear();
	triangle_tbo_.reset();
	bvh_tbo_.reset();
}

void TriangleMeshManager::BuildTriangles(const std::vector<TriangleMesh>& meshes/*, const std::vector<Material>& materials*/) {
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

			// Extract vertex positions using indices
			triangles_encoded_[triangle_index].p1 = glm::vec3(
				vertices[idx0 * 3],
				vertices[idx0 * 3 + 1],
				vertices[idx0 * 3 + 2]);

			triangles_encoded_[triangle_index].p2 = glm::vec3(
				vertices[idx1 * 3],
				vertices[idx1 * 3 + 1],
				vertices[idx1 * 3 + 2]);

			triangles_encoded_[triangle_index].p3 = glm::vec3(
				vertices[idx2 * 3],
				vertices[idx2 * 3 + 1],
				vertices[idx2 * 3 + 2]);

			// Extract normals using indices
			triangles_encoded_[triangle_index].n1 = glm::vec3(
				normals[idx0 * 3],
				normals[idx0 * 3 + 1],
				normals[idx0 * 3 + 2]);

			triangles_encoded_[triangle_index].n2 = glm::vec3(
				normals[idx1 * 3],
				normals[idx1 * 3 + 1],
				normals[idx1 * 3 + 2]);

			triangles_encoded_[triangle_index].n3 = glm::vec3(
				normals[idx2 * 3],
				normals[idx2 * 3 + 1],
				normals[idx2 * 3 + 2]);

			// Extract UV coordinates using indices (z-component set to 0)
			triangles_encoded_[triangle_index].uv1 = glm::vec3(
				texcoords[idx0 * 2],
				texcoords[idx0 * 2 + 1],
				0.0f);

			triangles_encoded_[triangle_index].uv2 = glm::vec3(
				texcoords[idx1 * 2],
				texcoords[idx1 * 2 + 1],
				0.0f);

			triangles_encoded_[triangle_index].uv3 = glm::vec3(
				texcoords[idx2 * 2],
				texcoords[idx2 * 2 + 1],
				0.0f);

			// Copy material properties
			//triangles_encoded_[triangleIndex].emissive = material.emissive;
			//triangles_encoded_[triangleIndex].baseColor = material.baseColor;
			//triangles_encoded_[triangleIndex].param1 = glm::vec3(
			//	material.subsurface, material.metallic, material.specular);
			//triangles_encoded_[triangleIndex].param2 = glm::vec3(
			//	material.specularTint, material.roughness, material.anisotropic);
			//triangles_encoded_[triangleIndex].param3 = glm::vec3(
			//	material.sheen, material.sheenTint, material.clearcoat);
			//triangles_encoded_[triangleIndex].param4 = glm::vec3(
			//	material.clearcoatGloss, material.IOR, material.transmission);
			//triangles_encoded_[triangleIndex].tex = glm::vec3(
			//	material.isTex, material.texID, material.lightID);

			triangle_index++;
		}
	}
}

void TriangleMeshManager::BuildBVH() {
	if (triangles_encoded_.empty()) {
		ERROR("[error] No triangles to build BVH!");

		return;
	}

	std::vector<tinybvh::bvhvec4> vertices;
	vertices.reserve(triangles_encoded_.size() * 3);

	for (const auto& tri : triangles_encoded_) {
		vertices.push_back(tinybvh::bvhvec4{ tri.p1.x, tri.p1.y, tri.p1.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p2.x, tri.p2.y, tri.p2.z, 0.0f });
		vertices.push_back(tinybvh::bvhvec4{ tri.p3.x, tri.p3.y, tri.p3.z, 0.0f });
	}

	tinybvh::BVH_GPU bvh;
	bvh.Build(vertices.data(), triangles_encoded_.size());

	unsigned int usedNodes = bvh.usedNodes;
	bvh_nodes_.resize(usedNodes);

	const tinybvh::BVH_GPU::BVHNode* src_nodes = bvh.bvhNode;

	for (unsigned int i = 0; i < usedNodes; ++i) {
		const auto& src_node = src_nodes[i];
		auto& dst_node = bvh_nodes_[i];

		dst_node.lmin = Point4f(src_node.lmin.x, src_node.lmin.y, src_node.lmin.z, 0.0f);
		dst_node.lmax = Point4f(src_node.lmax.x, src_node.lmax.y, src_node.lmax.z, 0.0f);
		dst_node.rmin = Point4f(src_node.rmin.x, src_node.rmin.y, src_node.rmin.z, 0.0f);
		dst_node.rmax = Point4f(src_node.rmax.x, src_node.rmax.y, src_node.rmax.z, 0.0f);

		dst_node.lmin.w = *reinterpret_cast<const float*>(&src_node.left);
		dst_node.lmax.w = *reinterpret_cast<const float*>(&src_node.right);
		dst_node.rmin.w = *reinterpret_cast<const float*>(&src_node.triCount);
		dst_node.rmax.w = *reinterpret_cast<const float*>(&src_node.firstTri);
	}
}

void TriangleMeshManager::CreateGPUBuffers() {
	triangle_tbo_ = std::make_unique<TBO>(
		triangles_encoded_.data(),
		triangles_encoded_.size() * sizeof(TriangleEncoded),
		GL_RGB32F,
		GL_STATIC_DRAW
		);

	bvh_tbo_ = std::make_unique<TBO>(
		bvh_nodes_.data(),
		bvh_nodes_.size() * sizeof(BVHNodeEncoded),
		GL_RGBA32F,
		GL_STATIC_DRAW
		);
}

const TBO& TriangleMeshManager::GetTriangleTBO() const noexcept {
	return *triangle_tbo_;
}

const TBO& TriangleMeshManager::GetBVHTBO() const noexcept {
	return *bvh_tbo_;
}

unsigned int TriangleMeshManager::GetNumTriangles() const noexcept {
	return static_cast<unsigned int>(triangles_encoded_.size());
}

unsigned int TriangleMeshManager::GetNumBVHNodes() const noexcept {
	return static_cast<unsigned int>(bvh_nodes_.size());
}

NAMESPACE_END(dream)