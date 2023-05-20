#include <Camera.h>

PinholeCamera::PinholeCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect) {
	//右手坐标系
	origin = lookfrom;

	float theta = radians(vfov);

	//计算近平面的宽和高，宽和高是正数
	float half_height = abs(znear) * tan(theta / 2.0f);
	float half_width = aspect * half_height;
	w = normalize(lookfrom - lookat);//相机的z轴
	u = normalize(cross(vup, w));//相机的x轴
	v = normalize(cross(w, u));//相机的y轴

	//确定相机投影平面在世界空间的左下角位置
	lower_left_corner = origin - half_width * u - half_height * v - abs(znear) * w;

	horizontal = 2.0f * half_width * u;//近平面的宽向量
	vertical = 2.0f * half_height * v;//近平面的高向量
}

RTCRay PinholeCamera::GenerateRay(float x, float y) noexcept {
	RTCRay ray;

	vec3 direction = normalize(lower_left_corner + x * horizontal + y * vertical - origin);

	ray.org_x = origin.x;
	ray.org_y = origin.y;
	ray.org_z = origin.z;

	ray.dir_x = direction.x;
	ray.dir_y = direction.y;
	ray.dir_z = direction.z;

	ray.tnear = EPS;
	ray.tfar = INF;
	ray.time = 0.0f;

	return ray;
}

ThinlensCamera::ThinlensCamera(vec3 lookfrom, vec3 lookat, vec3 vup, float znear, float vfov, float aspect, float aperture, float focus_dist) {
	//右手坐标系
	origin = lookfrom;

	lens_radius = aperture / 2.0f;
	float theta = radians(vfov);

	//计算近平面的宽和高，宽和高是正数
	float half_height = abs(znear) * tan(theta / 2.0f);
	float half_width = aspect * half_height;
	w = normalize(lookfrom - lookat);//相机的z轴
	u = normalize(cross(vup, w));//相机的x轴
	v = normalize(cross(w, u));//相机的y轴

	//确定相机投影平面在世界空间的左下角位置
	lower_left_corner = origin - half_width * u * focus_dist - half_height * v * focus_dist - abs(znear) * w * focus_dist;

	horizontal = 2.0f * half_width * u * focus_dist;//近平面的宽向量
	vertical = 2.0f * half_height * v * focus_dist;//近平面的高向量
}

RTCRay ThinlensCamera::GenerateRay(float x, float y) noexcept {
	RTCRay ray;

	vec3 rd = lens_radius * RandomInUnitDisk();
	vec3 offset = u * rd.x + v * rd.y;

	vec3 direction = normalize(lower_left_corner + x * horizontal + y * vertical - origin - offset);

	ray.org_x = origin.x + offset.x;
	ray.org_y = origin.y + offset.y;
	ray.org_z = origin.z + offset.z;

	ray.dir_x = direction.x;
	ray.dir_y = direction.y;
	ray.dir_z = direction.z;

	ray.tnear = EPS;
	ray.tfar = INF;
	ray.time = 0.0f;

	return ray;
}