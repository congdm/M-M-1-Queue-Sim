#include <random>
#include <thread>
#include <intrin.h>

export module RNG;

namespace RNG {
	enum TypePRNG { MersenneTwister, cpuRDRAND };
	export TypePRNG mode = MersenneTwister;

	export class Generator {
	private:
		std::mt19937_64* genMt19937;
	public:
		Generator();
		void generateExponential(double* out, double lambda, int n);
		~Generator();
	};

	unsigned __int64 rdrand() {
		unsigned __int64 result;
		_rdrand64_step(&result);
		return result;
	}

	unsigned __int64 rdseed() {
		unsigned __int64 result;
		_rdseed64_step(&result);
		return result;
	}

	Generator::Generator() {
		genMt19937 = new std::mt19937_64(rdseed());
	}

	Generator::~Generator() {
		delete genMt19937;
	}

	void Generator::generateExponential(double* out, double lambda, int n) {
		if (mode == MersenneTwister) {
			std::exponential_distribution<double> distribution(lambda);
			for (int i = 0; i < n; i++)
				out[i] = distribution(*genMt19937);
		}
		else if (mode == cpuRDRAND) {
			unsigned __int64 rand;
			double x;
			double maxInt = 18446744073709551615.0;
			rdseed();
			for (int i = 0; i < n; i++) {
				rand = rdrand();
				x = rand; x = x / maxInt;
				x = std::log(1 - x) / (-lambda);
				out[i] = x;
			}
		}
	}
}