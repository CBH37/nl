/*
 * @Author: CBH37
 * @Date: 2023-01-11 15:38:59
 * @Description: 全局通用函数
 */
#include "global.hpp"

void error(std::string message) {
    std::cerr << "Nl ERROR: " << message << '\n';
    exit(- 1);
}