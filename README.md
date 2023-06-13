# DreamRender
——————————————————————————

相对于stage-1，加入了体渲染路径追踪。

——————————————————————————

这是我写的第一个离线渲染器，起源于raytracing三部曲，但已经被我改了看不出来了😄，未来会更新更多的渲染算法(参考了github上的大量开源项目)。
直接构建即可运行，构建速度很快，移除了vcpkg，将embree和tbb换成了和oidn一样的提前编译好的库。

- 构建项目(使用vs打开运行)
```bash
mkdir build
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

- 渲染算法
  - 路径追踪(PathTracing)
  - 体渲染路径追踪(VolumetricPathTracing)

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

- 相函数
  - IsotropicPhaseFunction
  - HenyeyGreensteinPhaseFunction

- 参与介质
  - HomogeneousMedium

- 相机
  - PinholeCamera
  - ThinlensCamera

- 采样器
  - Independent
  - SimpleSobol

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

- 场景描述
  - json场景解析

- 截图
- ![y](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/dfb4d77c-32a3-40ee-99d7-a3bd895c6ac8)

![S5STOR8@6M 7JTZQPBBVMHT](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/5f7ec90f-93d3-4764-be35-64377dba9dce)

![vpt](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/97833289-3b16-4ad6-aa8b-c042fdb688bd)

![X(XKN`IW{MVA8ZG(RTV 3P](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/8f6ad074-7109-47af-8dc5-44ae2b35c33c)

