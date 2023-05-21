# DreamRender
这是我写的第一个离线渲染器(参考了github上的大量开源项目)，测试场景用到的资产：我用夸克网盘分享了「TestScene.zip」，链接：https://pan.quark.cn/s/c712f40dab1c

glfw, glad, glm, nlohmann_json库用vcpkg安装，构建时记得修改vcpkg路径

![image](https://github.com/qaz123w/DreamRender/assets/75780167/99953c96-80ea-4e0d-a902-e892995be9d0)

构建后还需将所需dll移动到exe文件所在的目录。

- 渲染算法
  - 路径追踪(PathTracing，实现了多重重要性采样)

- 几何
  - TriangleMesh
  - Sphere
  - Quad

- 加速结构
  - 使用embree3进行光线求交

- 降噪
  - 可选择是否开启oidn降噪

- 材质
  - Disney BSDF
  - 粗糙材质(GGX Microfacet BSDF，包括金属，电介质，塑料，以及kulla-conty方法，采样VNDF)
  - Lambertian
  - Oren-Nayer
  - 平滑材质(包括金属，电介质，塑料)
  - 薄的电介质(ThinDielectric)
  - 金属工作流(MetalWorkflow，即Cook-Torrance BRDF)
  - ClearcoatedConductor(在粗糙金属表面涂一层清漆)

- 相机
  - PinholeCamera
  - ThinlensCamera

- 采样器
  - Sobol采样器

- 滤波器
  - GaussianFilter
  - BoxFilter
  - TriangleFilter
  - TentFilter

- 光源
  - 点光源
  - 面光源(Quad，Sphere)
  - 平行光
  - HDR环境光

- 截图

MitsubaKnob
![DisneyPrinciple_spp=128](https://github.com/qaz123w/DreamRender/assets/75780167/bfa099a5-f65f-48eb-acf0-280e43ee95c3)
CornellBox
![spp=128](https://github.com/qaz123w/DreamRender/assets/75780167/10bd6784-cb64-40a6-968f-15346f246a96)
Teapot
![spp=128](https://github.com/qaz123w/DreamRender/assets/75780167/aacae0d8-54c6-4f92-801d-492d3ae1a2cd)
![g2](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/850e5a01-ef81-48f5-8a03-319cfd004d64)
Boy
![spp=1024_quad_light](https://github.com/qaz123w/DreamRender/assets/75780167/eb1a7b93-299f-4c9d-8552-68534994665a)
![spp=1024_hdr](https://github.com/qaz123w/DreamRender/assets/75780167/848615c9-05f1-479b-8e7a-f8ef751cafc1)
![g1](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/b79565a6-1846-4221-9878-400581c89fd8)

