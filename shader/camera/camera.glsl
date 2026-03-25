#ifndef _CAMERA__GLSL__
#define _CAMERA__GLSL__

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
vec3 OffsetRay(vec3 p, vec3 n) {
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
    vec3 offset_origin = OffsetRay(position, offset_normal);
    
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
    bool filmic;          ///< True for filmic tonemap, false for gamma correction
    int medium;           ///< Medium index the camera is in
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
    Ray ray;              ///< Ray from position to camera
    float we;             ///< Sampling we
    float pdf;            ///< Probability density function(lens area to solid angle)
    ivec2 raster;         ///< Raster index
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
 * @param filmic Tonemap type flag
 * @param med Medium index
 * @return Initialized Camera structure looking at target
 */
Camera CreateCamera(vec3 pos, vec3 target, vec3 world_up, vec2 res, float dist, float angle, float radius, float focal, bool filmic, int med) {
    Camera cam;
    cam.position = pos;
    cam.resolution = res;
    cam.distance = dist;
    cam.fov = angle;
    cam.aperture_radius = max(0.01f, radius);
    cam.focal_distance = focal;
    cam.filmic = filmic;
    cam.medium = med;
    
    // Compute camera basis vectors using LookAt method
    cam.forward = normalize(pos - target);
    cam.right = normalize(cross(world_up, cam.forward));
    cam.up = normalize(cross(cam.forward, cam.right));
    
    // Compute camera geometry properties
    float half_fov = cam.fov * 0.5f;
    cam.height = tan(radians(half_fov)) * cam.distance;
    cam.width = cam.height * cam.resolution.x / cam.resolution.y;
    cam.sensor_area = 4.0f * cam.width * cam.height; // Sensor area
    
    cam.lens_area = PI * cam.aperture_radius * cam.aperture_radius;
    
    cam.pixel_to_screen = vec2(
        2.0f * cam.width / cam.resolution.x,
        2.0f * cam.height / cam.resolution.y
    );
    cam.ratio = cam.focal_distance / cam.distance;
    
    return cam;
}

/**
 * @brief Generates a primary ray from camera
 * @param cam Camera structure
 * @param pixel_x Pixel x-coordinate
 * @param pixel_y Pixel y-coordinate
 * @param sample_xy Random sample for depth of field
 * @return Generated ray
 */
Ray GeneratePrimaryRay(Camera cam, float pixel_x, float pixel_y, vec2 sample_xy) {
    float screen_x = pixel_x * cam.pixel_to_screen.x - cam.width;
    float screen_y = pixel_y * cam.pixel_to_screen.y - cam.height;

    vec3 dir;
    vec3 origin = cam.position;

    vec2 aperture_xy = sample_xy * cam.aperture_radius;
    float focal_x = cam.ratio * screen_x;
    float focal_y = cam.ratio * screen_y;
    vec3 aperture_offset = vec3(aperture_xy, 0.0f);
    vec3 focal_point = vec3(focal_x, focal_y, -cam.focal_distance);

    dir = focal_point - aperture_offset; 
    dir = dir.x * cam.right + dir.y * cam.up + dir.z * cam.forward;
    origin += (aperture_offset.x * cam.right + aperture_offset.y * cam.up);

    Ray ray;
    ray.origin = origin;
    ray.direction = normalize(dir);
    ray.tmin = 0.0f;
    ray.tmax = MaxFloat;

    return ray;
}

/**
 * @brief Calculates the camera sampling we
 * @param cam Camera structure
 * @param cos_theta Cosine of the angle between the ray direction and camera forward axis
 * @return Calculated we value
 */
float CameraWe(Camera cam, float cos_theta) {
    // Calculate we (We): (distance²) / (sensor_area * lens_area * cos⁴θ)
    return cam.distance * cam.distance / (cam.sensor_area * cam.lens_area * pow(cos_theta, 4.0f));
}

/**
 * @brief Samples the camera from a given position
 * @param cam Camera structure
 * @param sample_pos Sampling position
 * @return CameraSampleInfo structure with sampling results
 */
CameraSampleInfo CameraSample(Camera cam, vec3 sample_pos) {
    CameraSampleInfo result;
    result.pdf = 0.0f; // Default to invalid
    result.we = 0.0f;
    
    vec3 dir = cam.position - sample_pos;
    vec3 normalized_dir = normalize(dir);

    result.ray.origin = sample_pos;
    result.ray.direction = normalized_dir;
    result.ray.tmin = Epsilon;
    result.ray.tmax = length(dir) - Epsilon;

    vec3 negative_dir = -normalized_dir;
    
    // Convert to camera space
    vec3 camera_space_dir = ToLocal(negative_dir, cam.right, cam.up, cam.forward);
    if (camera_space_dir.z >= 0.0f) {
        return result;
    }
    
    float cos_theta = dot(camera_space_dir, vec3(0.0f, 0.0f, -1.0f));
    float scale = -cam.distance / camera_space_dir.z;
    camera_space_dir *= scale;
    vec2 plane = vec2(camera_space_dir.x, camera_space_dir.y);
    plane /= vec2(cam.width, cam.height);
    
    if (plane.x > 1.0f || plane.x < -1.0f || plane.y > 1.0f || plane.y < -1.0f) {
        return result;
    }

    plane = plane * 0.5f + vec2(0.5f, 0.5f);
    int pixel_x = int(floor(plane.x * (cam.resolution.x - 1.0f) + 0.5f));
    int pixel_y = int(floor(plane.y * (cam.resolution.y - 1.0f) + 0.5f));
    result.raster = ivec2(pixel_x, pixel_y);
    
    // Calculate PDF: p(ω) = p(A) * (r² / cosθ) = (1 / cam.lens_area) * (r² / cosθ)
    // where r² = dot(dir, dir)
    result.pdf = dot(dir, dir) / (cos_theta * cam.lens_area);
    
    result.we = CameraWe(cam, cos_theta);
    
    return result;
}

/**
 * @brief Computes PDF for camera sampling
 * @param cam Camera structure
 * @param dir Direction from camera position to destination
 * @return PDF values
 */
float CameraPDF(Camera cam, vec3 dir) {
    // For area PDF, we assume uniform sampling over the lens area
    float pdf_area = 1.0f / cam.lens_area;
    
    float cos_theta = dot(dir, -cam.forward);
    
    // Calculate solid angle PDF: p(ω) = (distance²) / (sensor_area * cos³θ)
    float pdf_solid_angle = cam.distance * cam.distance / (cam.sensor_area * pow(cos_theta, 3.0f));

    return pdf_area * pdf_solid_angle;
}

#endif // _CAMERA__GLSL__