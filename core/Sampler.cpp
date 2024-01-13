#include "Sampler.h"
#include "SobolMatrices1024x52.h"

Independent::Independent() : Sampler(SamplerType::IndependentSampler) {
	rng.seed(GlobalRandomEngine());
}

float Independent::Get1() {
	return std::uniform_real_distribution<float>(0.0f, FloatOneMinusEpsilon)(rng);
}

void Independent::SetPixel(int x, int y) {
	rng.seed(GlobalRandomEngine());
}

void Independent::NextSample() {
	rng.seed(GlobalRandomEngine());
}

void Independent::NextSamples(size_t samples) {
	rng.seed(GlobalRandomEngine());
}

SimpleSobol::SimpleSobol(uint32_t seed) : seed(seed), scramble(seed), Sampler(SamplerType::SimpleSobolSampler) {}

uint32_t SobolSample(uint64_t index, int dim, uint32_t scramble = 0) {
	uint32_t r = scramble;
	for (int i = dim * SobolMatricesSize; index; index >>= 1, i++) {
		if (index & 1) {
			r ^= SobolMatrices[i];
		}
	}

	return r;
}

float SimpleSobol::Get1() {
	float r = static_cast<float>(SobolSample(index, dim++, scramble)) * 0x1p-32f;

	return std::min(r, FloatOneMinusEpsilon);
}

void SimpleSobol::SetPixel(int x, int y) {
	dim = 0;
	rng.seed(y << 16 | x);
	scramble = rng();
}

void SimpleSobol::NextSample() {
	index++;
	dim = 0;
}

void SimpleSobol::NextSamples(size_t samples) {
	index += samples;
	dim = 0;
}

std::shared_ptr<Sampler> Sampler::Create(const SamplerParams& params) {
	if (params.type == SamplerType::IndependentSampler) {
		return std::make_shared<Independent>();
	}
	else if (params.type == SamplerType::SimpleSobolSampler) {
		return std::make_shared<SimpleSobol>(params.seed);
	}

	return NULL;
}
