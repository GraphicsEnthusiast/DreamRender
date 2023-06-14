# DreamRender

相对于stage-1，加入了体渲染路径追踪，删除了点光源，平行光，注释了这个有偏的去除噪点的方法![image](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/4a793f5c-4e3f-4a14-9e2b-7979da1b00c1)
scene网盘链接: 我用夸克网盘分享了「scene.7z」：https://pan.quark.cn/s/0962a3a1b1ab

这是我写的第一个离线渲染器，起源于raytracing三部曲，但已经被我改了看不出来了😄，未来会更新更多的渲染算法(参考了github上的大量开源项目)。
直接构建即可运行，构建速度很快，移除了vcpkg，将embree和tbb换成了和oidn一样的提前编译好的库。

- 构建项目(使用vs打开运行)
```bash
mkdir build
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
cd bin/Release
powershell: ./DreamRender scene/boy.json
cmd: DreamRender scene/boy.json
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
  - 面光源(Quad，Sphere)
  - HDR环境光

- 场景描述
  - json场景解析

- 截图
![vol3](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/2885448a-17d8-447f-ba07-9af83a2053ff)
![vol4](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/2fa9bb02-063e-4ac5-becf-e21cf7e4ad42)
![vol2](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/2742b0da-857c-48b9-86e4-a645a1709184)
![vol1](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/e19ad1b9-a0f8-48fa-98e1-b32cd8daa0f7)

