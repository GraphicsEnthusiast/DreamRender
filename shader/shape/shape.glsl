#ifndef _SHAPE__GLSL__
#define _SHAPE__GLSL__

uniform samplerBuffer Triangles;
uniform samplerBuffer Indices;
uniform samplerBuffer BVHNodes;

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
uint FetchIndex(int index) {
    return uint(texelFetch(Indices, index).r);
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

#endif