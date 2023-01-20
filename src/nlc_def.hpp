/*
 * @Author: CBH37
 * @Date: 2023-01-09 23:38:32
 * @Description: nlc（nl语言字节码文件 NL Code）文件格式的定义
 */
#pragma once

namespace NlcFile {
    using Num = long double;    // `nlc`文件格式中的数字是`long double`

    /* 1. file header */
    const static int magicNum = 0x20090307;  // 我的生日是`2009.3.7`，将其作为魔数，且该常量位于头文件中只能是静态的
    struct FileHeader {
        int magic = magicNum;
        size_t numNum;
        size_t strNum;
    };
    
    /* 2. numbers */
    /* 3. strings(string: stringLength<int> + char*) */
    /* 4. code(instrs) */
};