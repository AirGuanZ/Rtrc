#include <iostream>
#include <random>

#include <Rtrc/Core/Math/DynamicDiscreteDistribution.h>

using namespace Rtrc;

template<typename F>
void Verify()
{
    const std::vector<F> weights = {
        static_cast<F>(1.18540),
        static_cast<F>(2.480),
        static_cast<F>(3.108),
        static_cast<F>(4.4078),
        static_cast<F>(5.94150)
    };

    DynamicDiscreteDistribution<F> sampler(1, 1);
    for(F weight : weights)
    {
        sampler.Add(weight);
    }

    std::vector<uint64_t> counts(weights.size());

    constexpr uint64_t N = 100000000;
    for(uint64_t i = 0; i < N; ++i)
    {
        const int index = sampler.Sample();
        ++counts[index];
    }

    const F sumWeights = std::ranges::fold_left(weights, F(0), [](F x, F y) { return x + y; });
    std::cout << "Weights:\n";
    for(auto weight : weights)
    {
        std::cout << static_cast<double>(weight) / sumWeights << " ";
    }
    std::cout << "\n";

    std::cout << "Samples:\n";
    for(auto count : counts)
    {
        std::cout << static_cast<double>(count) / N << " ";
    }
    std::cout << "\n";
}

int main()
{
    std::println(std::cout, "Verifying integer");
    Verify<int>();
    std::println(std::cout, "Verifying floating point");
    Verify<double>();
}
