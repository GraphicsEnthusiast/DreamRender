#include <shape.h>
#include <tiny_obj_loader.h>
#include <utils.h>
#include <memory>

NAMESPACE_BEGIN(dream)

bool Texture::IsValid() const noexcept {
	return width > 0 && height > 0 && !data.empty();
}

unsigned int Texture::GetDataSize() const noexcept {
	return static_cast<unsigned int>(data.size() * sizeof(float));
}

int Texture::GetPixelCount() const noexcept {
	return width * height;
}

Medium::Medium()
	: phase_type(PhaseType::HenyeyGreenstein)
	, g(0.0f)
	, type(MediumType::HOMOGENEOUS)
	, sigma_s(0.5f)
	, sigma_t(1.0f) {
}

Material::Material()
	: type(MaterialType::DIFFUSE)
	, diffuse(0.8f, 0.8f, 0.8f)
	, roughness(0.5f)
	, emission(0.0f, 0.0f, 0.0f) {}

bool Material::HasTexture(TextureType type) const {
	auto it = texture_ids.find(type);

	return texture_ids.end() != it && it->second >= 0;
}

int Material::GetTextureID(TextureType type) const {
	auto it = texture_ids.find(type);
	if (texture_ids.end() != it) {
		return it->second;
	}

	return -1;
}

void Material::SetTexture(TextureType type, int texture_id) {
	if (texture_id < 0) {
		WARN("[warning] Setting invalid texture ID ({}) for texture type {}.", texture_id, static_cast<int>(type));

		return;
	}

	texture_ids[type] = texture_id;

	INFO("[info] Set texture type {} to ID {}.", static_cast<int>(type), texture_id);
}

void Material::RemoveTexture(TextureType type) {
	auto it = texture_ids.find(type);
	if (it != texture_ids.end()) {
		texture_ids.erase(it);
		INFO("[info] Removed texture type {}.", static_cast<int>(type));
	}
	else {
		WARN("[warning] Attempted to remove non-existent texture type {}.", static_cast<int>(type));
	}
}

void Material::ClearTextures() {
	unsigned int count = texture_ids.size();
	texture_ids.clear();
	INFO("[info] Cleared all {} textures from material.", count);
}

unsigned int Material::GetTextureCount() const noexcept {
	unsigned int count = 0;

	for (const auto& pair : texture_ids) {
		if (pair.second >= 0) {
			++count;
		}
	}

	return count;
}

bool Material::HasTextures() const noexcept {
	return GetTextureCount() > 0;
}

TriangleMesh::TriangleMesh(const std::string& file, const Transform& trans,
	std::unique_ptr<Material> material,
	std::unique_ptr<Medium> in_medium,
	std::unique_ptr<Medium> out_medium)
	: transform_(trans)
	, material_(std::move(material))
	, in_medium_(std::move(in_medium))
	, out_medium_(std::move(out_medium)) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str()) || 0 == shapes.size()) {
		ERROR("[error] Load from obj {} failed!", file.c_str());

		return;
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
						attrib.texcoords[2 * static_cast<unsigned int>(idx.texcoord_index) + 0];
					const tinyobj::real_t ty =
						attrib.texcoords[2 * static_cast<unsigned int>(idx.texcoord_index) + 1];
					texcoords.push_back(Point2f(tx, ty));
				}
			}

			// if normals is empty, add geometric normal
			if (normals.empty()) {
				const Point3f v1 = glm::normalize(vertices[1] - vertices[0]);
				const Point3f v2 = glm::normalize(vertices[2] - vertices[0]);
				const Vector3f n = glm::normalize(glm::cross(v1, v2));
				normals.push_back(n);
				normals.push_back(n);
				normals.push_back(n);
			}

			// if texcoords is empty, add barycentric coords
			if (texcoords.empty()) {
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

const Material* TriangleMesh::GetMaterial() const noexcept {
	return material_.get();
}

void TriangleMesh::SetMaterial(std::unique_ptr<Material> material) {
	if (!material) {
		ERROR("[error] Cannot set null material for triangle mesh");
		return;
	}
	material_ = std::move(material);
}

const Medium* TriangleMesh::GetInMedium() const noexcept {
	return in_medium_.get();
}

void TriangleMesh::SetInMedium(std::unique_ptr<Medium> medium) {
	in_medium_ = std::move(medium);
}

const Medium* TriangleMesh::GetOutMedium() const noexcept {
	return out_medium_.get();
}

void TriangleMesh::SetOutMedium(std::unique_ptr<Medium> medium) {
	out_medium_ = std::move(medium);
}

NAMESPACE_END(dream)