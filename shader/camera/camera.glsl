#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "util/util.glsl"

/**
 * @brief Ray structure for ray tracing parameters
 */
struct Ray {
    vec3 origin;    ///< Ray origin point
    vec3 direction; ///< Ray direction vector (normalized)
    float tmin;     ///< Minimum ray distance (avoid self-intersection)
    float tmax;     ///< Maximum ray distance
};

const float OriginScale = 1.0f / 32.0f;
const float FloatScale = 1.0f / 65536.0f;
const float IntScale = 256.0f;

/**
 * @brief Computes the offset ray origin to avoid self-intersection
 * @param p Original intersection point
 * @param n Surface normal (points outward for rays exiting the surface, else is flipped)
 * @return Offset point
 */
vec3 OffsetRayOrigin(vec3 p, vec3 n) {
    // Compute integer offset scaled by int_scale
    ivec3 of_i = ivec3(int(IntScale * n.x), 
                       int(IntScale * n.y), 
                       int(IntScale * n.z));
    
    // Convert float to int, apply integer offset, then convert back to float
    vec3 p_i = vec3(
        intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0.0) ? -of_i.x : of_i.x)),
        intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0.0) ? -of_i.y : of_i.y)),
        intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0.0) ? -of_i.z : of_i.z))
    );
    
    // Apply appropriate offset based on coordinate magnitude
    return vec3(
        abs(p.x) < OriginScale ? p.x + FloatScale * n.x : p_i.x,
        abs(p.y) < OriginScale ? p.y + FloatScale * n.y : p_i.y,
        abs(p.z) < OriginScale ? p.z + FloatScale * n.z : p_i.z
    );
}

/**
 * @brief Generates a new ray from an intersection point with origin offset to avoid self-intersection
 * @param position Intersection point in world space
 * @param direction Ray direction vector (should be normalized)
 * @param normal Surface normal at the intersection point (should be normalized)
 * @param tmin Minimum ray distance to prevent self-intersection
 * @param tmax Maximum ray distance for intersection testing
 * @return New ray with properly offset origin to prevent numerical precision issues
 */
Ray SpawnRay(vec3 position, vec3 direction, vec3 normal, float tmin, float tmax) {
    vec3 offset_normal = (dot(normal, direction) >= 0.0f) ? normal : -normal;
    vec3 offset_origin = OffsetRayOrigin(position, offset_normal);
    
    Ray ray;
    ray.origin = offset_origin;
    ray.direction = direction;
    ray.tmin = tmin;
    ray.tmax = tmax;
    
    return ray;
}

/**
 * @brief Generates a new ray from an intersection point with origin offset to avoid self-intersection
 * @param position Intersection point in world space
 * @param direction Ray direction vector (should be normalized)
 * @param distance Maximum distance to the target point (typically to light source)
 * @return New ray with properly offset origin to prevent numerical precision issues
 */
Ray SpawnShadowRay(vec3 position, vec3 direction, float distance) {
    Ray ray;
    ray.origin = position;
    ray.direction = direction;
    ray.tmin = Epsilon;
    ray.tmax = distance - Epsilon;
    
    return ray;
}

/**
 * @brief Camera structure for ray generation and sampling
 */
struct Camera {
    vec3 position;        ///< Camera position in world space
    vec3 right;           ///< Camera right vector
    vec3 up;              ///< Camera up vector
    vec3 forward;         ///< Camera forward vector
    vec2 resolution;      ///< Image resolution (width, height)
    float distance;       ///< Distance to image plane
    float fov;            ///< Field of view in degrees
    float aperture_radius;///< Lens aperture radius
    float focal_distance; ///< Focal distance for depth of field
    float width;          ///< Image plane width
    float height;         ///< Image plane height
    vec2 pixel_to_screen; ///< Pixel to screen space conversion factor
    float ratio;          ///< Focal distance ratio
    float sensor_area;    ///< Image plane area (sensor area)
    float lens_area;      ///< Lens area (calculated from aperture_radius)
};

/**
 * @brief Result structure for camera sampling
 */
struct CameraSampleInfo {
    vec3 world_l;
    float distance;
    float we_cosine;
    float pdf;
    vec2 raster_ndc;
    ivec2 raster;
};

/**
 * @brief Result structure for camera ray generation
 */
struct CameraRayInfo {
    Ray ray;
    float we_cosine;
    float pdf;
};

/**
 * @brief Creates and configures a camera to look at a specific point
 * @param pos Camera position
 * @param target Target position to look at
 * @param world_up World up vector
 * @param res Image resolution
 * @param dist Distance to image plane
 * @param angle Field of view in degrees
 * @param radius Aperture radius
 * @param focal Focal distance
 * @return Initialized Camera structure looking at target
 */
Camera CreateCamera(vec3 pos, vec3 target, vec3 world_up, vec2 res, float dist, float angle, float radius, float focal) {
    Camera camera;
    camera.position = pos;
    camera.resolution = res;
    camera.distance = dist;
    camera.fov = angle;
    camera.aperture_radius = max(0.01f, radius);
    camera.focal_distance = focal;
    
    // Compute camera basis vectors using LookAt method
    camera.forward = normalize(pos - target);
    camera.right = normalize(cross(world_up, camera.forward));
    camera.up = normalize(cross(camera.forward, camera.right));
    
    // Compute camera geometry properties
    float half_fov = camera.fov * 0.5f;
    camera.height = tan(radians(half_fov)) * camera.distance;
    camera.width = camera.height * camera.resolution.x / camera.resolution.y;
    camera.sensor_area = 4.0f * camera.width * camera.height; // Sensor area
    
    camera.lens_area = PI * camera.aperture_radius * camera.aperture_radius;
    
    camera.pixel_to_screen = vec2(
        2.0f * camera.width / camera.resolution.x,
        2.0f * camera.height / camera.resolution.y
    );
    camera.ratio = camera.focal_distance / camera.distance;
    
    return camera;
}

