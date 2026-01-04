#include <iostream>
#include <string>
#include "qe/version.hpp"

int main(int argc, char** argv) {
  if (argc >= 2) {
    std::string arg = argv[1];
    // version check
    if (arg == "--version" || arg == "-v") {
      std::cout << "qe_cli version " << qe::version() << "\n";
      return 0;
    }
  }

  std::cout << "qe_cli (WIP)\n";
  std::cout << "Try: qe_cli --version\n";
  return 0;
}
