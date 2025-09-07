#include <interface.h>

void CreateSingleton() {
	dream::ThreadPool::Instance(1);
	dream::TriangleMeshManager::Instance();
}

void ReleaseSingleton() {
	dream::ThreadPool::Release();
	dream::TriangleMeshManager::Release();
}

int main() {
	CreateSingleton();
	auto gui = dream::Interface::Create(1280, 720);
	auto pipeline = std::make_unique<dream::TestPipeline>(gui->GetMainWindow());
	pipeline->SetRenderingSize(dream::Point2i(1280, 720));
	pipeline->Init();
	gui->SetRenderPipeline(std::move(pipeline));
	gui->Render();
	ReleaseSingleton();

	return 0;
}

//using namespace dream;

//void TestRGBToSpectrum() {
//	// 1. ��ʼ����ɫ�ռ�
//	RGBColorSpace colorSpace;
//
//	// 2. ���Դ���ɫ
//	{
//		RGB black(0, 0, 0);
//		RGBSigmoidPolynomial coeffs = colorSpace.ToRGBCoeffs(black);
//
//		// ��֤���в������Ϊ0
//		for (int lambda = 360; lambda <= 830; lambda += 50) {
//			float value = coeffs(lambda);
//			if (std::abs(value) > 1e-6f) {
//				std::cerr << "FAIL: Black spectrum not zero at " << lambda << "nm\n";
//			}
//		}
//		std::cout << "Test 1 (Black): PASS\n";
//	}
//
//	// 3. ���Դ���ɫ
//	{
//		RGB white(1, 1, 1);
//		RGBSigmoidPolynomial coeffs = colorSpace.ToRGBCoeffs(white);
//
//		// ��֤����ֵ�ӽ�1.0
//		for (int lambda = 360; lambda <= 830; lambda += 50) {
//			float value = coeffs(lambda);
//			if (std::abs(value - 1.0f) > 0.05f) { // ����5%���
//				std::cerr << "FAIL: White spectrum mismatch at " << lambda
//					<< "nm: " << value << "\n";
//			}
//		}
//		std::cout << "Test 2 (White): PASS\n";
//	}
//
//	// 4. ���Ժ�ɫ��������
//	{
//		RGB red(1, 0, 0);
//		RGBSigmoidPolynomial coeffs = colorSpace.ToRGBCoeffs(red);
//
//		// ��֤�������죩> �̲�������
//		float redValue = coeffs(650.f); // ��Ⲩ��
//		float blueValue = coeffs(450.f); // ���Ⲩ��
//
//		if (redValue < blueValue * 2.0f) { // ��������������2��
//			std::cerr << "FAIL: Red spectrum not dominant\n"
//				<< "650nm: " << redValue << " vs 450nm: " << blueValue << "\n";
//		}
//		std::cout << "Test 3 (Red Dominance): "
//			<< (redValue > blueValue * 2.0f ? "PASS" : "FAIL") << "\n";
//	}
//}
//
//int main() {
//	TestRGBToSpectrum();
//	return 0;
//}