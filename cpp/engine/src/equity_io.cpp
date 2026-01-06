#include "qe/report.hpp"
#include "qe/equity_io.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace qe {

// equity output

void write_equity_csv(const std::string& path,
                      const std::vector<double>& equity) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open equity path for write: " + path);
  }

  out << "i,equity\n";

  for (std::size_t i = 0; i < equity.size(); ++i) {
    out << i << "," << equity[i] << "\n";
  }
}

} 