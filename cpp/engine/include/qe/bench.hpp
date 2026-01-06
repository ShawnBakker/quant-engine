#pragma once

#include <cstddef>
#include <string>

namespace qe {


// prints timing results to stdout
int run_benchmarks(const std::string& csv_path, std::size_t iters);

} // qe
