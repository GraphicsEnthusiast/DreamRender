#ifndef _CAMERA__GLSL__
#define _CAMERA__GLSL__

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
    bool filmic;          ///< True for filmic tonemap, false for gamma correction
    bool environment;     ///< True if environment camera
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
struct SampleCameraResult {
    Ray ray;              ///< Ray from position to camera
    float weight;         ///< Sampling weight
    float pdf;            ///< Probability density function
    int raster;           ///< Raster index
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
    cam.aperture_radius = radius;
    cam.focal_distance = focal;
    cam.filmic = filmic;
    cam.environment = false; // Default to perspective camera
    cam.medium = med;
    
    // Compute camera basis vectors using LookAt method
    cam.forward = normalize(pos - target);
    cam.right = normalize(cross(world_up, cam.forward));
    cam.up = normalize(cross(cam.forward, cam.right));
    
    // Compute camera geometry properties
    float half_fov = cam.fov * 0.5f;
    cam.height = tan(DegreesToRadians(half_fov)) * cam.distance;
    cam.width = cam.height * cam.resolution.x / cam.resolution.y;
    cam.sensor_area = 4.0f * cam.width * cam.height; // Sensor area
    
    // Calculate lens area based on aperture radius
    // For pinhole camera (aperture_radius == 0), lens area is conceptually 1 in the PDF calculation
    if (cam.aperture_radius > 0.0f) {
        cam.lens_area = PI * cam.aperture_radius * cam.aperture_radius;
    } 
    else {
        cam.lens_area = 1.0f;
    }
    
    cam.pixel_to_screen = vec2(
        2.0f * cam.width / cam.resolution.x,
        2.f * cam.height / cam.resolution.y
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
    if (cam.environment) {
        vec3 origin = cam.position;
        float theta = PI * (1.0f - pixel_y / cam.resolution.y);
        float phi = 2.0f * PI * (1.0f - pixel_x / cam.resolution.x);
        vec3 dir = vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
        dir = dir.x * cam.right + dir.y * cam.up - dir.z * cam.forward;

        Ray ray;
        ray.origin = origin;
        ray.direction = normalize(dir);
        ray.tmin = 0.0f;
        ray.tmax = MaxFloat;

        return ray;
    }

    float screen_x = pixel_x * cam.pixel_to_screen.x - cam.width;
    float screen_y = pixel_y * cam.pixel_to_screen.y - cam.height;

    vec3 dir, origin = cam.position;

    if (cam.aperture_radius > 0.0f) {
        vec2 aperture_xy = sample_xy * cam.aperture_radius;
        float focal_x = cam.ratio * screen_x;
        float focal_y = cam.ratio * screen_y;
        vec3 aperture_offset = vec3(aperture_xy, 0.0f);
        vec3 focal_point = vec3(focal_x, focal_y, -cam.focal_distance);

        dir = focal_point - aperture_offset; 
        dir = dir.x * cam.right + dir.y * cam.up + dir.z * cam.forward;
        origin += (aperture_offset.x * cam.right + aperture_offset.y * cam.up);
    } 
    else {
        dir = screen_x * cam.right + screen_y * cam.up - cam.distance * cam.forward;
    }

    Ray ray;
    ray.origin = origin;
    ray.direction = normalize(dir);
    ray.tmin = 0.0f;
    ray.tmax = MaxFloat;

    return ray;
}

/**
 * @brief Samples the camera from a given position
 * @param cam Camera structure
 * @param sample_pos Sampling position
 * @param epsilon Ray epsilon value
 * @return SampleCameraResult structure with sampling results
 */
SampleCameraResult SampleCamera(Camera cam, vec3 sample_pos, float epsilon) {
    SampleCameraResult result;
    result.pdf = 0.0f; // Default to invalid
    
    vec3 dir = cam.position - sample_pos;
    vec3 normalized_dir = normalize(dir);
    result.ray = Ray(sample_pos, normalized_dir, epsilon, length(dir) - epsilon);
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
    result.raster = pixel_y * int(cam.resolution.x) + pixel_x;
    
    // Calculate PDF: p(ω) = p(A) * (r² / cosθ) = (1 / (cam.area * cam.lens_area)) * (r² / cosθ)
    // where r² = dot(dir, dir)
    result.pdf = dot(dir, dir) / (cos_theta * cam.sensor_area * cam.lens_area);
    
    // Calculate weight (We): (distance²) / (sensor_area * lens_area * cos⁴θ)
    result.weight = cam.distance * cam.distance / (cam.sensor_area * cam.lens_area * pow(cos_theta, 4.0f));
    
    return result;
}

/**
 * @brief Computes PDF for camera sampling
 * @param cam Camera structure
 * @param dir Direction from camera position to destination
 * @return PDF values
 */
float PDFCamera(Camera cam, vec3 dir) {
    // For area PDF, we assume uniform sampling over the lens area
    float pdf_area = 1.0f / cam.lens_area;
    
    float cos_theta = dot(dir, -cam.forward);
    
    // Calculate solid angle PDF: p(ω) = (distance²) / (sensor_area * cos³θ)
    float pdf_solid_angle = cam.distance * cam.distance / (cam.sensor_area * pow(cos_theta, 3.0f));

    return pdf_area * pdf_solid_angle;
}

#endif // _CAMERA__GLSL__