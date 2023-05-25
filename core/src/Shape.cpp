#include <Shape.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Shape::Shape(shared_ptr<Material> mat, vec3 pos, mat4 trans, int geom_id) :
	material(mat), position(pos), transform(trans), geometry_id(geom_id) {}

TriangleMesh::TriangleMesh(shared_ptr<Material> mat, string file, mat4 trans, vec3 pos) :
	Shape(mat, pos, trans), filename(file) {
	m_type = ShapeType::TriangleMesh;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()) || shapes.size() == 0) {
		printf("LoadFromObj %s failed!", filename);
	}

	// loop over shapes
	for (size_t s = 0; s < shapes.size(); ++s) {
		size_t index_offset = 0;
		// loop over faces
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			const size_t fv =
				static_cast<size_t>(shapes[s].mesh.num_face_vertices[f]);

			std::vector<vec3> vertices;
			std::vector<vec3> normals;
			std::vector<vec2> texcoords;

			// loop over vertices
			// get vertices, normals, texcoords of a triangle
			for (size_t v = 0; v < fv; ++v) {
				const tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				const tinyobj::real_t vx =
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
				const tinyobj::real_t vy =
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
				const tinyobj::real_t vz =
					attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];

				vec4 v_object(vx, vy, vz, 1.0f);
				vec3 v_world = transform * v_object;
				vertices.push_back(vec3(v_world.x, v_world.y, v_world.z));

				if (idx.normal_index >= 0) {
					const tinyobj::real_t nx =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
					const tinyobj::real_t ny =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
					const tinyobj::real_t nz =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];

					vec4 n_object(nx, ny, nz, 0.0f);
					vec3 n_world = transform * n_object;
					normals.push_back(normalize(vec3(n_world.x, n_world.y, n_world.z)));
				}

				if (idx.texcoord_index >= 0) {
					const tinyobj::real_t tx =
						attrib
						.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
					const tinyobj::real_t ty =
						attrib
						.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
					texcoords.push_back(vec2(tx, ty));
				}
			}

			// if normals is empty, add geometric normal
			if (normals.size() == 0) {
				const vec3 v1 = normalize(vertices[1] - vertices[0]);
				const vec3 v2 = normalize(vertices[2] - vertices[0]);
				const vec3 n = normalize(cross(v1, v2));
				normals.push_back(n);
				normals.push_back(n);
				normals.push_back(n);
			}

			// if texcoords is empty, add barycentric coords
			if (texcoords.size() == 0) {
				texcoords.push_back(vec2(0.0f));
				texcoords.push_back(vec2(1.0f, 0.0f));
				texcoords.push_back(vec2(0.0f, 1.0f));
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
}

int TriangleMesh::ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	RTCGeometry geom = rtcNewGeometry(rtc_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	// set vertices
	float* vb = (float*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0,
		RTC_FORMAT_FLOAT3,
		3 * sizeof(float),
		Vertices());
	for (int i = 0; i < vertices.size(); ++i) {
		vb[i] = vertices[i];
	}

	// set indices
	unsigned* ib = (unsigned*)rtcSetNewGeometryBuffer(
		geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned),
		Faces());
	for (int i = 0; i < indices.size(); ++i) {
		ib[i] = indices[i];
	}

	rtcCommitGeometry(geom);
	this->geometry_id = rtcAttachGeometry(rtc_scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(rtc_scene);

	return 0;
}

float TriangleMesh::Pdf(const IntersectionInfo& info, const vec3& L, float dist) {
	return 0.0f;
}

Sphere::Sphere(shared_ptr<Material> mat, vec3 cen, float rad, vec3 pos, mat4 trans) :
	Shape(mat, pos, trans), radius(rad), center(cen) {
	m_type = ShapeType::Sphere;
}

