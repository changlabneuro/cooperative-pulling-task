#include "time.hpp"
#include <sstream>
#include <ctime>
#include <iomanip>

std::string om::date_string() {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::stringstream ss;
  ss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
  return ss.str();
}