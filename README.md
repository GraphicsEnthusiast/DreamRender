# DreamRender
这是我写的第一个离线渲染器

- 渲染算法
  - 路径追踪(PathTracing)

- 几何
  - TriangleMesh
  - Sphere
  - Quad

- 材质
  - Disney BSDF
  - 粗糙材质(Microfacet BSDF，包括金属，电介质，塑料)
  - Lambertian
  - Oren-Nayer
  - 平滑材质(包括金属，电介质，塑料)

- 相机
  - 针孔相机(PinholeCamera)

- 采样器
  - Sobol采样器

- 滤波器(Filter)
  - GaussianFilter
  - BoxFilter
  - TriangleFilter
  - TentFilter

- 光源
 - 点光源
 - 面光源(Quad，Sphere)
 - 平行光
 - HDR环境光
