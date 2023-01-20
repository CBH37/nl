/*
 * @Author: CBH37
 * @Date: 2023-01-14 18:25:56
 * @Description: 标准库的输入输出实现
 */
#include <iostream>
#include <string>
#include <vector>

#include "nl.hpp"

// 为了防止`cpp`重载函数导致`dlopen`时找不到对应符号，使用`C`编译方式
extern "C" {
    std::vector<std::string>* driver(void) {
        std::vector<std::string>* externFNNameTable = new std::vector<std::string>();
        *externFNNameTable = { "print", "input" };

        return externFNNameTable;
    }

    // 可输出0个或多个数字或字符串
    NlObject* print(Nlthread* thread, ListObject* args) {
        for(NlObject arg : (*args)) {
            switch(arg.type) {
                case NUM: {
                    std::cout << arg.num;
                    break;
                }

                case STRING: {
                    std::cout << *(arg.string);
                    break;
                }

                default: {
                    std::cerr << "std::io::print ERROR: argument can only be numeric or string type\n";
                    exit(- 1);
                }
            }
        }

        NlObject* returnValue = new NlObject();
        return returnValue;
    }

    // 与`Python`的`raw_input`函数相似获取整行输入并将其作为字符串返回
    NlObject* input(Nlthread* thread, ListObject* args) {
        // `input`函数不需要参数
        if((*args).size() > 0) {
            std::cerr << "std::io::input ERROR: there should be no parameter\n";
            exit(- 1);
        }

        // `target`不使用`new`就会在栈上分配，外部函数端与虚拟机端的栈不同从而就会发生差错，所以必须使用`new`在堆上分配
        std::string* target = new std::string();
        std::getline(std::cin, *target);

        NlObject* returnValue = new NlObject();
        (*returnValue).type = STRING;
        (*returnValue).string = target;

        return returnValue;
    }
}