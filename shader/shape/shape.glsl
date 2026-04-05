#ifndef SHAPE_GLSL
#define SHAPE_GLSL

#include "camera/camera.glsl"

uniform samplerBuffer Triangles;
uniform samplerBuffer BVHNodes;

uniform samplerBuffer TrianglesLight;
uniform samplerBuffer BVHNodesLight;

/**
 * @brief Hit information structure for ray intersection results
 */
struct Hit {
    float distance; ///< Intersection distance along the rays
    float u;        ///< Barycentric u coordinate
    float v;        ///< Barycentric v coordinate
    int tri_index;  ///< Intersected triangle index
    bool is_light;  ///< Whether the hit is on a light triangle
};

/**
 * @brief Triangle data structure (must match CPU-side TriangleEncoded)
 */
struct Triangle {
    vec3 p1, p2, p3; ///< Vertex positions
    vec3 n1, n2, n3; ///< Vertex normals
    vec2 t1, t2, t3; ///< Vertex texcoords

    int material_type;
    vec4 emission;
    vec4 diffuse;    ///< Diffuse color (rgb) and texture flag (a: 0=const, 1=texture)
    vec4 roughness;  ///< Roughness (x) and texture flag (a: 0=const, 1=texture)
    
    int in_phase_type;      ///< Inside medium phase function type
    float in_g;             ///< Inside medium asymmetry parameter
    int in_medium_type;     ///< Inside medium type
    bool has_in_medium;
    vec3 in_sigma_s;        ///< Inside medium scattering coefficient
    vec3 in_sigma_t;        ///< Inside medium extinction coefficient
    
    int out_phase_type;     ///< Outside medium phase function type
    float out_g;            ///< Outside medium asymmetry parameter
    int out_medium_type;    ///< Outside medium type
    bool has_out_medium;
    vec3 out_sigma_s;       ///< Outside medium scattering coefficient
    vec3 out_sigma_t;       ///< Outside medium extinction coefficient
};

/**
 * @brief BVH Node structure for GPU traversal (must match CPU-side BVHNodeEncoded)
 */
struct BVHNode {
    vec4 lmin; ///< Left child AABB min + left child index in w component
    vec4 lmax; ///< Left child AABB max + right child index in w component
    vec4 rmin; ///< Right child AABB min + triangle count in w component
    vec4 rmax; ///< Right child AABB max + first triangle index in w component
};

/**
 * @brief Fetches triangle data from the texture buffer object (TBO)
 * @param index Index of the triangle to fetch
 * @param trangles_buffer Sampler for the texture buffer storing packed triangle data (positions, normals, and UVs)
 * @return Fetched Triangle structure with position and normal data
 */
