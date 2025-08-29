#include <shape.h>
#include <tiny_obj_loader.h>

NAMESPACE_BEGIN(dream)

TriangleMesh::TriangleMesh(const std::string& file, const Transform& trans) : transform(trans) {
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

				Point3f v_world = transform.TransformPoint(Point3f(vx, vy, vz));
				vertices.push_back(Point3f(v_world.x, v_world.y, v_world.z));

				if (idx.normal_index >= 0) {
					const tinyobj::real_t nx =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 0];
					const tinyobj::real_t ny =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 1];
					const tinyobj::real_t nz =
						attrib.normals[3 * static_cast<unsigned int>(idx.normal_index) + 2];

					Vector3f n_world = transform.TransformVector(Vector3f(nx, ny, nz));
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
				this->vertices.push_back(vertices[i][0]);
				this->vertices.push_back(vertices[i][1]);
				this->vertices.push_back(vertices[i][2]);

				this->normals.push_back(normals[i][0]);
				this->normals.push_back(normals[i][1]);
				this->normals.push_back(normals[i][2]);

				this->texcoords.push_back(texcoords[i][0]);
				this->texcoords.push_back(texcoords[i][1]);

				this->indices.push_back(this->indices.size());
			}

			index_offset += fv;
		}
	}

	// Initialize TBOs after loading all data
	InitializeTBOs();
}

const TBO& TriangleMesh::GetVertexTBO() const noexcept {
	return *vertex_tbo_;
}

const TBO& TriangleMesh::GetNormalTBO() const noexcept {
	return *normal_tbo_;
}

const TBO& TriangleMesh::GetTexCoordTBO() const noexcept {
	return *texcoord_tbo_;
}

const TBO& TriangleMesh::GetIndexTBO() const noexcept {
	return *index_tbo_;
}

unsigned int TriangleMesh::GetNumTriangles() const noexcept {
	return indices.size() / 3;
}

unsigned int TriangleMesh::GetNumVertices() const noexcept {
	return vertices.size() / 3;
}

void TriangleMesh::InitializeTBOs() {
	// Create TBO for vertices (vec3 per vertex)
	vertex_tbo_ = std::make_unique<TBO>(
		vertices.data(),
		vertices.size() * sizeof(float),
		GL_RGB32F
		);

	// Create TBO for normals (vec3 per vertex)
	normal_tbo_ = std::make_unique<TBO>(
		normals.data(),
		normals.size() * sizeof(float),
		GL_RGB32F
		);

	// Create TBO for texture coordinates (vec2 per vertex)
	texcoord_tbo_ = std::make_unique<TBO>(
		texcoords.data(),
		texcoords.size() * sizeof(float),
		GL_RG32F
		);

	// Create TBO for indices (uint per vertex)
	index_tbo_ = std::make_unique<TBO>(
		indices.data(),
		indices.size() * sizeof(unsigned int),
		GL_R32UI
		);
}

NAMESPACE_END(dream)