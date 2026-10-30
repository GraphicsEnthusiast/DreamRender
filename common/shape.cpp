#include <shape.h>
#include <tiny_obj_loader.h>
#include <utils.h>

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
		ERROR("[error] Cannot set null material for triangle mesh.");

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

Point3f TriangleMesh::GetMinBound() const noexcept {
	if (vertices_.empty()) {
		return Point3f(0.0f);
	}

	Point3f min_bound(std::numeric_limits<float>::max());

	for (int i = 0; i < vertices_.size(); i += 3) {
		min_bound.x = std::min(min_bound.x, vertices_[i]);
		min_bound.y = std::min(min_bound.y, vertices_[i + 1]);
		min_bound.z = std::min(min_bound.z, vertices_[i + 2]);
	}

	return min_bound;
}

Point3f TriangleMesh::GetMaxBound() const noexcept {
	if (vertices_.empty()) {
		return Point3f(0.0f);
	}

	Point3f max_bound(std::numeric_limits<float>::lowest());

	for (int i = 0; i < vertices_.size(); i += 3) {
		max_bound.x = std::max(max_bound.x, vertices_[i]);
		max_bound.y = std::max(max_bound.y, vertices_[i + 1]);
		max_bound.z = std::max(max_bound.z, vertices_[i + 2]);
	}

	return max_bound;
}

void TrimLeft(std::string& s) {
	while (!s.empty() && (s[0] == ' ' || s[0] == '\t' || s[0] == '\r' || s[0] == '\n')) {
		s.erase(s.begin());
	}
}

bool ParseFloat(std::stringstream& ss, float& out) {
	std::string tok;
	if (!(ss >> tok)) {
		return false;
	}
	out = static_cast<float>(atof(tok.c_str()));

	return true;
}

bool ParseInt(std::stringstream& ss, int& out) {
	std::string tok;
	if (!(ss >> tok)) {
		return false;
	}
	out = atoi(tok.c_str());

	return true;
}

float Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

// Wrap horizontal angle into [0, 2PI)
float WrapTwoPi(float a) {
	a = std::fmod(a, 2.0f * PI);
	if (a < 0.0f) {
		a += 2.0f * PI;
	}

	return a;
}

// Vertical Catmull-Rom interpolation for a given horizontal row
float InterpVert(const IESProfile& p, int v_idx, float v_frac, int h) {
	const auto& table = p.candela_table;
	int nv = static_cast<int>(p.angles_v_rad.size());

	auto Get = [&](int vi) -> float {
		if (vi < 0) vi = 0;
		if (vi > nv - 1) vi = nv - 1;
		return table[h][vi];
	};

	float a = Get((v_idx == 0) ? 1 : v_idx - 1);
	float b = Get(v_idx);
	float c = Get(v_idx + 1);
	float d = Get(std::min(v_idx + 2, nv - 1));

	return IESLoader::Cubic(a, b, c, d, v_frac);
}

#pragma pack(push, 1)
struct BmpFileHeader {
	uint16_t signature;
	uint32_t file_size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t data_offset;
};

struct BmpInfoHeader {
	uint32_t header_size;
	int32_t  width;
	int32_t  height;
	uint16_t planes;
	uint16_t bpp;
	uint32_t compression;
	uint32_t image_size;
	int32_t  xppm;
	int32_t  yppm;
	uint32_t colors_used;
	uint32_t important_colors;
};

struct RgbQuad {
	uint8_t b, g, r, a;
};
#pragma pack(pop)

bool IESProfile::IsValid() const noexcept {
	return !angles_v_rad.empty() && !angles_h_rad.empty() &&
		!candela_table.empty() && angles_h_rad.back() > PI;
}

bool IESProfile::IsDisk() const noexcept {
	return LuminousShape::DISK == shape;
}

bool IESProfile::IsRect() const noexcept {
	return LuminousShape::RECT == shape;
}

float IESProfile::MaxCandela() const noexcept {
	float m = 0.0f;
	for (const auto& row : candela_table) {
		for (float v : row) {
			m = std::max(m, v);
		}
	}

	return m;
}