Triangle FetchTriangle(int index, samplerBuffer trangles_buffer) {
    int base = index * 16; // 16 vec4 (updated to include medium parameters)
    Triangle tri;
    
    // Fetch vertex positions and extract uv.x from w component
    vec4 pos1 = texelFetch(trangles_buffer, base + 0);
    vec4 pos2 = texelFetch(trangles_buffer, base + 1);
    vec4 pos3 = texelFetch(trangles_buffer, base + 2);
    
    tri.p1 = pos1.xyz;
    tri.p2 = pos2.xyz;
    tri.p3 = pos3.xyz;
    
    // Fetch vertex normals and extract uv.y from w component
    vec4 norm1 = texelFetch(trangles_buffer, base + 3);
    vec4 norm2 = texelFetch(trangles_buffer, base + 4);
    vec4 norm3 = texelFetch(trangles_buffer, base + 5);
    
    tri.n1 = norm1.xyz;
    tri.n2 = norm2.xyz;
    tri.n3 = norm3.xyz;
    
    // Reconstruct uv coordinates from w components
    tri.t1 = vec2(pos1.w, norm1.w);
    tri.t2 = vec2(pos2.w, norm2.w);
    tri.t3 = vec2(pos3.w, norm3.w);

    // Fetch material parameters
    vec4 mat_type_data = texelFetch(trangles_buffer, base + 6);
    vec4 emission_data = texelFetch(trangles_buffer, base + 7);
    vec4 diffuse_data = texelFetch(trangles_buffer, base + 8);
    vec4 roughness_data = texelFetch(trangles_buffer, base + 9);

    tri.material_type = int(mat_type_data.x);
    tri.emission = emission_data;
    tri.diffuse = diffuse_data;
    tri.roughness = roughness_data;
    
    // Fetch medium parameters and extract useful information
    vec4 in_type_info = texelFetch(trangles_buffer, base + 10);
    vec4 in_sigma_s_data = texelFetch(trangles_buffer, base + 11);
    vec4 in_sigma_t_data = texelFetch(trangles_buffer, base + 12);
    
    vec4 out_type_info = texelFetch(trangles_buffer, base + 13);
    vec4 out_sigma_s_data = texelFetch(trangles_buffer, base + 14);
    vec4 out_sigma_t_data = texelFetch(trangles_buffer, base + 15);
    
    // Extract inside medium parameters
    tri.in_phase_type = int(in_type_info.x);
    tri.in_g = in_type_info.y;
    tri.in_medium_type = int(in_type_info.z);
    tri.has_in_medium = in_type_info.w >= 0.0f ? true : false;
    tri.in_sigma_s = in_sigma_s_data.xyz;
    tri.in_sigma_t = in_sigma_t_data.xyz;
    
    // Extract outside medium parameters
    tri.out_phase_type = int(out_type_info.x);
    tri.out_g = out_type_info.y;
    tri.out_medium_type = int(out_type_info.z);
    tri.has_out_medium = in_type_info.w >= 0.0f ? true : false;
    tri.out_sigma_s = out_sigma_s_data.xyz;
    tri.out_sigma_t = out_sigma_t_data.xyz;
    
    return tri;
}

/**
 * @brief Fetches BVH node data from the bvhnodes TBO
 * @param index Index of the BVH node to fetch (0-based)
 * @param bvh_nodes_buffer Sampler for the texture buffer storing BVH node data (each node is stored as 4 vec4)
 * @return Fetched BVHNode structure with AABB and child/triangle information
 */
BVHNode FetchBVHNode(int index, samplerBuffer bvh_nodes_buffer) {
    int base = index * 4;
    BVHNode node;
    
    node.lmin = texelFetch(bvh_nodes_buffer, base + 0);
    node.lmax = texelFetch(bvh_nodes_buffer, base + 1);
    node.rmin = texelFetch(bvh_nodes_buffer, base + 2);
    node.rmax = texelFetch(bvh_nodes_buffer, base + 3);
    
    return node;
}

/**
 * @brief BVH traversal function for a single buffer
 * @param ray Ray structure containing origin, direction, min and max distance
 * @param bvh_nodes_buffer BVH nodes buffer to traverse
 * @param triangles_buffer Triangles buffer for intersection tests
 * @param is_light_buffer Whether this is the light buffer traversal
 * @return Hit structure containing intersection information
 */
