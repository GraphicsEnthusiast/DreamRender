# DreamRender

My cpu offline renderer for learning. If I have time, I will keep refactoring the code. I don't plan on writing scene analysis, it's too tiring. Some features I want to implement in the future: (1) heterogeneous medium; (2) approximate bssrdf; (3) hair material; (4) dreamworks fabric material; (5) volumetric bidirectional path tracing; (6) stochastic progressive photon mapping...

- Build Project
  - Execute build.bat
 
- Spectrum
  - RGB Spectrum
  - Sampled Spectrum

- Light Transport Method
  - Volumetric Path Tracing

- Geometry
  - Triangle Mesh
  - Sphere
  - Quad

- Accelerated Structure
  - Embree3

- Material
  - Diffuse
  - Conductor
  - Dielectric
  - Plastic
  - Thin Dielectric
  - Metal Workflow
  - Clearcoated Conductor
  - Diffuse Transmitter
  - Mixture
  - Randow Walk Subsurface

- Phase Function
  - Isotropic
  - Henyey Greenstein

- Medium
  - Homogeneous

- Camera
  - Pinhole
  - Thinlens

- Sampler
  - Independent
  - Simple Sobol

- Filter
  - Gaussian
  - Box
  - Triangle
  - Tent

- Light
  - Quad Area
  - Sphere Area
  - Triangle Mesh Area
  - Infinite Area

- Gallery

RGB Spectrum Scenes:
![Diningroom_MeshLight(spp=1307，rgb)](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/8604f2ea-11ff-455a-b383-9b48cf55a722)

  
Sampled Spectrum Scenes:
![Diningroom_MeshLight(spp=512)](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/64ee949c-06a6-4174-a490-cd74b1ca1232)

![Diningroom_SphereLight(spp=256)](https://github.com/GraphicsEnthusiast/DreamRender/assets/75780167/8744dabe-75ee-4c1d-abb8-5c6cd204214e)


