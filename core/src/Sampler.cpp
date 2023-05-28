#include <Sampler.h>
#include <SobolMatrices1024x52.h>

IndependentSampler::IndependentSampler() : Sampler(SamplerType::Independent) {
	rng.seed(GlobalRandomEngine());
}

float IndependentSampler::Get1() {
	return uniform_real_distribution<float>(0.0f, OneMinusEpsilon)(rng);
}

void IndependentSampler::SetPixel(int x, int y) {
	rng.seed(GlobalRandomEngine());
}

void IndependentSampler::NextSample() {
	rng.seed(GlobalRandomEngine());
}

void IndependentSampler::NextSamples(size_t samples) {
	rng.seed(GlobalRandomEngine());
}

uint32_t SobolSample(uint64_t index, int dim, uint32_t scramble = 0) {
	uint32_t r = scramble;
	for (int i = dim * SobolMatricesSize; index; index >>= 1, i++) {
		if (index & 1) {
			r ^= SobolMatrices[i];
		}
	}

	return r;
}

float SobolSampler::Get1() {
	float r = static_cast<float>(SobolSample(index, dim++, scramble)) * 0x1p-32f;

	return std::min(r, OneMinusEpsilon);
}

void SobolSampler::SetPixel(int x, int y) {
	dim = 0;
	rng.seed(y << 16 | x);
	scramble = rng();
}

void SobolSampler::NextSample() {
	index++;
	dim = 0;
}

void SobolSampler::NextSamples(size_t samples) {
	index += samples;
	dim = 0;
}