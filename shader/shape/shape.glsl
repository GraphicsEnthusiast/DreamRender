#ifndef _SHAPE__GLSL__
#define _SHAPE__GLSL__

uniform samplerBuffer Triangles;
uniform samplerBuffer Indices;
uniform samplerBuffer BVHNodes;

const float MaxFloat = 3.402823466e+38f;

/**
 * @brief Ray structure for ray tracing parameters
 */
struct Ray {
    vec3 origin;    ///< Ray origin point
    vec3 direction; ///< Ray direction vector (normalized)
    float tmin;     ///< Minimum ray distance (avoid self-intersection)
    float tmax;     ///< Maximum ray distance
};

/**
 * @brief Hit information structure for ray intersection results
 */
struct Hit {
    float distance; ///< Intersection distance along the ray
    float u;        ///< Barycentric u coordinate
    float v;        ///< Barycentric v coordinate
    int tri_index;  ///< Intersected triangle index
};

/**
 * @brief Triangle data structure (must match CPU-side TriangleEncoded)
 */
struct Triangle {
    vec3 p1, p2, p3; ///< Vertex positions
    vec3 n1, n2, n3; ///< Vertex normals
    vec3 t1, t2, t3; ///< Vertex texcoords(z=0.0f)
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
 * @return Fetched Triangle structure with position and normal data
 */
Triangle FetchTriangle(int index) {
    int base = index * 9; // Each triangle occupies 9 vec3s in the buffer (3 for positions + 3 for normals + 3 for texcoords)
    Triangle tri;
    // Fetch vertex positions
    tri.p1 = texelFetch(Triangles, base + 0).xyz;
    tri.p2 = texelFetch(Triangles, base + 1).xyz;
    tri.p3 = texelFetch(Triangles, base + 2).xyz;
    // Fetch vertex normals
    tri.n1 = texelFetch(Triangles, base + 3).xyz;
    tri.n2 = texelFetch(Triangles, base + 4).xyz;
    tri.n3 = texelFetch(Triangles, base + 5).xyz;
    // Fetch vertex texcoords
    tri.t1 = texelFetch(Triangles, base + 6).xyz;
    tri.t2 = texelFetch(Triangles, base + 7).xyz;
    tri.t3 = texelFetch(Triangles, base + 8).xyz;
    
    return tri;
}

/**
 * @brief Fetches triangle index data from the indices TBO
 * @param index Index position in the indices buffer
 * @return Triangle index value that points to the actual triangle data
 */
int FetchIndex(int index) {
    return int(texelFetch(Indices, index).r);
}

/**
 * @brief Fetches BVH node data from the bvhnodes TBO
 * @param index Index of the BVH node to fetch (0-based)
 * @return Fetched BVHNode structure with AABB and child/triangle information
 */
BVHNode FetchBVHNode(int index) {
    int base = index * 4;
    BVHNode node;
    
    node.lmin = texelFetch(BVHNodes, base + 0);
    node.lmax = texelFetch(BVHNodes, base + 1);
    node.rmin = texelFetch(BVHNodes, base + 2);
    node.rmax = texelFetch(BVHNodes, base + 3);
    
    return node;
}

/**
 * @brief BVH traversal function for ray tracing
 * @param ray Ray structure containing origin, direction, min and max distance
 * @return Hit structure containing intersection information
 */
Hit BVHTraverse(const Ray ray) {
    Hit hit;
    hit.distance = ray.tmax;
    hit.u = 0.0f;
    hit.v = 0.0f;
    hit.tri_index = 0;
    
    int stack[64];
    int stack_ptr = 0;
    int current_node = 0; // Start from root node
    
    // Precompute reciprocal of direction for AABB testing
    vec3 inv_d = 1.0f / ray.direction;
    
    while (true) {
        BVHNode n = FetchBVHNode(current_node);
        
        // Extract child indices and triangle information from node
        int left_child = int(n.lmin.w);
        int right_child = int(n.lmax.w);
        int tri_count = int(n.rmin.w);
        int first_tri = int(n.rmax.w);
        
        // Leaf node: test triangles
        if (tri_count > 0) {
            for (int i = 0; i < tri_count; i++) {
                int tri_idx = FetchIndex(first_tri + i);
                Triangle tri = FetchTriangle(tri_idx);
                
                // Möller–Trumbore intersection algorithm
                vec3 edge1 = tri.p2 - tri.p1;
                vec3 edge2 = tri.p3 - tri.p1;
                vec3 h = cross(ray.direction, edge2);
                float a = dot(edge1, h);
                
                if (abs(a) < 0.0000001f) {
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

#endif