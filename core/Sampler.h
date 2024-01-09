#pragma once

#include "Utils.h"

static std::default_random_engine GlobalRandomEngine(time(0));

enum class SamplerType {
	IndependentSampler,
	SimpleSobolSampler
};

class Sampler {
public:
	Sampler(SamplerType type) : m_type(type) {}

	virtual float Get1() = 0;

	inline virtual Point2f Get2() { 
		return { Get1(), Get1() }; 
	}

	inline virtual Point3f Get3() { 
		return { Get2(), Get1() }; 
	}

	inline virtual Point4f Get4() { 
		return { Get3(), Get1() }; 
	}

	template<int N>
	std::array<float, N> Get() {
		auto ret = array<float, N>();
		for (int i = 0; i < N; i++) {
			ret[i] = Get1();
		}

		return ret;
	}

	virtual void SetPixel(int x, int y) = 0;

	virtual void NextSample() = 0;

	virtual void NextSamples(size_t samples) = 0;

	inline SamplerType GetType() const {
		return m_type;
	}

protected:
	SamplerType m_type;
};

class Independent : public Sampler {
public:
	Independent();

	virtual float Get1() override;

	virtual void SetPixel(int x, int y) override;

	virtual void NextSample() override;

	virtual void NextSamples(size_t samples) override;

private:
	std::mt19937 rng;
};

class SimpleSobol : public Sampler {
public:
	SimpleSobol(uint32_t seed);

	virtual float Get1() override;

	virtual void SetPixel(int x, int y) override;

	virtual void NextSample() override;

	virtual void NextSamples(size_t samples) override;

private:
	uint64_t index = 0;
	int dim = 0;
	uint32_t seed = 0;
	uint32_t scramble = 0;
	std::mt19937 rng;
};