/**
 * @brief Calculates the camera sampling we
 * @param camera Camera structure
 * @param dir Direction from camera position to destination
 * @return Calculated we value
 */
float CameraWe(Camera camera, vec3 dir) {
    float cos_theta = dot(dir, -camera.forward);

    // Calculate we (We): (distance²) / (sensor_area * lens_area * cos⁴θ)
    return camera.distance * camera.distance / (camera.sensor_area * camera.lens_area * pow(cos_theta, 4.0f));
}

/**
 * @brief Computes PDF for camera sampling
 * @param camera Camera structure
 * @param dir Direction from camera position to destination
 * @return PDF value (solid angle measure)
 */
float CameraPDF(Camera camera, vec3 dir) {
    // For area PDF, we assume uniform sampling over the lens area
    float pdf_area = 1.0f / camera.lens_area;
    
    float cos_theta = dot(dir, -camera.forward);
    
    // Calculate solid angle PDF: p(ω) = (distance²) / (sensor_area * cos³θ)
    float pdf_solid_angle = camera.distance * camera.distance / (camera.sensor_area * pow(cos_theta, 3.0f));

    return pdf_area * pdf_solid_angle;
}

/**
 * @brief Generates a primary ray from camera with PDF and we
 * @param camera Camera structure
 * @param pixel_x Pixel x-coordinate
 * @param pixel_y Pixel y-coordinate
 * @param sample_xy Random sample for depth of field
 * @return CameraRayInfo containing ray, PDF, and we
 */
CameraRayInfo GenerateCameraRay(Camera camera, float pixel_x, float pixel_y, vec2 sample_xy) {
    CameraRayInfo result;
    
    float screen_x = pixel_x * camera.pixel_to_screen.x - camera.width;
    float screen_y = pixel_y * camera.pixel_to_screen.y - camera.height;

    vec3 dir;
    vec3 origin = camera.position;

    vec2 aperture_xy = sample_xy * camera.aperture_radius;
    float focal_x = camera.ratio * screen_x;
    float focal_y = camera.ratio * screen_y;
    vec3 aperture_offset = vec3(aperture_xy, 0.0f);
    vec3 focal_point = vec3(focal_x, focal_y, -camera.focal_distance);

    dir = focal_point - aperture_offset; 
    dir = dir.x * camera.right + dir.y * camera.up + dir.z * camera.forward;
    origin += (aperture_offset.x * camera.right + aperture_offset.y * camera.up);

    result.ray.origin = origin;
    result.ray.direction = normalize(dir);
    result.ray.tmin = 0.0f;
    result.ray.tmax = MaxFloat;
    
    // Compute PDF and we for the generated ray
    float cos_theta = dot(result.ray.direction, -camera.forward);
    result.we_cosine = CameraWe(camera, result.ray.direction) * cos_theta;
    result.pdf = CameraPDF(camera, result.ray.direction);
    
    return result;
}

/**
 * @brief Samples the camera from a given position
 * @param camera Camera structure
 * @param position Shading point position
 * @param sample_xy 2D random sample in [0,1] range (typically from low-discrepancy sequence)
 * @return CameraSampleInfo structure with sampling results
 */
CameraSampleInfo CameraSample(Camera camera, vec3 position, vec2 sample_xy) {
    CameraSampleInfo result;
    result.pdf = 0.0f; // Default to invalid
    result.we_cosine = 0.0f;

    float r = sqrt(sample_xy.x);
    float theta = 2.0f * PI * sample_xy.y;
    vec2 disk_sample = vec2(r * cos(theta), r * sin(theta)) * camera.aperture_radius;

    vec3 lens_point = camera.position + disk_sample.x * camera.right + disk_sample.y * camera.up;
    vec3 dir = lens_point - position;
    vec3 normalized_dir = normalize(dir);

    result.world_l = normalized_dir;
    result.distance = length(dir);

    vec3 negative_dir = -normalized_dir;
    
    // Convert to camera space
    vec3 camera_space_dir = ToLocal(negative_dir, camera.right, camera.up, camera.forward);
    if (camera_space_dir.z >= 0.0f) {
        return result;
    }
    
    float cos_theta = dot(camera_space_dir, vec3(0.0f, 0.0f, -1.0f));
    float scale = -camera.distance / camera_space_dir.z;
    camera_space_dir *= scale;
    vec2 plane = vec2(camera_space_dir.x, camera_space_dir.y);
    plane /= vec2(camera.width, camera.height);
    
    if (plane.x > 1.0f || plane.x < -1.0f || plane.y > 1.0f || plane.y < -1.0f) {
        return result;
    }

    plane = plane * 0.5f + vec2(0.5f);
    result.raster_ndc = plane;
    int pixel_x = int(floor(plane.x * (camera.resolution.x - 1.0f) + 0.5f));
    int pixel_y = int(floor(plane.y * (camera.resolution.y - 1.0f) + 0.5f));
    result.raster = ivec2(pixel_x, pixel_y);
    
    // Calculate PDF: p(ω) = p(A) * (r² / cosθ) = (1 / camera.lens_area) * (r² / cosθ)
    // where r² = dot(dir, dir)
    result.pdf = dot(dir, dir) / (cos_theta * camera.lens_area);
    result.we_cosine = CameraWe(camera, negative_dir) * cos_theta;

    return result;
}

#endif // CAMERA_GLSL