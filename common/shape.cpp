#include <shape.h>
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
}

void TriangleMeshManager::EncodeTriangles(const std::vector<TriangleMesh>& meshes/*, const std::vector<Material>& materials*/) {
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

void TriangleMeshManager::CreateTBO() {
	// Create and initialize texture buffer object
	tbo_ = std::make_unique<TBO>(
		triangles_encoded_.data(),
		triangles_encoded_.size() * sizeof(TriangleEncoded),
		GL_RGB32F,
		GL_STATIC_DRAW
	);
}

const TBO& TriangleMeshManager::GetTBO() const noexcept {
	return *tbo_;
}

unsigned int TriangleMeshManager::GetNumTriangles() const noexcept {
	return static_cast<unsigned int>(triangles_encoded_.size());
}

std::vector<TriangleEncoded>& TriangleMeshManager::GetTriangleEncoded() {
	return triangles_encoded_;
}

std::unique_ptr<BVH> BVH::instance_ = nullptr;

BVH& BVH::Instance() {
	static std::once_flag init_flag;
	std::call_once(init_flag, []() {
		instance_ = std::unique_ptr<BVH>(new BVH());
		});

	return *instance_;
}

void BVH::Release() {
	if (instance_) {
		instance_->ReleaseResources();
		instance_.reset();
	}
}

void BVH::ReleaseResources() {
	nodes_.clear();
	nodes_encoded_.clear();
}

void BVH::Build(unsigned int n) {
	auto& mesh_manager = TriangleMeshManager::Instance();
	unsigned int num_triangles = mesh_manager.GetNumTriangles();

	BuildBVHWithSAH(0, num_triangles - 1, n);
	TransformBVHNode();
}

int BVH::BuildBVHWithSAH(int l, int r, int n) {
	if (l > r) {
		return -1;
	}

	auto& mesh_manager = TriangleMeshManager::Instance();
	auto& triangles = mesh_manager.GetTriangleEncoded();

	nodes_.push_back(BVHNode());
	unsigned int id = nodes_.size() - 1;
	BVHNode& node = nodes_[id];

	node.left = -1;
	node.right = -1;
	node.n = 0;
	node.index = 0;
	node.aa = Point3f(MaxFloat);
	node.bb = Point3f(MinFloat);

	for (unsigned int i = l; i <= r; i++) {
		const TriangleEncoded& tri = triangles[i];
		node.aa.x = std::min(node.aa.x, std::min(tri.p1.x, std::min(tri.p2.x, tri.p3.x)));
		node.aa.y = std::min(node.aa.y, std::min(tri.p1.y, std::min(tri.p2.y, tri.p3.y)));
		node.aa.z = std::min(node.aa.z, std::min(tri.p1.z, std::min(tri.p2.z, tri.p3.z)));

		node.bb.x = std::max(node.bb.x, std::max(tri.p1.x, std::max(tri.p2.x, tri.p3.x)));
		node.bb.y = std::max(node.bb.y, std::max(tri.p1.y, std::max(tri.p2.y, tri.p3.y)));
		node.bb.z = std::max(node.bb.z, std::max(tri.p1.z, std::max(tri.p2.z, tri.p3.z)));
	}

	unsigned int triangle_count = r - l + 1;
	if (triangle_count <= n) {
		node.n = triangle_count;
		node.index = l;

		return id;
	}

	float best_cost = InfFloat;
	int best_axis = -1;
	int best_split_index = l;

	auto CompareX = [](const TriangleEncoded& a, const TriangleEncoded& b) {
		Point3f centroid_a = (a.p1 + a.p2 + a.p3) / 3.0f;
		Point3f centroid_b = (b.p1 + b.p2 + b.p3) / 3.0f;

		return centroid_a.x < centroid_b.x;
	};
	auto CompareY = [](const TriangleEncoded& a, const TriangleEncoded& b) {
		Point3f centroid_a = (a.p1 + a.p2 + a.p3) / 3.0f;
		Point3f centroid_b = (b.p1 + b.p2 + b.p3) / 3.0f;

		return centroid_a.y < centroid_b.y;
	};
	auto CompareZ = [](const TriangleEncoded& a, const TriangleEncoded& b) {
		Point3f centroid_a = (a.p1 + a.p2 + a.p3) / 3.0f;
		Point3f centroid_b = (b.p1 + b.p2 + b.p3) / 3.0f;

		return centroid_a.z < centroid_b.z;
	};

	for (int axis = 0; axis < 3; axis++) {
		switch (axis) {
		case 0: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareX); break;
		case 1: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareY); break;
		case 2: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareZ); break;
		}

		std::vector<Point3f> left_min(r - l + 1), left_max(r - l + 1);
		std::vector<Point3f> right_min(r - l + 1), right_max(r - l + 1);

		for (unsigned int i = l; i <= r; i++) {
			unsigned int idx = i - l;
			const TriangleEncoded& tri = triangles[i];

			if (i == l) {
				left_min[idx] = Point3f(
					std::min(tri.p1.x, std::min(tri.p2.x, tri.p3.x)),
					std::min(tri.p1.y, std::min(tri.p2.y, tri.p3.y)),
					std::min(tri.p1.z, std::min(tri.p2.z, tri.p3.z))
				);
				left_max[idx] = Point3f(
					std::max(tri.p1.x, std::max(tri.p2.x, tri.p3.x)),
					std::max(tri.p1.y, std::max(tri.p2.y, tri.p3.y)),
					std::max(tri.p1.z, std::max(tri.p2.z, tri.p3.z))
				);
			}
			else {
				left_min[idx] = Point3f(
					std::min(left_min[idx - 1].x, std::min(tri.p1.x, std::min(tri.p2.x, tri.p3.x))),
					std::min(left_min[idx - 1].y, std::min(tri.p1.y, std::min(tri.p2.y, tri.p3.y))),
					std::min(left_min[idx - 1].z, std::min(tri.p1.z, std::min(tri.p2.z, tri.p3.z)))
				);
				left_max[idx] = Point3f(
					std::max(left_max[idx - 1].x, std::max(tri.p1.x, std::max(tri.p2.x, tri.p3.x))),
					std::max(left_max[idx - 1].y, std::max(tri.p1.y, std::max(tri.p2.y, tri.p3.y))),
					std::max(left_max[idx - 1].z, std::max(tri.p1.z, std::max(tri.p2.z, tri.p3.z)))
				);
			}
		}

		for (int i = r; i >= l; i--) {
			unsigned int idx = i - l;
			const TriangleEncoded& tri = triangles[i];

			if (i == r) {
				right_min[idx] = Point3f(
					std::min(tri.p1.x, std::min(tri.p2.x, tri.p3.x)),
					std::min(tri.p1.y, std::min(tri.p2.y, tri.p3.y)),
					std::min(tri.p1.z, std::min(tri.p2.z, tri.p3.z))
				);
				right_max[idx] = Point3f(
					std::max(tri.p1.x, std::max(tri.p2.x, tri.p3.x)),
					std::max(tri.p1.y, std::max(tri.p2.y, tri.p3.y)),
					std::max(tri.p1.z, std::max(tri.p2.z, tri.p3.z))
				);
			}
			else {
				right_min[idx] = Point3f(
					std::min(right_min[idx + 1].x, std::min(tri.p1.x, std::min(tri.p2.x, tri.p3.x))),
					std::min(right_min[idx + 1].y, std::min(tri.p1.y, std::min(tri.p2.y, tri.p3.y))),
					std::min(right_min[idx + 1].z, std::min(tri.p1.z, std::min(tri.p2.z, tri.p3.z)))
				);
				right_max[idx] = Point3f(
					std::max(right_max[idx + 1].x, std::max(tri.p1.x, std::max(tri.p2.x, tri.p3.x))),
					std::max(right_max[idx + 1].y, std::max(tri.p1.y, std::max(tri.p2.y, tri.p3.y))),
					std::max(right_max[idx + 1].z, std::max(tri.p1.z, std::max(tri.p2.z, tri.p3.z)))
				);
			}
		}

		for (unsigned int i = l; i < r; i++) {
			unsigned int left_count = i - l + 1;
			unsigned int right_count = r - i;

			Point3f left_aa = left_min[i - l];
			Point3f left_bb = left_max[i - l];
			Point3f right_aa = right_min[i - l + 1];
			Point3f right_bb = right_max[i - l + 1];

			float left_surface_area = 2.0f * (
				(left_bb.x - left_aa.x) * (left_bb.y - left_aa.y) +
				(left_bb.x - left_aa.x) * (left_bb.z - left_aa.z) +
				(left_bb.y - left_aa.y) * (left_bb.z - left_aa.z)
				);

			float right_surface_area = 2.0f * (
				(right_bb.x - right_aa.x) * (right_bb.y - right_aa.y) +
				(right_bb.x - right_aa.x) * (right_bb.z - right_aa.z) +
				(right_bb.y - right_aa.y) * (right_bb.z - right_aa.z)
				);


			Point3f node_aa = node.aa;
			Point3f node_bb = node.bb;
			float node_surface_area = 2.0f * (
				(node_bb.x - node_aa.x) * (node_bb.y - node_aa.y) +
				(node_bb.x - node_aa.x) * (node_bb.z - node_aa.z) +
				(node_bb.y - node_aa.y) * (node_bb.z - node_aa.z)
				);

			// cost = traversal_cost + left_surface_area/node_surface_area * left_count + right_surface_area/node_surface_area * right_count
			const float traversal_cost = 0.125f;
			float cost = traversal_cost +
				(left_surface_area / node_surface_area) * left_count +
				(right_surface_area / node_surface_area) * right_count;

			if (cost < best_cost) {
				best_cost = cost;
				best_axis = axis;
				best_split_index = i;
			}
		}
	}

	if (-1 == best_axis) {
		node.n = triangle_count;
		node.index = l;

		return id;
	}

	switch (best_axis) {
	case 0: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareX); break;
	case 1: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareY); break;
	case 2: std::sort(triangles.begin() + l, triangles.begin() + r + 1, CompareZ); break;
	}

	node.left = BuildBVHWithSAH(l, best_split_index, n);
	node.right = BuildBVHWithSAH(best_split_index + 1, r, n);

	return id;
}

