void BM_empty(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(state.iterations());
  }
}
BENCHMARK(BM_empty)->ThreadPerCpu();
BENCHMARK(BM_empty)->DenseThreadRange(1, 4);