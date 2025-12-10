
#pragma once

#include <string>

namespace common {

// Generic function to get last error message.
// Call directly after the command or use the error num.
// This function might change the error code.
// Defined in error.cpp.
[[nodiscard]] auto GetLastErrorMsg() -> std::string;

// Like GetLastErrorMsg(), but passing an explicit error code.
// Defined in error.cpp.
[[nodiscard]] auto NativeErrorToString(int e) -> std::string;

}  // namespace common
