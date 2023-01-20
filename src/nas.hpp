/*
 * @Author: CBH37
 * @Date: 2023-01-09 20:01:29
 * @Description: nl汇编器头文件
 */
#pragma once
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "global.hpp"
#include "nlc_def.hpp"
#include "mnem_def.hpp"

class Nas {
public:
    Nas(std::string input, std::string outputFileName);

private:
    std::string src;
    size_t index;
    std::ofstream output;

    /************ Tokenizer部分 ************/
    enum Token {
        tok_num = 0,
        tok_str,
        tok_ident,
        tok_label_def,  // LABEL:   定义标签
        tok_label_name, // $LABEL   引用已有标签的值
        tok_eof,
    };

    int token = - 1;
    std::string tokVal;
    void getToken(void);

    /************ Parser部分 ************/
    #define DEF_X(x) { #x, x },
    std::map<std::string, Mnem> stringToMnem = {
        MNEM_GROUP
    };
    #undef DEF_X

    // 定义：指令为包括助记符和参数在内的一个整体，助记符是指令组成部分中所不可或缺者
    std::map<NlcFile::Num, size_t> numTable;
    std::map<std::string, size_t> stringTable;
    std::vector<std::string> labelNameTable;    // `labelNameTable`中的元素可以重复，只是为了结构统一而使用，所以没有必要使用`map`
    std::map<std::string, size_t> labels;

    enum InstrArg {
        NUM, STRING, LABEL_NAME,
    };

    struct Value {
        InstrArg type;
        size_t id;
    };

    struct Instr {
        Mnem mnem;
        std::vector<Value> values;
    };

    std::vector<Instr> instrs;
    size_t offset;  // 操作码一字节，指令值`sizeof(size_t)`字节，不断计算偏移量用于得出标签所对应的值
    void parser(void);

    /************ Pack部分（包装生成最后的字节码文件） ************/
    void pack(void);
};