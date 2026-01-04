#pragma once

#include <string>
#include "qe/data.hpp"

namespace qe {

// reads an OHLCV CSV file with headers
//  values : timestamp,open,high,low,close,volume
OhlcvTable read_ohlcv_csv(const std::string& path);

} 
