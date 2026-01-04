#pragma once

#include <string>
#include "qe/data.hpp"

namespace qe {

// OHLCV CSV with header:
// timestamp,open,high,low,close,volume
OhlcvTable read_ohlcv_csv(const std::string& path);

} 
