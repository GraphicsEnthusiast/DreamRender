#include <Shape.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

Shape::Shape(shared_ptr<Material> mat, vec3 pos, mat4 trans, int geom_id) :
	material(mat), position(pos), transform(trans), geometry_id(geom_id) {}

TriangleMesh::TriangleMesh(shared_ptr<Material> mat, string file, mat4 trans, vec3 pos) :
	Shape(mat, pos, trans, true), filename(file) {
	m_type = ShapeType::TriangleMesh;
}

TriangleMesh::~TriangleMesh() {
	if (aligned_normals != NULL) {
		delete[] aligned_normals;
		aligned_normals = NULL;
	}
	if (aligned_uvs != NULL) {
		delete[] aligned_uvs;
		aligned_uvs = NULL;
	}
}

int TriangleMesh::LoadFromObj(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()) || shapes.size() == 0) {
		printf("LoadFromObj %s failed!", filename);

		return -1;
	}

	// Load vertices
	int vertCount = int(attrib.vertices.size() / 3);
	for (int i = 0; i < vertCount; i++) {
		vec4 vv(attrib.vertices[3 * i + 0], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2], 1);
		vv = transform * vv;
		vec3 v(vv.x, vv.y, vv.z);
		vertices.push_back(v);
	}

	// Loop over shapes
	bool hasNormals = true;
	vector<vec2> t_uvs;
	vector<vec3> t_normals;
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		int index_offset = 0;

		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			vec4 indices;
			vec2 uv;
			vec3 normal;
			float vx, vy, vz, nx, ny, nz, tx, ty;

			int fv = shapes[s].mesh.num_face_vertices[f];
			for (int i = 0; i < fv; i++) {
				tinyobj::index_t idx = tinyobj::index_t(shapes[s].mesh.indices[index_offset + i]);
				indices[i] = (3 * idx.vertex_index + i) / 3;
				indices_v.push_back(indices[i]);

				//Normals
				if (idx.normal_index != -1) {
					nx = attrib.normals[3 * idx.normal_index + 0];
					ny = attrib.normals[3 * idx.normal_index + 1];
					nz = attrib.normals[3 * idx.normal_index + 2];
					indices[i] = (3 * idx.normal_index + i) / 3;
					indices_n.push_back(indices[i]);
				}
				else {
					hasNormals = false;
				}

				//TexCoords
				if (idx.texcoord_index != -1) {
					tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					ty = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
					indices[i] = (2 * idx.texcoord_index + i) / 2;
					indices_t.push_back(indices[i]);
				}
				else {
					//If model has no texture Coords then default to 0
					tx = ty = 0;
				}

				normal = vec3(nx, ny, nz);
				uv = vec2(tx, ty);
//				cout << to_string(uv) << endl;
				t_normals.push_back(normal);
				t_uvs.push_back(uv);
			}
			index_offset += fv;
		}
	}

	unsigned int vertices_size = vertices.size();
	unsigned int indices_size = indices_v.size();

	normals.resize(vertices_size);
	uvs.resize(vertices_size);
	for (int i = 0; i < indices_v.size(); i += 3) {
		if (/*!hasNormals*/true) {//模型自带法线有些有问题，直接自己算
			vec3 p1 = vertices[indices_v[i]];
			vec3 p2 = vertices[indices_v[i + 1]];
			vec3 p3 = vertices[indices_v[i + 2]];
			vec3 n = normalize(cross(p2 - p1, p3 - p1));
			normals[indices_v[i]] = n;
			normals[indices_v[i + 1]] = n;
			normals[indices_v[i + 2]] = n;
		}
		else {
			normals[indices_v[i]] = t_normals[indices_n[i]];
			normals[indices_v[i + 1]] = t_normals[indices_n[i + 1]];
			normals[indices_v[i + 2]] = t_normals[indices_n[i + 2]];
		}
		uvs[indices_v[i]] = t_uvs[indices_t[i]];
		uvs[indices_v[i + 1]] = t_uvs[indices_t[i + 1]];
		uvs[indices_v[i + 2]] = t_uvs[indices_t[i + 2]];
		//		cout << n.x << " " << n.y << " " << n.z << endl;
	}

	// Initializing Embree geometry
	RTCGeometry mesh = rtcNewGeometry(rtc_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	// Setting and filling the vertex buffer
	vec3* embree_vertices = (vec3*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
		sizeof(vec3), vertices_size);
	for (int i = 0; i < vertices_size; ++i) {
		embree_vertices[i].x = vertices[i].x;
		embree_vertices[i].y = vertices[i].y;
		embree_vertices[i].z = vertices[i].z;
	}

	vertices.clear();
	aligned_normals = new vec3[vertices_size];
	aligned_uvs = new vec2[vertices_size];

		// Setting and filling the index buffer
	uvec3* triangles = (uvec3*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
		sizeof(uvec3), indices_size / 3);
	for (int i = 0; i < indices_size; i += 3) {
		triangles[i / 3].x = indices_v[i];
		triangles[i / 3].y = indices_v[i + 1];
		triangles[i / 3].z = indices_v[i + 2];

		aligned_normals[indices_v[i]] = normals[indices_v[i]];
		aligned_normals[indices_v[i + 1]] = normals[indices_v[i + 1]];
		aligned_normals[indices_v[i + 2]] = normals[indices_v[i + 2]];

		aligned_uvs[indices_v[i]] = uvs[indices_v[i]];
		aligned_uvs[indices_v[i + 1]] = uvs[indices_v[i + 1]];
		aligned_uvs[indices_v[i + 2]] = uvs[indices_v[i + 2]];
	}

	uvs.clear();
	normals.clear();
	indices_v.clear();
	indices_n.clear();
	indices_t.clear();

	// Setting the buffer with normals (in order to get interpolated normals almost for free (since the uv-values are already calculated for intersections))
	rtcSetGeometryVertexAttributeCount(mesh, 2);

	rtcSetSharedGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3, &aligned_normals[0], 0,
		sizeof(vec3), vertices_size);
	rtcSetSharedGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2, &aligned_uvs[0], 0,
		 sizeof(vec2), vertices_size);

		// Commiting the geometry
	rtcCommitGeometry(mesh);

	// Attaching it to the scene and getting the primitive's Id
	this->geometry_id = rtcAttachGeometry(rtc_scene, mesh);

	rtcReleaseGeometry(mesh);

	return 0;
}

int TriangleMesh::ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	size_t found = filename.find_last_of(".");
	string extStr = filename.substr(found + 1);

	if (extStr == "obj") {
		return LoadFromObj(rtc_device, rtc_scene);
	}
// 	else if (extStr == "ply") {
// 		//return LoadFromPly(rtc_device, rtc_scene);
// 	}

	return -1;
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