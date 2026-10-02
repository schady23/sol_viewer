#pragma once
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
namespace solar::cli {
inline double number(const std::string& value) {
    std::size_t used=0;
    double result;
    try { result=std::stod(value,&used); }
    catch(const std::exception&) { throw std::runtime_error("ENTER A VALID NUMBER"); }
    if(used!=value.size()||!std::isfinite(result)) throw std::runtime_error("ENTER A FINITE NUMBER");
    return result;
}
inline int integer(const std::string& value) {
    std::size_t used=0;
    int result;
    try { result=std::stoi(value,&used); }
    catch(const std::exception&) { throw std::runtime_error("ENTER A VALID INTEGER"); }
    if(used!=value.size()) throw std::runtime_error("ENTER A VALID INTEGER");
    return result;
}
inline float real(const std::string& value) {
    const double result=number(value);
    if(std::abs(result)>std::numeric_limits<float>::max()) throw std::runtime_error("NUMBER IS TOO LARGE");
    return static_cast<float>(result);
}
}
