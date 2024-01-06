#include "Light.h"

Light::~Light() {
	if (shape != NULL) {
		delete shape;
		shape = NULL;
	}
}

RGBSpectrum Light::EvaluateEnvironment(const Vector3f& L, float& pdf) {
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum Light::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

RGBSpectrum QuadArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	float cos_theta = glm::dot(L, Nl);
	if (!twoSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);

	// surface pdf
	pdf = 1.0f / area;

	// solid angel pdf
	float distance = info.t;
	pdf *= glm::pow2(distance) / std::abs(cos_theta);

	return shape->GetMaterial()->Emit();// info record a point on a light source
}

RGBSpectrum QuadArea::Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Quad* quad = (Quad*)shape;
	Vector3f Nl = glm::cross(quad->u, quad->v);
	Point3f pos = quad->position + quad->u * sampler->Get1() + quad->v * sampler->Get1();
	L = pos - info.position;
	float dist_sq = glm::dot(L, L);
	float distance = std::sqrt(dist_sq);
	dist = distance;
	L /= distance;

	float cos_theta = glm::dot(L, Nl);

	if (!twoSide && cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = glm::length(quad->u) * glm::length(quad->v);
	pdf = 1.0f / area;
	pdf *= dist_sq / std::abs(cos_theta);

	return shape->GetMaterial()->Emit();// info record a point on a common shape
}

RGBSpectrum SphereArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	Sphere* sphere = (Sphere*)shape;
	Vector3f dir = sphere->center - info.position;
	float dist_sq = glm::dot(dir, dir);
	float sin_theta_sq = sphere->radius * sphere->radius / dist_sq;
	float cos_theta = std::sqrt(1.0f - sin_theta_sq);
	pdf = UniformPdfCone(cos_theta);

	return shape->GetMaterial()->Emit();
}

RGBSpectrum SphereArea::Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	Sphere* sphere = (Sphere*)shape;
	Vector3f dir = sphere->center - info.position;
	float dist_sq = glm::dot(dir, dir);
	float inv_dist = 1.0f / std::sqrt(dist_sq);
	dir *= inv_dist;
	float distance = dist_sq * inv_dist;

	float sin_theta = sphere->radius * inv_dist;
	if (sin_theta < 1.0f) {
		float cos_theta = std::sqrt(1.0f - sin_theta * sin_theta);
		Vector3f local_L = UniformSampleCone(sampler->Get2(), cos_theta);
		float cos_i = local_L.z;
		L = ToWorld(local_L, dir);
		pdf = UniformPdfCone(cos_theta);
		dist = cos_i * distance - std::sqrt(std::max(0.0f, sphere->radius * sphere->radius - (1.0f - cos_i * cos_i) * dist_sq));
		//dist = distance - sphere->radius;

		return shape->GetMaterial()->Emit();
	}

	pdf = 0.0f;

	return RGBSpectrum(0.0f);
}

InfiniteArea::InfiniteArea(std::shared_ptr<Hdr> h, float sca) : Light(LightType::InfiniteAreaLight, NULL), hdr(h), scale(sca) {
	int mWidth = hdr->nx;
	int mHeight = hdr->ny;
	int mBits = hdr->nn;
	float* data = hdr->data;

	std::vector<float> pdf(mWidth * mHeight);
	for (int j = 0; j < mHeight; j++) {
		for (int i = 0; i < mWidth; i++) {
			float a[3] = { data[mBits * (j * mWidth + i)], data[mBits * (j * mWidth + i) + 1], data[mBits * (j * mWidth + i) + 2] };
			RGBSpectrum l = RGBSpectrum::FromRGB(a);
			pdf[j * mWidth + i] = Luminance(l) * sin(((float)j + 0.5f) / (float)mHeight * PI);
		}
	}

	table = AliasTable2D(pdf, mWidth, mHeight);
}

RGBSpectrum InfiniteArea::EvaluateEnvironment(const Vector3f& L, float& pdf) {
	int mWidth = hdr->nx;
	int mHeight = hdr->ny;

	Point2f planeUV = SphereToPlane(L);

	RGBSpectrum radiance = hdr->GetColor(planeUV);

	pdf = Luminance(radiance) / table.Sum() * float(mWidth * mHeight) * 0.5f * INV_PI * INV_PI;

	return radiance * scale;
}

RGBSpectrum InfiniteArea::Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	dist = Infinity;

	auto [col, row] = table.Sample(sampler->Get2(), sampler->Get2());

	int mWidth = hdr->nx;
	int mHeight = hdr->ny;

	Point2f planeUV((col + 0.5f) / mWidth, (row + 0.5f) / mHeight);
	L = PlaneToSphere(planeUV);

	RGBSpectrum radiance = hdr->GetColor(planeUV);

	pdf = Luminance(radiance) / table.Sum() * float(mWidth * mHeight) * 0.5f * INV_PI * INV_PI;

	return radiance * scale;
}

TriangleMeshArea::TriangleMeshArea(Shape* s) : Light(LightType::TriangleMeshAreaLight, s) {
	TriangleMesh* mesh = (TriangleMesh*)shape;
	areas.resize(mesh->Faces());
	for (int i = 0; i < mesh->Faces(); i++) {
		Point3u index = mesh->GetIndices(i);
		Point3f v0 = mesh->GetVertex(index[0]);
		Point3f v1 = mesh->GetVertex(index[1]);
		Point3f v2 = mesh->GetVertex(index[2]);
		Vector3f e1 = v1 - v0;
		Vector3f e2 = v2 - v0;
		areas[i] = glm::length(glm::cross(e1, e2)) / 2.0f;
	}

	table = AliasTable1D(areas);
}

RGBSpectrum TriangleMeshArea::Evaluate(const Vector3f& L, float& pdf, const IntersectionInfo& info) {
	TriangleMesh* mesh = (TriangleMesh*)shape;
	int id = info.primID;
	float dist = info.t;
	float cos_theta = glm::dot(L, info.Ng);

	if (cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = areas[id];
	pdf = 1.0f / area;
	pdf *= dist * dist / std::abs(cos_theta);
	pdf *= area / table.Sum();

	return shape->GetMaterial()->Emit();
}

RGBSpectrum TriangleMeshArea::Sample(Vector3f& L, float& pdf, float& dist, const IntersectionInfo& info, std::shared_ptr<Sampler> sampler) {
	TriangleMesh* mesh = (TriangleMesh*)shape;
	int id = table.Sample(sampler->Get2());
	Point3u index = mesh->GetIndices(id);
	Point3f v0 = mesh->GetVertex(index[0]);
	Point3f v1 = mesh->GetVertex(index[1]);
	Point3f v2 = mesh->GetVertex(index[2]);
	Vector3f e1 = v1 - v0;
	Vector3f e2 = v2 - v0;

	float a = std::sqrt(sampler->Get1());
	float b1 = 1.0f - a;
	float b2 = a * sampler->Get1();

	Point3f p = v0 + (e1 * b1) + (e2 * b2);
	L = p - info.position;
	dist = glm::length(L);
	L /= dist;

	Vector3f Nl = mesh->GetGeometryNormal(id);
	float cos_theta = glm::dot(L, Nl);

	if (cos_theta > 0.0f) {
		pdf = 0.0f;

		return RGBSpectrum(0.0f);
	}

	float area = areas[id];
	pdf = 1.0f / area;
	pdf *= dist * dist / std::abs(cos_theta);
	pdf *= area / table.Sum();

	return shape->GetMaterial()->Emit();
}
