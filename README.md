# DreamRender
这是我写的第一个离线渲染器，起源于raytracing三部曲，但已经被我改了看不出来了😄，未来会更新更多的渲染算法(参考了github上的大量开源项目，构建非常慢，资产在release的压缩包里，想运行可以去下release版本的:https://github.com/GraphicsEnthusiast/DreamRender/releases/tag/v1.0 )。

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

- 场景描述
  - json场景解析

- 截图

BreakfastRoom:
![breakfast](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/1bcd9866-c7a0-4244-b9e5-ef6a5a0bf62d)
Staircase
![staircase](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/f37ba1ac-5446-4d73-a518-7f8578bf80a3)
Mis
![mis](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/02720511-924a-42e4-b13d-c7c10a5f8b4c)
CornellBox
![cornellbox](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/e3916bde-65b4-4f96-a69f-a834aab860cf)
Teapot
![teapot](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/1578a3c0-70d3-4ee2-b1c7-dc5c8896bacd)
Boy
![boy](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/9f009d6b-a023-47d3-bbd2-0abaa3c7afd0)
![boy2](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/6fb67647-6300-4ad1-832e-9289efc0e00f)

