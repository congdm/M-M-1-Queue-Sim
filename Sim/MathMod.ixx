#include <random>
#include <time.h>
#include <thread>

export module MathMod;

namespace MathMod {
	export void gen_exponential(double* out, double lambda, int n) {
		static thread_local std::mt19937* generator = nullptr;
		
		if (generator == nullptr) {
			std::hash<std::thread::id> hasher;
			time_t now = time(nullptr);
			unsigned int seed = now + hasher(std::this_thread::get_id());
			generator = new std::mt19937(seed);
		}

		std::exponential_distribution<double> distribution(lambda);
		for (int i = 0; i < n; i++)
			out[i] = distribution(*generator);
	}
}