Hit BVHTraverseSingleBuffer(const Ray ray, samplerBuffer bvh_nodes_buffer, samplerBuffer triangles_buffer, bool is_light_buffer) {
    Hit hit;
    hit.distance = ray.tmax;
    hit.u = 0.0f;
    hit.v = 0.0f;
    hit.tri_index = -1;
    hit.is_light = is_light_buffer;
    
    int stack[64];
    int stack_ptr = 0;
    int current_node = 0; // Start from root node
    
    // Precompute reciprocal of direction for AABB testing
    vec3 inv_d = 1.0f / ray.direction;
    
    while (true) {
        BVHNode n = FetchBVHNode(current_node, bvh_nodes_buffer);
        
        // Extract child indices and triangle information from node
        int left_child = int(n.lmin.w);
        int right_child = int(n.lmax.w);
        int tri_count = int(n.rmin.w);
        int first_tri = int(n.rmax.w);
        
        // Leaf node
        if (tri_count > 0) {
            for (int i = 0; i < tri_count; i++) {
                int tri_idx = first_tri + i;
                Triangle tri = FetchTriangle(tri_idx, triangles_buffer);
                
                // Möller–Trumbore intersection algorithm
                vec3 edge1 = tri.p2 - tri.p1;
                vec3 edge2 = tri.p3 - tri.p1;
                vec3 h = cross(ray.direction, edge2);
                float a = dot(edge1, h);
                
                if (abs(a) < Epsilon) {
                    continue;
                }
                
                float f = 1.0f / a;
                vec3 s = ray.origin - tri.p1;
                float u = f * dot(s, h);
                
                if (u < 0.0f || u > 1.0f) {
                    continue;
                }
                    
                vec3 q = cross(s, edge1);
                float v = f * dot(ray.direction, q);
                
                if (v < 0.0f || u + v > 1.0f) {
                    continue;
                }           
                
                float d = f * dot(edge2, q);
                
                // Use ray.tmin instead of hardcoded 0.0 to avoid self-intersection
                if (d > ray.tmin && d < hit.distance) {
                    hit.distance = d;
                    hit.u = u;
                    hit.v = v;
                    hit.tri_index = tri_idx;
                }
            }
            
            if (0 == stack_ptr) {
                break;
            }
            current_node = stack[--stack_ptr];
            continue;
        }
        
        // Internal node: test children AABBs using direct division
        vec3 t1a = (n.lmin.xyz - ray.origin) * inv_d;
        vec3 t2a = (n.lmax.xyz - ray.origin) * inv_d;
        vec3 t1b = (n.rmin.xyz - ray.origin) * inv_d;
        vec3 t2b = (n.rmax.xyz - ray.origin) * inv_d;
        
        vec3 min1 = min(t1a, t2a);
        vec3 max1 = max(t1a, t2a);
        vec3 min2 = min(t1b, t2b);
        vec3 max2 = max(t1b, t2b);
        
        // Consider ray.tmin in AABB intersection tests
        float tmin1 = max(max(min1.x, min1.y), min1.z);
        float tmax1 = min(min(max1.x, max1.y), max1.z);
        float tmin2 = max(max(min2.x, min2.y), min2.z);
        float tmax2 = min(min(max2.x, max2.y), max2.z);
        
        // Ensure we don't consider intersections before ray.tmin
        float dist1 = (tmin1 > tmax1 || tmax1 < ray.tmin) ? MaxFloat : max(tmin1, ray.tmin);
        float dist2 = (tmin2 > tmax2 || tmax2 < ray.tmin) ? MaxFloat : max(tmin2, ray.tmin);
        
        // Order children by distance
        if (dist1 > dist2) {
            float temp_dist = dist1;
            dist1 = dist2;
            dist2 = temp_dist;
            
            int temp_node = left_child;
            left_child = right_child;
            right_child = temp_node;
        }
        
        // Traverse or pop from stack
        if (MaxFloat == dist1) {
            if (0 == stack_ptr) {
                break;
            }
            current_node = stack[--stack_ptr];
        } 
        else {
            current_node = left_child;
            if (MaxFloat != dist2) {
                stack[stack_ptr++] = right_child;
            }
        }
    }
    
    return hit;
}

/**
 * @brief BVH traversal function for ray tracing that traverses both regular and light buffers
 * @param ray Ray structure containing origin, direction, min and max distance
 * @return Hit structure containing intersection information with the closest hit
 */
Hit BVHTraverse(const Ray ray) {
    // Traverse both regular triangles and light triangles
    Hit regular_hit = BVHTraverseSingleBuffer(ray, BVHNodes, Triangles, false);
    Hit light_hit = BVHTraverseSingleBuffer(ray, BVHNodesLight, TrianglesLight, true);
    
    return (regular_hit.distance < light_hit.distance) ? regular_hit : light_hit;
}

#endif // SHAPE_GLSL