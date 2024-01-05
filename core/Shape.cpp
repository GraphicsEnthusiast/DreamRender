#include "Shape.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

TriangleMesh::TriangleMesh(std::shared_ptr<Material> m, const std::string& file, const Transform& trans, std::shared_ptr<Medium> out, std::shared_ptr<Medium> in) : 
	Shape(ShapeType::TriangleMeshShape, m, trans, out, in) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str()) || shapes.size() == 0) {
		printf("LoadFromObj %s failed!", file.c_str());
		assert(0);
	}

	// loop over shapes
	for (size_t s = 0; s < shapes.size(); ++s) {
		size_t index_offset = 0;
		// loop over faces
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
			const size_t fv =
				static_cast<size_t>(shapes[s].mesh.num_face_vertices[f]);

			std::vector<Point3f> vertices;
			std::vector<Vector3f> normals;
			std::vector<Point2f> texcoords;

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

				Point3f v_world = transform.TransformPoint(Point3f(vx, vy, vz));
				vertices.push_back(Point3f(v_world.x, v_world.y, v_world.z));

				if (idx.normal_index >= 0) {
					const tinyobj::real_t nx =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
					const tinyobj::real_t ny =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
					const tinyobj::real_t nz =
						attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];

					Vector3f n_world = transform.TransformVector(Vector3f(nx, ny, nz));
					normals.push_back(glm::normalize(Vector3f(n_world.x, n_world.y, n_world.z)));
				}

				if (idx.texcoord_index >= 0) {
					const tinyobj::real_t tx =
						attrib
						.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
					const tinyobj::real_t ty =
						attrib
						.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
					texcoords.push_back(Point2f(tx, ty));
				}
			}

			// if normals is empty, add geometric normal
			if (normals.size() == 0) {
				const Point3f v1 = glm::normalize(vertices[1] - vertices[0]);
				const Point3f v2 = glm::normalize(vertices[2] - vertices[0]);
				const Vector3f n = glm::normalize(glm::cross(v1, v2));
				normals.push_back(n);
				normals.push_back(n);
				normals.push_back(n);
			}

			// if texcoords is empty, add barycentric coords
			if (texcoords.size() == 0) {
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

	Point3f org(rayhit->ray.org_x, rayhit->ray.org_y, rayhit->ray.org_z);
	Vector3f dir(rayhit->ray.dir_x, rayhit->ray.dir_y, rayhit->ray.dir_z);

	assert(args->N == 1);

	const Sphere* spheres = (const Sphere*)ptr;
	const Sphere& sphere = spheres[primID];

	if (!valid[0]) {
		return;
	}

	Point3f center = sphere.center;

	const Vector3f op = center - org;
	const float dop = glm::dot(dir, op);
	const float D = dop * dop - glm::dot(op, op) + sphere.radius * sphere.radius;

	if (D < 0.0f) {
		return;
	}

	const float sqrtD = sqrt(D);

	const float tmin = dop - sqrtD;
	if (rayhit->ray.tnear < tmin && tmin < rayhit->ray.tfar) {
		rayhit->ray.tfar = tmin;
		Point3f p = GetHitPos(*rayhit);
		Point2f uv = GetSphereUV(p, center);

		rayhit->hit.u = uv.x;
		rayhit->hit.v = uv.y;
		rayhit->hit.geomID = sphere.geometry_id;
		rayhit->hit.primID = primID;
		Vector3f ng = glm::normalize(org + dir * tmin - center);
		rayhit->hit.Ng_x = ng.x;
		rayhit->hit.Ng_y = ng.y;
		rayhit->hit.Ng_z = ng.z;
	}

	const float tmax = dop + sqrtD;
	if (rayhit->ray.tnear < tmax && tmax < rayhit->ray.tfar) {
		rayhit->ray.tfar = tmax;
		Point3f p = GetHitPos(*rayhit);
		Point2f uv = GetSphereUV(p, center);

		rayhit->hit.u = uv.x;
		rayhit->hit.v = uv.y;
		rayhit->hit.geomID = sphere.geometry_id;
		rayhit->hit.primID = primID;
		Vector3f ng = glm::normalize(org + dir * tmax - center);
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

	Point3f org(ray->org_x, ray->org_y, ray->org_z);
	Vector3f dir(ray->dir_x, ray->dir_y, ray->dir_z);

	assert(args->N == 1);

	const Sphere* spheres = (const Sphere*)ptr;
	const Sphere& sphere = spheres[primID];

	if (!valid[0]) {
		return;
	}

	Point3f center = sphere.center;

	const Vector3f op = center - org;
	const float dop = glm::dot(dir, op);
	const float D = dop * dop - glm::dot(op, op) + sphere.radius * sphere.radius;

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

Point2f Sphere::GetSphereUV(const Point3f& surface_pos, const Point3f& center) {
	Vector3f dir = glm::normalize(surface_pos - center);

	Point2f uv;
	float phi = atan2(dir.z, dir.x);// PI ~ -PI => 1 ~ 0 => 0 ~ 1
	float theta = asin(dir.y);// -PI / 2 ~ PI / 2 => 0 ~ 1
	uv.x = 1.0f - (phi + PI) * INV_2PI;
	uv.y = (theta + PI / 2.0f) * INV_PI;

	return uv;
}

int Quad::ConstructEmbreeObject(RTCDevice& rtc_device, RTCScene& rtc_scene) {
	Point3f p[4] = { position, position + u, position + u + v, position + v };

	// Initializing Embree geometry
	RTCGeometry mesh = rtcNewGeometry(rtc_device, RTC_GEOMETRY_TYPE_QUAD);

	// Setting and filling the vertex buffer
	Point3f* embree_vertices = (Point3f*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
		sizeof(Point3f), 4);
	for (int i = 0; i < 4; ++i) {
		embree_vertices[i].x = p[i].x;
		embree_vertices[i].y = p[i].y;
		embree_vertices[i].z = p[i].z;
	}

	// Setting and filling the index buffer
	Point4u* quads = (Point4u*)rtcSetNewGeometryBuffer(mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT4,
		sizeof(Point4u), 1);
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

Point2f Quad::GetQuadUV(const Point3f& p, const Point3f& position, const Vector3f& u, const Vector3f& v) {
	Point2f uv;
	Vector3f op = p - position;
	float a1 = glm::dot(glm::normalize(u), op);
	float a2 = glm::dot(glm::normalize(v), op);
	uv.y = a1 / glm::length(u);
	uv.x = a2 / glm::length(v);

	return uv;
}