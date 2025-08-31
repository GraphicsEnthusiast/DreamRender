#ifndef _SHAPE__GLSL__
#define _SHAPE__GLSL__

uniform samplerBuffer triangles;

/**
 * @brief Triangle data structure (must match CPU-side TriangleEncoded)
 */
struct Triangle {
    vec3 p1, p2, p3; ///< Vertex positions
    vec3 n1, n2, n3; ///< Vertex normals
    vec3 t1, t2, t3; ///< Vertex texcoords(z=0.0f)
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
    tri.p1 = texelFetch(triangles, base + 0).xyz;
    tri.p2 = texelFetch(triangles, base + 1).xyz;
    tri.p3 = texelFetch(triangles, base + 2).xyz;
    // Fetch vertex normals
    tri.n1 = texelFetch(triangles, base + 3).xyz;
    tri.n2 = texelFetch(triangles, base + 4).xyz;
    tri.n3 = texelFetch(triangles, base + 5).xyz;
    // Fetch vertex texcoords
    tri.t1 = texelFetch(triangles, base + 6).xyz;
    tri.t2 = texelFetch(triangles, base + 7).xyz;
    tri.t3 = texelFetch(triangles, base + 8).xyz;
    
    return tri;
}

#endif