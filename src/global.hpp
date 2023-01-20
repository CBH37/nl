/*
 * @Author: CBH37
 * @Date: 2023-01-11 15:38:04
 * @Description: 全局通用头文件
 */
#pragma once
#include <string>
#include <iostream>

/*
 * 编写代码规则：
 * 1. 为了代码更加清晰及减少冗余代码，`.cpp`代码文件中所需要的头文件只能在其对应的`.hpp`头文件中引用
 * 2. 为了统一代码风格，使用`sizeof`时后面尽量跟变量
 */
void error(std::string message);