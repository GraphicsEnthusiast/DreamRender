#include <Light.h>
#include <Texture.h>

LightSample SphereLight::Sample(const IntersectionInfo& info, vec2 sample) {
	float x1 = sample.x;
	float x2 = sample.y;

	//球面均匀采样
	float costheta = 1.0f - 2.0f * x1;
	float sintheta = sqrt(std::max(0.0f, 1.0f - costheta * costheta));
	float phi = 2.0f * PI * x2;

	vec3 lightL(sintheta * cos(phi), sintheta * sin(phi), costheta);
	vec3 light_pos = sphere->center + lightL * sphere->radius;
	float dist = length(light_pos - info.position);
	float area = 4.0f * PI * sqr(sphere->radius);

	vec3 L = normalize(light_pos - info.position);
	vec3 light_normal = normalize(light_pos - sphere->center);
	float light_pdf = sqr(dist) / (area * abs(dot(light_normal, L)));

	IntersectionInfo light_info;
	light_info.uv = sphere->GetSphereUV(light_pos, sphere->center);
	light_info.position = light_pos;
	vec3 radiance = sphere->material->Emitted(light_info);

	return { L, light_pdf, dist, light_normal, radiance };
}

LightSample QuadLight::Sample(const IntersectionInfo& info, vec2 sample) {
	float r1 = sample.x;
	float r2 = sample.y;

	float area = length(quad->u) * length(quad->v);
	vec3 light_pos = quad->position + quad->u * r1 + quad->v * r2;
	vec3 L = normalize(light_pos - info.position);
	float dist = length(light_pos - info.position);
	float distSq = sqr(dist);
	vec3 light_normal = normalize(cross(quad->u, quad->v));

	float light_pdf = distSq / (area * abs(dot(light_normal, L)));

	IntersectionInfo light_info;
	light_info.uv = quad->GetQuadUV(light_pos, quad);
	light_info.position = light_pos;
	vec3 radiance = quad->material->Emitted(light_info);

	return { L, light_pdf, dist, light_normal, radiance };
}

Piecewise1D::Piecewise1D(const vector<float>& distrib) {
	queue<Element> greater, lesser;

	sumDistrib = 0.0f;
	for (auto i : distrib) {
		sumDistrib += i;
	}

	for (int i = 0; i < distrib.size(); i++) {
		float scaledPdf = distrib[i] * distrib.size();
		(scaledPdf >= sumDistrib ? greater : lesser).push(Element(i, scaledPdf));
	}

	table.resize(distrib.size(), Element(-1, 0.0f));

	while (!greater.empty() && !lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		auto [g, pg] = greater.front();
		greater.pop();

		table[l] = Element(g, pl);

		pg += pl - sumDistrib;
		(pg < sumDistrib ? lesser : greater).push(Element(g, pg));
	}

	while (!greater.empty()) {
		auto [g, pg] = greater.front();
		greater.pop();
		table[g] = Element(g, pg);
	}

	while (!lesser.empty()) {
		auto [l, pl] = lesser.front();
		lesser.pop();
		table[l] = Element(l, pl);
	}
}

int Piecewise1D::Sample(const vec2& u) {
	int rx = u.x * table.size();
	float ry = u.y;

	return (ry <= table[rx].second / sumDistrib) ? rx : table[rx].first;
}

PiecewiseIndependent2D::PiecewiseIndependent2D(float* pdf, int width, int height) {
	vector<float> colDistrib(height);
	for (int i = 0; i < height; i++) {
		vector<float> table(pdf + i * width, pdf + (i + 1) * width);
		Piecewise1D rowDistrib(table);
		rowTables.push_back(rowDistrib);
		colDistrib[i] = rowDistrib.Sum();
	}
	colTable = Piecewise1D(colDistrib);
}

pair<int, int> PiecewiseIndependent2D::Sample(const vec2& u1, const vec2& u2) {
	int row = colTable.Sample(u1);

	return pair<int, int>(rowTables[row].Sample(u2), row);
}

InfiniteAreaLight::InfiniteAreaLight(shared_ptr<HdrTexture> h, float scl) {
	scale = scl;
	hdr = h;

	int mWidth = hdr->nx;
	int mHeight = hdr->ny;
	int mBits = hdr->nn;
	float* data = hdr->data;
	
	float* pdf = new float[mWidth * mHeight];
	float sum = 0.0f;
	for (int j = 0; j < mHeight; j++) {
		for (int i = 0; i < mWidth; i++) {
			vec3 l(data[mBits * (j * mWidth + i)], data[mBits * (j * mWidth + i) + 1], data[mBits * (j * mWidth + i) + 2]);
			pdf[j * mWidth + i] = Luminance(l) * sin((float)(j + 0.5f) / mHeight * PI);
			sum += pdf[j * mWidth + i];
		}
	}
	mDistrib = PiecewiseIndependent2D(pdf, mWidth, mHeight);
	delete[] pdf;
}

vec3 InfiniteAreaLight::Emitted(const vec3& dir) {
	vec2 planeUV = SphereToPlane(normalize(dir));

	return scale * hdr->Value(planeUV);
}

HDRSample InfiniteAreaLight::Sample(const IntersectionInfo& info, vec4 sample) {
	vec2 u1(sample.x, sample.y);
	vec2 u2(sample.z, sample.w);
	auto [col, row] = mDistrib.Sample(u1, u2);

	int mWidth = hdr->nx;
	int mHeight = hdr->ny;

	float sinTheta = sin(PI * (row + 0.5f) / mHeight);
	auto L = PlaneToSphere(vec2((col + 0.5f) / mWidth, (row + 0.5f) / mHeight));

	float pdf = Pdf(L, info.normal);

	return { L, Emitted(L), pdf };
}

float InfiniteAreaLight::Pdf(const vec3& L, const vec3& N) {
	int mWidth = hdr->nx;
	int mHeight = hdr->ny;

	float sinTheta = sqrt(1.0f - sqr(abs(dot(L, N))));

	float pdf = GetPortion(L) * float(mWidth * mHeight) * 0.5f * INV_PI * INV_PI * 1.0f / sinTheta;

	return pdf;
}

float InfiniteAreaLight::GetPortion(const vec3& L) {
	vec2 planeUV = SphereToPlane(normalize(L));
	vec3 l = hdr->Value(planeUV);

	return Luminance(l) / mDistrib.Sum();
}