#include "random.hpp"
#include <random>
#include <cmath>

namespace om {

class Random {
public:
  Random();
  double operator()();
  void set_seed(uint32_t seed);

private:
  std::random_device device;
  std::mt19937 generator;
  std::uniform_real_distribution<> distribution;
};

thread_local Random global_random_stream;

Random::Random() :
  generator(device()),
  distribution(0.0, 1.0) {
  //
}

double Random::operator()() {
  return distribution(generator);
}

void Random::set_seed(uint32_t seed) {
  generator.seed(seed);
}

double urand() {
  return global_random_stream();
}

double exprand(double mu) {
  return -mu * std::log(urand());
}

void seed_urand(unsigned int seed) {
  global_random_stream.set_seed(seed);
}

}