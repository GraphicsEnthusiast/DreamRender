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

auto PinholeCamera::GenerateRay(float x, float y) const noexcept -> RTCRay {
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