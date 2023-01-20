/*
 * @Author: CBH37
 * @Date: 2023-01-10 00:07:35
 * @Description: nl虚拟机头文件
 */
#pragma once
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "nl.hpp"
#include "global.hpp"
#include "nlc_def.hpp"
#include "mnem_def.hpp"

// 不同平台访问共享文件的`API`不同（现仅支持`Windows`和`Linux`两个系统）
#if(defined __linux__)
    #include <dlfcn.h>
#elif(defined _WIN32 || defined _WIN64)
#endif

class Nvm {
public:
    Nvm(std::string inputFileName);

private:
    /************ Load File（加载文件）部分 ************/
    std::ifstream input;
    std::vector<NlcFile::Num> numTable;
    std::vector<std::string> stringTable;
    void loadFile(void);

    /************ Execute（执行）部分 ************/
    bool objectToBool(NlObject object); // 将普通值转为布尔值
    void execute(void);
};