// Bounding box construction routine
void Sphere::SphereBoundsFunc(const struct RTCBoundsFunctionArguments* args) {
	const Sphere* spheres = (const Sphere*)args->geometryUserPtr;
	RTCBounds* bounds_o = args->bounds_o;
	const Sphere& sphere = spheres[args->primID];

	bounds_o->lower_x = sphere.center.x - sphere.radius;
	bounds_o->lower_y = sphere.center.y - sphere.radius;
	bounds_o->lower_z = sphere.center.z - sphere.radius;
	bounds_o->upper_x = sphere.center.x + sphere.radius;
	bounds_o->upper_y = sphere.center.y + sphere.radius;
	bounds_o->upper_z = sphere.center.z + sphere.radius;
}

// Intersection routine
void Sphere::SphereIntersectFunc(const RTCIntersectFunctionNArguments* args) {
	int* valid = args->valid;
	void* ptr = args->geometryUserPtr;
	RTCRayHit* rayhit = (RTCRayHit*)args->rayhit;
	unsigned int primID = args->primID;

	vec3 org(rayhit->ray.org_x, rayhit->ray.org_y, rayhit->ray.org_z);
	vec3 dir(rayhit->ray.dir_x, rayhit->ray.dir_y, rayhit->ray.dir_z);

	assert(args->N == 1);

	const Sphere* spheres = (const Sphere*)ptr;
	const Sphere& sphere = spheres[primID];

	if (!valid[0]) {
		return;
	}

	vec3 center = sphere.center;

	const vec3 op = center - org;
	const float dop = dot(dir, op);
	const float D = dop * dop - dot(op, op) + sphere.radius * sphere.radius;

	if (D < 0.0f) {
		return;
	}

	const float sqrtD = sqrt(D);

	const float tmin = dop - sqrtD;
	if (rayhit->ray.tnear < tmin && tmin < rayhit->ray.tfar) {
		rayhit->ray.tfar = tmin;
		vec3 p = GetHitPos(*rayhit);
		vec2 uv = GetSphereUV(p, center);

		rayhit->hit.u = uv.x;
		rayhit->hit.v = uv.y;
		rayhit->hit.geomID = sphere.geometry_id;
		rayhit->hit.primID = primID;
		vec3 ng = normalize(org + dir * tmin - center);
		rayhit->hit.Ng_x = ng.x;
		rayhit->hit.Ng_y = ng.y;
		rayhit->hit.Ng_z = ng.z;
	}

	const float tmax = dop + sqrtD;
	if (rayhit->ray.tnear < tmax && tmax < rayhit->ray.tfar) {
		rayhit->ray.tfar = tmax;
		vec3 p = GetHitPos(*rayhit);
		vec2 uv = GetSphereUV(p, center);

		rayhit->hit.u = uv.x;
		rayhit->hit.v = uv.y;
		rayhit->hit.geomID = sphere.geometry_id;
		rayhit->hit.primID = primID;
		vec3 ng = normalize(org + dir * tmax - center);
		rayhit->hit.Ng_x = ng.x;
		rayhit->hit.Ng_y = ng.y;
		rayhit->hit.Ng_z = ng.z;
	}

	return;
}

// Occlusion routine
void Sphere::SphereOccludedFunc(const RTCOccludedFunctionNArguments* args) {
	// TODO: Implement the occlusion routine and make sure to use it for shadow rays or something similar
	// Hint: It's almost the same as sphereIntersectFunc
	int* valid = args->valid;
	void* ptr = args->geometryUserPtr;
	RTCRay* ray = (RTCRay*)args->ray;
	unsigned int primID = args->primID;

	vec3 org(ray->org_x, ray->org_y, ray->org_z);
	vec3 dir(ray->dir_x, ray->dir_y, ray->dir_z);

	assert(args->N == 1);

	const Sphere* spheres = (const Sphere*)ptr;
	const Sphere& sphere = spheres[primID];

	if (!valid[0]) {
		return;
	}

	vec3 center = sphere.center;

	const vec3 op = center - org;
	const float dop = dot(dir, op);
	const float D = dop * dop - dot(op, op) + sphere.radius * sphere.radius;

	if (D < 0.0f) {
		return;
	}

	const float sqrtD = sqrt(D);

	const float tmin = dop - sqrtD;
	if (ray->tnear < tmin && tmin < ray->tfar) {
		ray->tfar = tmin;
	}

	const float tmax = dop + sqrtD;
	if (ray->tnear < tmax && tmax < ray->tfar) {
		ray->tfar = tmax;
	}

	return;
}