void BVH::TransformBVHNode() {
	unsigned int num_nodes = nodes_.size();
	nodes_encoded_.resize(num_nodes);

	for (int i = 0; i < num_nodes; i++) {
		nodes_encoded_[i].children = Point3f(nodes_[i].left, nodes_[i].right, 0.0f);
		nodes_encoded_[i].leaf_info = Point3f(nodes_[i].n, nodes_[i].index, 0.0f);
		nodes_encoded_[i].aa = nodes_[i].aa;
		nodes_encoded_[i].bb = nodes_[i].bb;
		INFO("[info] Index : {}; aa {} {} {}; bb {} {} {}; leaf : {} {}; children {} {}", i, nodes_encoded_[i].aa.x, nodes_encoded_[i].aa.y, nodes_encoded_[i].aa.z,
			nodes_encoded_[i].bb.x, nodes_encoded_[i].bb.y, nodes_encoded_[i].bb.z, nodes_encoded_[i].leaf_info.x, nodes_encoded_[i].leaf_info.y,
			nodes_encoded_[i].children.x, nodes_encoded_[i].children.y);
	}
}

void BVH::CreateTBO() {
	tbo_ = std::make_unique<TBO>(
		nodes_encoded_.data(),
		nodes_encoded_.size() * sizeof(BVHNodeEncoded),
		GL_RGB32F,
		GL_STATIC_DRAW
		);
}

int BVH::GetNumNodes() const noexcept {
	return nodes_.size();
}

const TBO& BVH::GetTBO() const noexcept {
	return *tbo_;
}

NAMESPACE_END(dream)