float IESLoader::Cubic(float p0, float p1, float p2, float p3, float t) {
	float t2 = t * t;
	float t3 = t2 * t;
	return 0.5f * (
		(2.0f * p1) +
		(-p0 + p2) * t +
		(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
		(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
		);
}

float IESLoader::FindSegment(const std::vector<float>& angles, float val) {
	if (angles.empty()) {
		return 0.0f;
	}

	const int n = static_cast<int>(angles.size());
	if (val <= angles[0]) {
		return 0.0f;
	}
	if (val >= angles[n - 1]) {
		return static_cast<float>(n - 1) - 1e-3f;
	}

	int lo = 0;
	int hi = n - 1;
	while (hi - lo > 1) {
		int mid = (lo + hi) >> 1;
		if (angles[mid] <= val) {
			lo = mid;
		}
		else {
			hi = mid;
		}
	}

	float denom = angles[lo + 1] - angles[lo];
	float f = (val - angles[lo]) / (denom > EPS ? denom : EPS);
	f = std::min(1.0f, std::max(0.0f, f));

	return static_cast<float>(lo) + f;
}

bool IESLoader::Load(const std::string& path, IESProfile& target) {
	std::ifstream fin(path, std::ios::binary);
	if (!fin.is_open()) {
		ERROR("[error] Failed to open IES file: {}", path);

		return false;
	}

	std::stringstream buf;
	buf << fin.rdbuf();

	std::string line;
	if (!std::getline(buf, target.version)) {
		ERROR("[error] Empty IES file.");

		return false;
	}
	TrimLeft(target.version);

	bool tilt_ok = false;
	while (std::getline(buf, line)) {
		std::string l = line;
		TrimLeft(l);
		if (l.find("TILT=") == 0) {
			if (l.find("TILT=NONE") == std::string::npos) {
				ERROR("[error] Only TILT=NONE supported.");
				return false;
			}
			tilt_ok = true;
			break;
		}
	}
	if (!tilt_ok) {
		ERROR("[error] Missing TILT keyword.");

		return false;
	}

	std::string rest, ln;
	while (std::getline(buf, ln)) {
		rest += " " + ln;
	}

	std::stringstream hs(rest);

	int num_v = 0;
	int num_h = 0;
	float unused = 0.0f;

	if (!ParseFloat(hs, target.total_lights)) {
		return false;
	}
	if (!ParseFloat(hs, target.total_lumens)) {
		return false;
	}
	if (!ParseFloat(hs, target.candela_multiplier)) {
		return false;
	}
	if (!ParseInt(hs, num_v)) {
		return false;
	}
	if (!ParseInt(hs, num_h)) {
		return false;
	}

	int photo = 2;
	if (!ParseInt(hs, photo)) {
		return false;
	}
	switch (photo) {
		case 1: target.photometric_type = IESPhotometricType::TYPE_C; break;
		case 2: target.photometric_type = IESPhotometricType::TYPE_B; break;
		case 3: target.photometric_type = IESPhotometricType::TYPE_A; break;
		default: target.photometric_type = IESPhotometricType::TYPE_C; break;
	}
	if (IESPhotometricType::TYPE_A == target.photometric_type) {
		return false;
	}

	if (!ParseInt(hs, target.units)) {
		return false;
	}
	if (!ParseFloat(hs, target.width)) {
		return false;
	}
	if (!ParseFloat(hs, target.length)) {
		return false;
	}
	if (!ParseFloat(hs, target.height)) {
		return false;
	}
	if (!ParseFloat(hs, target.ballast_factor)) {
		return false;
	}
	if (!ParseFloat(hs, unused)) {
		return false;
	}
	if (!ParseFloat(hs, target.input_watts)) {
		return false;
	}

	if (1 == target.units) {  // feet -> meters
		target.width *= 0.3048f;
		target.length *= 0.3048f;
		target.height *= 0.3048f;
	}

	target.raw_vertical_angles.resize(num_v);
	for (int i = 0; i < num_v; ++i) {
		if (!ParseFloat(hs, target.raw_vertical_angles[i])) {
			return false;
		}
	}

	target.raw_horizontal_angles.resize(num_h);
	for (int i = 0; i < num_h; ++i) {
		if (!ParseFloat(hs, target.raw_horizontal_angles[i])) {
			return false;
		}
	}

	target.raw_candela.assign(num_h, std::vector<float>(num_v, 0.0f));
	for (int h = 0; h < num_h; ++h) {
		for (int v = 0; v < num_v; ++v) {
			float cd;
			if (!ParseFloat(hs, cd)) {
				return false;
			}
			target.raw_candela[h][v] = cd * target.candela_multiplier * target.ballast_factor;
		}
	}

	INFO("[info] Loaded IES '{}': {}H x {}V", path, num_h, num_v);

	// Infer luminous opening shape from original signed dimensions
	// (must run before Process mutates anything; relies on raw width/length/height)
	target.shape = IESLoader::InferShape(target);

	INFO("[info] Inferred luminous shape: {}", target.IsDisk() ? "DISK" : "RECT");

	return Process(target);
}

LuminousShape IESLoader::InferShape(const IESProfile& profile) {
	const float w = profile.width;
	const float l = profile.length;
	const float h = profile.height;

	const bool w_neg = w < -EPS;
	const bool w_pos = w > EPS;
	const bool l_zero = std::fabs(l) <= EPS;
	const bool h_zero = std::fabs(h) <= EPS;
	const bool all_zero = std::fabs(w) <= EPS &&
		std::fabs(l) <= EPS &&
		std::fabs(h) <= EPS;

	// Point light: no luminous area
	if (all_zero) {
		ERROR("[warn] IES reports point-light (0,0,0); area-light path unsupported.");

		return LuminousShape::RECT; // fallback, but logged
	}

	// Circular luminaire: negative width = diameter, no length/height
	if (w_neg && l_zero && h_zero) {
		return LuminousShape::DISK;
	}

	// Explicit rectangle: positive width and length
	if (w_pos && !l_zero) {
		return LuminousShape::RECT;
	}

	// Any other signed combination (ellipse/cylinder/sphere) is unsupported
	ERROR("[warn] Unsupported IES luminous shape (w={}, l={}, h={}); falling back to RECT.", w, l, h);

	return LuminousShape::RECT;
}

float IESLoader::ComputeTotalPower(const IESProfile& profile) {
	const auto& h = profile.angles_h_rad;
	const auto& v = profile.angles_v_rad;
	const auto& cd = profile.candela_table;

	const int nh = static_cast<int>(h.size());
	const int nv = static_cast<int>(v.size());
	if (nh < 2 || nv < 2) {
		return 0.0f;
	}

	// Integrate vertically for each horizontal angle.
	std::vector<float> G(nh, 0.0f);
	for (int i = 0; i < nh; ++i) {
		float sum = 0.0f;
		for (int j = 0; j < nv - 1; ++j) {
			const float v0 = v[j];
			const float v1 = v[j + 1];
			const float dv = v1 - v0;
			if (dv <= 0.0f) {
				continue;
			}

			const float c0 = cd[i][j];
			const float c1 = cd[i][j + 1];

			// Exact integral of sin(v) over [v0, v1].
			const float sin_integral = std::cos(v0) - std::cos(v1);

			// Exact integral for linearly interpolated candela:
			// กา I(v) sin(v) dv = c0 * (cos v0 - cos v1)
			//   + (c1 - c0) / dv * (sin v1 - sin v0 - dv * cos v1)
			const float sin_v0 = std::sin(v0);
			const float sin_v1 = std::sin(v1);
			const float linear_correction =
				(c1 - c0) / dv * (sin_v1 - sin_v0 - dv * std::cos(v1));

			sum += c0 * sin_integral + linear_correction;
		}
		G[i] = sum;
	}

	// Integrate horizontally with the trapezoidal rule.
	float lumens = 0.0f;
	for (int i = 0; i < nh - 1; ++i) {
		lumens += (G[i] + G[i + 1]) * 0.5f * (h[i + 1] - h[i]);
	}

	return lumens;
}

bool IESLoader::Process(IESProfile& profile) {
	if (profile.raw_candela.empty()) {
		return false;
	}

	auto& h = profile.raw_horizontal_angles;
	auto& v = profile.raw_vertical_angles;
	auto& cd = profile.raw_candela;

	if (IESPhotometricType::TYPE_B == profile.photometric_type) {
		// Transpose candela table and swap angle axes
		std::vector<std::vector<float>> new_cd;
		new_cd.resize(v.size());
		for (int i = 0; i < v.size(); ++i) {
			new_cd[i].reserve(h.size());
			for (int j = 0; j < h.size(); ++j) {
				new_cd[i].push_back(cd[j][i]);
			}
		}
		cd.swap(new_cd);
		h.swap(v);

		float h_first = h[0];
		float h_last = h.back();
		if (h_last != 90.0f) {
			return false;
		}

		if (0.0f == h_first) {
			// Mirror horizontal quadrant to full 180-degree range
			std::vector<float> new_h;
			std::vector<std::vector<float>> new_cd2;
			int hnum = (int)h.size();
			for (int i = hnum - 1; i > 0; --i) {
				new_h.push_back(90.0f - h[i]);
				new_cd2.push_back(cd[i]);
			}
			for (int i = 0; i < hnum; ++i) {
				new_h.push_back(90.0f + h[i]);
				new_cd2.push_back(cd[i]);
			}
			h.swap(new_h);
			cd.swap(new_cd2);
		}
		else if (-90.0f == h_first) {
			// Shift full-range horizontal angles into [0, 180]
			for (int i = 0; i < h.size(); ++i) {
				h[i] += 90.0f;
			}
		}

		// Close horizontal circle by duplicating 0-degree entry at 360
		h.push_back(360.0f);
		cd.push_back(cd[0]);

		float v_first = v[0];
		float v_last = v.back();
		if (90.0f != v_last) {
			return false;
		}

		if (0.0f == v_first) {
			// Mirror vertical quadrant to full 180-degree range
			std::vector<float> new_v;
			int hnum = (int)h.size();
			int vnum = (int)v.size();
			for (int i = vnum - 1; i > 0; --i) {
				new_v.push_back(90.0f - v[i]);
			}
			for (int i = 0; i < vnum; ++i) {
				new_v.push_back(90.0f + v[i]);
			}

			for (int i = 0; i < hnum; ++i) {
				std::vector<float> new_row;
				new_row.reserve(2 * vnum - 1);
				for (int j = vnum - 2; j >= 0; --j) {
					new_row.push_back(cd[i][j]);
				}
				new_row.insert(new_row.end(), cd[i].begin(), cd[i].end());
				cd[i].swap(new_row);
			}
			v.swap(new_v);
		}
		else if (-90.0f == v_first) {
			for (int i = 0; i < v.size(); ++i) {
				v[i] += 90.0f;
			}
		}
	}
	else {
		// Type C (and default): normalize horizontal range to full 360 degrees
		if (90.0f == h[0]) {
			for (int i = 0; i < h.size(); ++i) {
				h[i] -= 90.0f;
			}
		}
		if (0.0f != h[0]) {
			return false;
		}

		if (1 == h.size()) {
			h.push_back(360.0f);
			cd.push_back(cd[0]);
		}

		if (90.0f == h.back()) {
			int hnum = (int)h.size();
			for (int i = hnum - 2; i >= 0; --i) {
				h.push_back(180.0f - h[i]);
				cd.push_back(cd[i]);
			}
		}

		if (180.0f == h.back()) {
			int hnum = (int)h.size();
			for (int i = hnum - 2; i >= 0; --i) {
				h.push_back(360.0f - h[i]);
				cd.push_back(cd[i]);
			}
		}

		if (360.0f != h.back()) {
			int hnum = (int)h.size();
			float last_step = h[hnum - 1] - h[hnum - 2];
			float first_step = h[1] - h[0];
			float diff = 360.0f - h[hnum - 1];
			if (last_step == diff || first_step == diff) {
				h.push_back(360.0f);
				cd.push_back(cd[0]);
			}
			else {
				return false;
			}
		}

		float v_first = v[0];
		float v_last = v.back();
		if (90.0f == v_first) {
			if (180.0f == v_last) {
				for (int i = 0; i < v.size(); ++i) {
					v[i] = 180.0f - v[i];
				}
			}
			else {
				return false;
			}
		}
		else if (0.0f != v_first) {
			return false;
		}
	}

	// Convert degrees to radians
	profile.angles_h_rad = h;
	profile.angles_v_rad = v;

	for (float& a : profile.angles_h_rad) {
		a *= PI / 180.0f;
	}
	for (float& a : profile.angles_v_rad) {
		a *= PI / 180.0f;
	}

	profile.candela_table = cd;

	INFO("[info] Processed IES: {}H x {}V (radians)", profile.angles_h_rad.size(), profile.angles_v_rad.size());

	profile.power = IESLoader::ComputeTotalPower(profile);

	return true;
}

float IESLoader::SampleBilinear(const IESProfile& profile, float h_angle, float v_angle) {
	if (!profile.IsValid()) {
		return 0.0f;
	}

	h_angle = WrapTwoPi(h_angle);
	v_angle = std::min(PI, std::max(0.0f, v_angle));

	const auto& ah = profile.angles_h_rad;
	const auto& av = profile.angles_v_rad;

	int nh = static_cast<int>(ah.size());
	int nv = static_cast<int>(av.size());

	if (v_angle >= av.back()) {
		return 0.0f;
	}

	float fh = FindSegment(ah, h_angle);
	float fv = FindSegment(av, v_angle);

	int hi = static_cast<int>(std::floor(fh));
	int vi = static_cast<int>(std::floor(fv));
	float th = fh - hi;
	float tv = fv - vi;

	auto Get = [&](int h, int v) -> float {
		h = (h % nh + nh) % nh;
		v = std::min(nv - 1, std::max(0, v));
		return profile.candela_table[h][v];
	};

	int h0 = hi;
	int h1 = std::min(nh - 1, hi + 1);
	int v0 = vi;
	int v1 = std::min(nv - 1, vi + 1);

	float c00 = Get(h0, v0);
	float c10 = Get(h1, v0);
	float c01 = Get(h0, v1);
	float c11 = Get(h1, v1);

	float c0 = Lerp(c00, c10, th);
	float c1 = Lerp(c01, c11, th);
	return std::max(0.0f, Lerp(c0, c1, tv));
}

float IESLoader::SampleCubic(const IESProfile& profile, float h_angle, float v_angle) {
	if (!profile.IsValid()) {
		return 0.0f;
	}

	h_angle = WrapTwoPi(h_angle);
	v_angle = std::min(PI, std::max(0.0f, v_angle));

	const auto& ah = profile.angles_h_rad;
	const auto& av = profile.angles_v_rad;
	int nh = static_cast<int>(ah.size());
	int nv = static_cast<int>(av.size());

	if (v_angle >= av.back()) {
		return 0.0f;
	}

	float fh = FindSegment(ah, h_angle);
	float fv = FindSegment(av, v_angle);

	int hi = static_cast<int>(std::floor(fh));
	int vi = static_cast<int>(std::floor(fv));
	float th = fh - hi;
	float tv = fv - vi;

	// Horizontal wrap indices for seamless cubic interpolation
	int hm1 = (0 == hi) ? nh - 2 : hi - 1;
	int h2 = (hi + 2 == nh) ? 1 : hi + 2;

	float a = InterpVert(profile, vi, tv, hm1);
	float b = InterpVert(profile, vi, tv, hi);
	float c = InterpVert(profile, vi, tv, hi + 1);
	float d = InterpVert(profile, vi, tv, h2);

	return std::max(0.0f, Cubic(a, b, c, d, th));
}

void IESLoader::Bake1D(const IESProfile& profile, std::vector<float>& out, int width) {
	out.resize(width, 0.0f);
	if (!profile.IsValid()) {
		return;
	}

	float max_cd = profile.MaxCandela();
	float inv = max_cd > EPS ? 1.0f / max_cd : 0.0f;

	for (int i = 0; i < width; ++i) {
		float v_angle = static_cast<float>(i) / static_cast<float>(width - 1) * PI;
		float sum = 0.0f;
		int nh = static_cast<int>(profile.angles_h_rad.size());
		for (int h = 0; h < nh; ++h) {
			sum += SampleBilinear(profile, profile.angles_h_rad[h], v_angle);
		}
		out[i] = (sum / static_cast<float>(std::max(1, nh))) * inv;
	}
}

void IESLoader::Bake2D(const IESProfile& profile, std::vector<float>& out,
	int width, int height) {
	out.resize(width * height, 0.0f);
	if (!profile.IsValid()) {
		return;
	}

	float max_cd = profile.MaxCandela();
	float inv = max_cd > EPS ? 1.0f / max_cd : 0.0f;

	for (int y = 0; y < height; ++y) {
		// y = 0 maps to top (vertical PI), y = height-1 maps to bottom (vertical 0)
		float v_angle = static_cast<float>(y) / static_cast<float>(height - 1) * PI;
		for (int x = 0; x < width; ++x) {
			float h_angle = static_cast<float>(x) / static_cast<float>(width) * (2.0f * PI);
			out[y * width + x] = SampleCubic(profile, h_angle, v_angle) * inv;
		}
	}
}

bool IESLoader::WritePreviewBMP(const IESProfile& profile, const std::string& path, int size) {
	if (!profile.IsValid() || size <= 0) {
		return false;
	}

	std::vector<float> tex;
	Bake2D(profile, tex, size, size);

	std::vector<uint8_t> pixels(static_cast<size_t>(size) * size, 0);
	float maxv = EPS;
	for (float v : tex) {
		maxv = std::max(maxv, v);
	}

	// Apply gamma for preview readability
	const float inv_gamma = 1.0f / 2.2f;
	for (int i = 0; i < size * size; ++i) {
		float n = tex[i] / maxv;
		n = std::pow(n, inv_gamma);
		pixels[i] = static_cast<uint8_t>(std::min(255, std::max(0, static_cast<int>(n * 255.0f))));
	}

	int stride = ((size + 3) / 4) * 4;
	int image_bytes = stride * size;

	BmpFileHeader fh;
	fh.signature = 0x4D42; // 'BM'
	fh.file_size = static_cast<uint32_t>(sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) +
		256 * sizeof(RgbQuad) + image_bytes);
	fh.reserved1 = 0;
	fh.reserved2 = 0;
	fh.data_offset = sizeof(BmpFileHeader) + sizeof(BmpInfoHeader) + 256 * sizeof(RgbQuad);

	BmpInfoHeader ih;
	ih.header_size = sizeof(BmpInfoHeader);
	ih.width = size;
	ih.height = size;
	ih.planes = 1;
	ih.bpp = 8;
	ih.compression = 0;
	ih.image_size = static_cast<uint32_t>(image_bytes);
	ih.xppm = 2835;
	ih.yppm = 2835;
	ih.colors_used = 256;
	ih.important_colors = 256;

	std::vector<RgbQuad> palette(256);
	for (int i = 0; i < 256; ++i) {
		palette[i] = { static_cast<uint8_t>(i), static_cast<uint8_t>(i), static_cast<uint8_t>(i), 0 };
	}

	std::ofstream f(path, std::ios::binary);
	if (!f.is_open()) {
		ERROR("[error] Cannot write BMP: {}", path);
		return false;
	}

	f.write(reinterpret_cast<const char*>(&fh), sizeof(fh));
	f.write(reinterpret_cast<const char*>(&ih), sizeof(ih));
	f.write(reinterpret_cast<const char*>(palette.data()), sizeof(RgbQuad) * 256);

	std::vector<uint8_t> row(stride, 0);
	for (int y = 0; y < size; ++y) {
		int sy = size - 1 - y; // flip vertically for image space
		for (int x = 0; x < size; ++x) {
			row[x] = pixels[sy * size + x];
		}
		f.write(reinterpret_cast<const char*>(row.data()), stride);
	}

	INFO("[info] Wrote IES preview: {}", path);

	return true;
}

NAMESPACE_END(dream)