// Construction of Embree object from the analytically given sphere
int Sphere::ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	RTCGeometry geom = rtcNewGeometry(rtc_device, RTC_GEOMETRY_TYPE_USER);
	this->geometry_id = rtcAttachGeometry(rtc_scene, geom);

	rtcSetGeometryUserPrimitiveCount(geom, 1);
	rtcSetGeometryUserData(geom, this);

	rtcSetGeometryBoundsFunction(geom, SphereBoundsFunc, nullptr);
	rtcSetGeometryIntersectFunction(geom, SphereIntersectFunc);
	rtcSetGeometryOccludedFunction(geom, SphereOccludedFunc);
	rtcCommitGeometry(geom);
	rtcReleaseGeometry(geom);

	return 0;
}

float Sphere::Pdf(const IntersectionInfo& info, const vec3& L, float dist) {
	float area = 4.0f * PI * radius * radius;

	return (dist * dist) / (area * abs(dot(info.normal, L)));
}

vec2 Sphere::GetSphereUV(const vec3& p, const vec3& center) {
	vec3 object_p = normalize(p - center);

	vec2 uv;
	float phi = atan2(object_p.z, object_p.x);//点x,y,z的方位角, PI~-PI=>1~0=>0~1
	float theta = asin(object_p.y);//点x,y,z的天顶角, -PI/2~PI/2=>0~1
	uv.x = 1.0f - (phi + PI) * INV_2PI;
	uv.y = (theta + PI / 2.0f) * INV_PI;

	return uv;
}

Quad::Quad(shared_ptr<Material> mat, vec3 p, vec3 _u, vec3 _v, vec3 pos, mat4 trans) : Shape(mat, pos, trans) {
	position = p;
	u = _u;
	v = _v;
	m_type = ShapeType::Quad;
}

vec2 Quad::GetQuadUV(const vec3& p, const Quad* quad) {
	vec2 uv;
	vec3 op = p - quad->position;
	float a1 = dot(normalize(quad->u), op);
	float a2 = dot(normalize(quad->v), op);
	uv.y = a1 / length(quad->u);
	uv.x = a2 / length(quad->v);

	return uv;
}

int Quad::ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	vec3 p[4] = { position, position + u, position + u + v, position + v };

	// Initializing Embree geometry
	RTCGeometry mesh = rtcNewGeometry(rtc_device, RTC_GEOMETRY_TYPE_QUAD);

	// Setting and filling the vertex buffer
	vec3* embree_vertices = (vec3*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
		sizeof(vec3), 4);
	for (int i = 0; i < 4; ++i) {
		embree_vertices[i].x = p[i].x;
		embree_vertices[i].y = p[i].y;
		embree_vertices[i].z = p[i].z;
	}

	// Setting and filling the index buffer
	uvec4* quads = (uvec4*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT4,
		sizeof(uvec4), 1);
	quads[0].x = 0;
	quads[0].y = 1;
	quads[0].z = 2;
	quads[0].w = 3;

	// Commiting the geometry
	rtcCommitGeometry(mesh);

	// Attaching it to the scene and getting the primitive's Id
	this->geometry_id = rtcAttachGeometry(rtc_scene, mesh);

	rtcReleaseGeometry(mesh);

	return 0;
}

float Quad::Pdf(const IntersectionInfo& info, const vec3& L, float dist) {
	float area = length(u) * length(v);

	return (dist * dist) / (area * abs(dot(info.normal, L)));
}
