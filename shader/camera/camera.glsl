#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "util/util.glsl"

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
 * @brief Result structure for camera evaluation
 */
struct CameraEvalInfo {
    float distance;       ///< Distance to the lens point
    float pdf;            ///< PDF with respect to solid angle at the shading point
    float we;             ///< Camera importance
    vec2 raster_ndc;      ///< Raster NDC coordinates [0,1]^2
    ivec2 raster;         ///< Pixel coordinates
    bool valid;           ///< True if the direction hits the camera lens
};

/**
 * @brief Result structure for camera sampling
 */
struct CameraSampleInfo {
    vec3 world_out;
    float distance;
    float we;
    float pdf;
    vec2 raster_ndc;
    ivec2 raster;
};

/**
 * @brief Result structure for camera ray generation
 */
struct CameraRayInfo {
    Ray ray;
    float we;
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
 * @brief Evaluates the camera sampling PDF and importance for a given direction
 * @param camera Camera structure
 * @param position Shading point position in world space
 * @param world_out Direction from shading point toward camera (normalized)
 * @return CameraEvalInfo containing PDF, importance, and pixel coordinates (if valid)
 */
CameraEvalInfo CameraEvaluate(Camera camera, vec3 position, vec3 world_out) {
    CameraEvalInfo result;
    result.distance = 0.0f;
    result.pdf = 0.0f;
    result.we = 0.0f;
    result.raster_ndc = vec2(0.0f);
    result.raster = ivec2(0);
    result.valid = false;

    vec3 d = normalize(world_out);
    // Direction from camera to shading point
    vec3 camera_dir = -d;

    // Transform to camera local space
    vec3 camera_space_dir = ToLocal(camera_dir, camera.right, camera.up, camera.forward);
    if (camera_space_dir.z >= 0.0f) {
        return result;  // Behind the camera
    }

    float cos_theta = -camera_space_dir.z;

    // Project onto the image plane
    float scale = -camera.distance / camera_space_dir.z;
    vec2 plane = camera_space_dir.xy * scale;
    plane /= vec2(camera.width, camera.height);
    if (plane.x > 1.0f || plane.x < -1.0f || plane.y > 1.0f || plane.y < -1.0f) {
        return result;  // Outside the image plane
    }

    // Compute intersection with the lens plane (plane through camera.position, normal = -camera.forward)
    float denom = dot(d, -camera.forward);
    if (abs(denom) < Epsilon) {
        return result;
    }
    float t = dot(camera.position - position, -camera.forward) / denom;
    if (t <= 0.0f) {
        return result;
    }
    vec3 lens_point = position + d * t;
    vec3 lens_offset = lens_point - camera.position;
    if (dot(lens_offset, lens_offset) > camera.aperture_radius * camera.aperture_radius) {
        return result;  // Ray misses the lens disk
    }

    // Fill results
    result.distance = t;

    result.raster_ndc = plane * 0.5f + vec2(0.5f);
    int pixel_x = int(floor(result.raster_ndc.x * (camera.resolution.x - 1.0f) + 0.5f));
    int pixel_y = int(floor(result.raster_ndc.y * (camera.resolution.y - 1.0f) + 0.5f));
    result.raster = ivec2(pixel_x, pixel_y);

    // PDF: p_ω = r² / (cosθ * lens_area), same as CameraSample
    float r2 = dot(lens_point - position, lens_point - position);
    result.pdf = r2 / (cos_theta * camera.lens_area);

    // Importance: CameraWe expects the direction from camera to shading point
    result.we = CameraWe(camera, camera_dir);

    result.valid = true;
    
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
    result.we = 0.0f;

    float r = sqrt(sample_xy.x);
    float theta = 2.0f * PI * sample_xy.y;
    vec2 disk_sample = vec2(r * cos(theta), r * sin(theta)) * camera.aperture_radius;

    vec3 lens_point = camera.position + disk_sample.x * camera.right + disk_sample.y * camera.up;
    vec3 dir = lens_point - position;
    vec3 normalized_dir = normalize(dir);

    result.world_out = normalized_dir;
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
    result.we = CameraWe(camera, negative_dir);

    return result;
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
    result.ray.transport_mode = TransportMode_Radiance;
    
    // Compute PDF and we for the generated ray
    result.we = CameraWe(camera, result.ray.direction);
    result.pdf = CameraPDF(camera, result.ray.direction);
    
    return result;
}

#endif // CAMERA_GLSL