#pragma once

#include <random>
#include <Utils.h>

static std::default_random_engine GlobalRandomEngine(time(0));

enum class SamplerType {
	Independent, 
	SimpleSobol
};

class Sampler {
public:
	Sampler(SamplerType type) : type(type) {}

	virtual float Get1() = 0;
	virtual vec2 Get2() { return { Get1(), Get1() }; }
	virtual vec3 Get3() { return { Get2(), Get1() }; }
	virtual vec4 Get4() { return { Get3(), Get1() }; }

	template<int N>
	array<float, N> Get() {
		auto ret = array<float, N>();
		for (int i = 0; i < N; i++) {
			ret[i] = Get1();
		}
		return ret;
	}

	virtual void SetPixel(int x, int y) = 0;
	virtual void NextSample() = 0;
	virtual void NextSamples(size_t samples) = 0;
	virtual bool IsProgressive() const = 0;

public:
	SamplerType type;
};

class IndependentSampler : public Sampler {
public:
	IndependentSampler();

	virtual float Get1() override;
	virtual void SetPixel(int x, int y) override;
	virtual void NextSample() override;
	virtual void NextSamples(size_t samples) override;
	virtual bool IsProgressive() const override { return true; }

private:
	mt19937 rng;
};

class SobolSampler : public Sampler {
public:
	SobolSampler(uint32_t seed) :
		seed(seed), scramble(seed), Sampler(SamplerType::SimpleSobol) {}

	virtual float Get1() override;
	virtual void SetPixel(int x, int y) override;
	virtual void NextSample() override;
	virtual void NextSamples(size_t samples) override;
	virtual bool IsProgressive() const override { return true; }

private:
	uint64_t index = 0;
	int dim = 0;
	uint32_t seed = 0;
	uint32_t scramble = 0;
	mt19937 rng;
};