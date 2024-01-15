#include "TestScenes.h"

int main() {
//	SampledSpectrum::Init();

//	auto renderer = TestScenes::Diningroom_MeshLight();
//	auto renderer = TestScenes::Diningroom_EnvironmentLight();
//	auto renderer = TestScenes::Subsurface();
	auto renderer = TestScenes::Surface();
	renderer->Run();

	return 0;

// 	int Width = 800;
// 	int Height = 800;
// 
// 	float sigma_a[3] = { 1.0f, 1.0f, 1.0f };
// 	float sigma_s[3] = { 1.0f, 1.0f, 1.0f };
// 	float scale = 0.1f;
// 	auto medium = std::make_shared<Homogeneous>(std::make_shared<Isotropic>(), Spectrum::FromRGB(sigma_s), Spectrum::FromRGB(sigma_a), scale);
// 
// 	float sigma_a2[3] = { 1.0f, 1.5f, 1.5f };
// 	float sigma_s2[3] = { 0.5f, 0.75f, 0.25f };
// 	float scale2 = 1.0f;
// 	//auto medium2 = std::make_shared<Homogeneous>(std::make_shared<Isotropic>(), Spectrum::FromRGB(sigma_s2), Spectrum::FromRGB(sigma_a2), scale2);
// 	
// 	//medium2 = NULL;
// 	medium = NULL;
// 	//std::shared_ptr<Medium> medium = NULL;
// 	//Transform tran = Transform::Rotate(0.0f, 45.0f, 0.0f);
// 	Transform tran2 = Transform::Translate(1.0f, 1.0f, 1.0f) * Transform::Scale(0.25f, 0.25f, 0.25f) * Transform::Rotate(0.0f, 0.0f, 0.0f);
// 	//Pinhole camera(Point3f(0.0f, 1.0f, 4.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 55.0f, (float)Width / (float)Height, medium);
// 	//Thinlens camera2(Point3f(10.0f, 8.0f, 10.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 60.0f, (float)Width / (float)Height, 2.0f);
// 	//PostProcessing post(std::make_shared<Reinhard>(), 0.0f);
// 	RTCDevice rtc_device = rtcNewDevice(NULL);
// 	Scene scene(rtc_device);
// 
// 	float specular[3] = { 1.0f, 1.0f, 1.0f };
// 	float albedo[3] = { 0.8f, 0.2f, 0.3f };
// 	float albedo2[3] = { 0.1f, 0.5f, 0.6f };
// 	float roughness[3] = { 0.0f };
// 	float roughness2[3] = { 0.3f };
// 	float roughness3[3] = { 0.6f };
// 	float metallic[3] = { 0.8f };
// 	float radiance[3] = { 100.0f, 100.0f, 100.0f };
// 	float radiance2[3] = { 5.0f, 0.0f, 0.0f };
// 	float eta[3] = { 2.76404, 1.95417, 1.62766 };
// 	float k[3] = { 3.83077, 2.73841, 2.31812 };
// //	auto material = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
// 	auto material2 = std::make_shared<DiffuseLight>(Spectrum::FromRGB(radiance));
// 	auto boundary = std::make_shared<MediumBoundary>();
// // 	auto material3 = std::make_shared<DiffuseLight>(Spectrum::FromRGB(radiance2));
//  	auto material4 = std::make_shared<Diffuse>(std::make_shared<Constant>(Spectrum::FromRGB(albedo2)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)));
// // 	auto material5 = std::make_shared<Conductor>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness3)),
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness3)), Spectrum::FromRGB(eta), Spectrum::FromRGB(k));
// // 	auto material6 = std::make_shared<Dielectric>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness3)),
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness3)), 1.5f, 1.0f);
// // 	auto material7 = std::make_shared<Plastic>(std::make_shared<Constant>(Spectrum::FromRGB(albedo2)), std::make_shared<Constant>(Spectrum::FromRGB(specular)), 
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f, true);
// // 	auto material8 = std::make_shared<ThinDielectric>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 1.5f, 1.0f);
// // 	auto material9 = std::make_shared<MetalWorkflow>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)), std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), std::make_shared<Constant>(Spectrum::FromRGB(metallic)));
// // 	auto material10 = std::make_shared<ClearCoatedConductor>(material5, std::make_shared<Constant>(Spectrum::FromRGB(roughness)),
// // 		std::make_shared<Constant>(Spectrum::FromRGB(roughness)), 2.0f);
// // 	auto material11 = std::make_shared<DiffuseTransmitter>(std::make_shared<Constant>(Spectrum::FromRGB(albedo)));
// // 	auto material12 = std::make_shared<Mixture>(material8, material6, 0.5f);
// 
// 	// Create phase function
// 	PhaseFunctionParams phaseParams{ PhaseFunctionType::IsotropicPhaseFunction, Spectrum(0.0f) };
// 	auto phase = PhaseFunction::Create(phaseParams);
// 
// 	// Create medium
// 	MediumParams mediumParams{ MediumType::HomogeneousMedium, phase, Spectrum::FromRGB(sigma_s2), Spectrum::FromRGB(sigma_a2), scale2 };
// 	auto medium2 = Medium::Create(mediumParams);
// 
// 	// Create texture
// 	TextureParams textureParams{ TextureType::ConstantTexture, Spectrum::FromRGB(albedo), "" };
// 	auto texture = Texture::Create(textureParams);
// 
// 	// Create material
// 	MaterialParams materialParams;
// 	materialParams.type = MaterialType::DiffuseMaterial;
// 	materialParams.albedoTexture = texture;
// 	materialParams.roughnessTexture = std::make_shared<Constant>(Spectrum::FromRGB(roughness));
// 	materialParams.normalTexture = NULL;
// 	auto material = Material::Create(materialParams);
// 
// 	// Create camera
// 	CameraParams cameraParams{ CameraType::PinholeCamera, Point3f(0.0f, 1.0f, 4.0f), Point3f(0.0f), Vector3f(0.0f, 1.0f, 0.0f), 1.0f, 55.0f,
// 		(float)Width / (float)Height, 0.0f, medium };
// 	auto camera = Camera::Create(cameraParams);
// 
// 	// Create filter
// 	FilterParams filterParams{ FilterType::GaussianFilter };
// 	auto filter = Filter::Create(filterParams);
// 
// 	// Create shape
// 	ShapeParams shapeParams;
// 	shapeParams.type = ShapeType::SphereShape;
// 	shapeParams.material = material2;
// 	shapeParams.center = Point3f(1.0f, 1.0f, 1.0f);
// 	shapeParams.radius = 0.25f;
// 	shapeParams.out_medium = NULL;
// 	shapeParams.in_medium = NULL;
// 	auto shape = Shape::Create(shapeParams);
// 
// 	// Create light
// 	LightParams lightParams{ LightType::SphereAreaLight, shape, NULL, 0.0f };
// 	auto light = Light::Create(lightParams);
// 
// 	scene.AddShape(new Quad(material4, Point3f(-10.0f, -0.51f, -10.0f), Vector3f(20.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 20.0f), medium, medium));
// 	scene.AddShape(new Sphere(material, Point3f(0.0f, 0.0f, 0.0f), 0.5f, medium2));
// 	scene.AddShape(new Sphere(boundary, Point3f(0.0f, 0.5f, 0.0f), 1.0f, medium, medium2));
// //	scene.AddShape(new TriangleMesh(material12, "teapot.obj", tran));
// 	scene.AddLight(std::make_shared<QuadArea>(new Quad(material2, Point3f(-0.25f, 1.75f, -0.25f), Vector3f(0.5f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.5f))));
// // 	scene.AddLight(light);
// //	scene.AddLight(std::make_shared<TriangleMeshArea>(new TriangleMesh(material2, "sphere.obj", tran2, medium)));//
// // 	scene.AddLight(std::make_shared<SphereArea>(new Sphere(material3, Point3f(14.0f, 8.0f, -14.0f), 3.0f)));
// //	scene.AddLight(std::make_shared<InfiniteArea>(std::make_shared<Hdr>("spruit_sunrise_4k.hdr"), 1.0f));
// 	scene.SetCamera(camera);
// 	scene.Commit();
// 
// 	VolumetricPathTracing vpt(std::make_shared<Scene>(scene), std::make_shared<Independent>(), filter, Width, Height, 50);
// 
// 	// Create sampler
// 	SamplerParams samplerParams{ SamplerType::IndependentSampler, 0 };
// 	auto sampler = Sampler::Create(samplerParams);
// 
// 	// Create integrator
// 	IntegratorParams integratorParams{ IntegratorType::VolumetricPathTracingIntegrator ,std::make_shared<Scene>(scene), sampler, filter, Width, Height, 50 };
// 	auto integrator = Integrator::Create(integratorParams);
// 
// 	// Create tone mapper
// 	ToneMapperParams toneParams{ ToneMapperType::ReinhardToneMapper };
// 	auto tone = ToneMapper::Create(toneParams);
// 
// 	// Create post processing
// 	PostProcessingParams postParams{ tone, 0.0f };
// 	auto post = PostProcessing::Create(postParams);
// 
// 	// Create renderer
// 	RendererParams rendererParams{ integrator, post };
// 	auto renderer = Renderer::Create(rendererParams);
// 
// 	renderer->Run();
// 
// 	return 0;
}
