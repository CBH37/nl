/*
 * @Author: CBH37
 * @Date: 2023-01-09 20:02:32
 * @Description:  nl汇编器，将nl汇编编译为字节码
 */
#include "nas.hpp"

Nas::Nas(std::string input, std::string outputFileName) {
    src = input;
    output.open(outputFileName, std::ios::binary | std::ios::out);
    if(! output.is_open()) {
        error(outputFileName + " open error");
    }
    
    parser();
    pack();
    output.close();
}

void Nas::getToken() {
    tokVal = "";
    token = - 1;
    while(true) {
        if(src[index] == '#') {
            while(src[index] != '\n') {
                index ++;
            }

            index ++;

            if(src[index] == '#') {
                continue;
            }
        }

        // 解析注释后可能会到文件末尾，若此时不及时返回`tok_eof`那么后续对`src[index]`的判断将导致访问非法内存，所以判断结束的代码必须在解析注释代码之后
        if(index >= src.length()) {
            token = tok_eof;
            return;
        }

        if(isdigit(src[index]) || src[index] == '-') {
            token = tok_num;
            if(src[index] == '-') {
                tokVal += '-';
                index ++;
                if(index >= src.length() || (! isdigit(src[index]))) {
                    error("Number format error");
                }
            }

            int dotNum = 0;
            while(isdigit(src[index]) || src[index] == '.') {
                if(src[index] == '.') {
                    dotNum ++;
                }

                tokVal += src[index];
                index ++;
            }

            // nl汇编只支持十进制浮点数
            if(dotNum > 1 || (dotNum == 0 && tokVal[0] == 0)) {
                error("Number format error " + tokVal);
            }

            return;
        }

        if(isalpha(src[index]) || src[index] == '$') {
            while(isalpha(src[index])
               || isdigit(src[index]) 
               || src[index] == '$'
               || src[index] == '_') {
                tokVal += src[index];
                index ++;
            }

            // 标签定义优先级大于标签使用
            if(src[index] == ':') {
                token = tok_label_def;
                index ++;
            } else if(tokVal[0] == '$') {
                token = tok_label_name;
                tokVal = tokVal.substr(1, tokVal.length() - 1);
            } else {
                token = tok_ident;
            }
            
            return;
        }

        if(src[index] == '"') {
            token = tok_str;
            index ++;

            while(src[index] != '"') {
                if(src[index] == '\\') {
                    index ++;
                }
                
                tokVal += src[index];
                index ++;
            }

            index ++;
            return;
        }

        // 既不是空白字符或注释，也不是需要处理的字符，那么只可能为非法字符
        if(isspace(src[index])) {
            index ++;
        } else {
            error("Illegal character");
        }
    }
}

void Nas::parser(void) {
    getToken();
    while(token != tok_eof) {
        if(token == tok_label_def) {
            labels[tokVal] = offset;
            getToken();
        } else if(token == tok_ident) {
            Instr instr;
            // 通过将助记符全部转为大写以实现助记符大小写无关的功能
            std::transform(tokVal.begin(), tokVal.end(), tokVal.begin(), ::toupper);
            if(stringToMnem.count(tokVal)) {
                instr.mnem = stringToMnem[tokVal];
            } else {
                error(tokVal + " mnemonic does not exist");
            }

            offset += 1;

            getToken();
            while(token != tok_ident
               && token != tok_label_def
               && token != tok_eof) {
                switch(token) {
                    case tok_num: {
                        NlcFile::Num num = std::stold(tokVal);

                        Value value;
                        value.type = NUM;
                        if(! numTable.count(num)) {
                            numTable.insert({ num, numTable.size() });  // 因为使用x[y]=z形式会出现差错，所以只能使用`insert`函数插入
                        }
                        value.id = numTable[num];

                        instr.values.push_back(value);
                        break;
                    }

                    case tok_str: {
                        Value value;
                        value.type = STRING;
                        if(! stringTable.count(tokVal)) {
                            stringTable.insert({ tokVal, stringTable.size() });
                        }
                        value.id = stringTable[tokVal];

                        instr.values.push_back(value);
                        break;
                    }

                    case tok_label_name: {
                        Value value;
                        value.type = LABEL_NAME;
                        labelNameTable.push_back(tokVal);
                        value.id = labelNameTable.size() - 1;

                        instr.values.push_back(value);
                        break;
                    }
                }

                offset += sizeof(size_t);
                getToken();
            }

            instrs.push_back(instr);
        } else {
            error("Each line of nl assembly can only start with a label or mnemonic");
        }
    }
}

void Nas::pack(void) {
    /* 1. file header */
    NlcFile::FileHeader fileHeader = {
        .numNum = numTable.size(),
        .strNum = stringTable.size(),
    };
    output.write((char*)&fileHeader, sizeof(fileHeader));

    /* 2. numbers */
    // 为了解决使用`forEach`直接将`map`写入文件发生的乱序问题，先将`map`转为`vector`再进行从小到大的排序，最后按照已经排好从小到大的顺序正确的写入文件
    std::vector<std::pair<NlcFile::Num, size_t>> nums(numTable.begin(), numTable.end());
    std::sort(nums.begin(), nums.end(), 
        [](const std::pair<NlcFile::Num, size_t>& x, const std::pair<NlcFile::Num, size_t>& y) {
            return x.second < y.second; // 从小到大排序
        });

    for(size_t i = 0; i < nums.size(); i ++) {
        output.write((char*)&nums[i].first, sizeof(NlcFile::Num));
    }

    /* 3. strings */
    // 同写`number`到文件的缘由一致
    std::vector<std::pair<std::string, size_t>> strings(stringTable.begin(), stringTable.end());
    std::sort(strings.begin(), strings.end(),
        [](const std::pair<std::string, size_t>& x, const std::pair<std::string, size_t>& y) {
            return x.second < y.second;
        });
    for(size_t i = 0; i < strings.size(); i ++) {
        // stringLength
        int stringLength = strings[i].first.length();
        output.write((char*)&stringLength, sizeof(stringLength));

        // char*
        output.write((char*)strings[i].first.c_str(), stringLength);
    }

    /* 4. code */
    size_t offset = output.tellp(); // 得到文件头和一些数字和字符串常量造成的偏移
    for(auto instr : instrs) {
        // mnem
        char mnem = (char)instr.mnem;
        output.write(&mnem, sizeof(mnem));

        // values
        for(auto value : instr.values) {
            if(value.type == LABEL_NAME) {
                std::string labelName = labelNameTable[value.id];
                if(labels.count(labelName)) {
                    labels[labelName] += offset;   // 添加文件头和一些数字和字符串常量造成的偏移得到真实偏移
                    output.write((char*)&labels[labelName], sizeof(size_t));
                } else {
                    error("Reference non-existent label " + labelName);
                }
            } else {
                output.write((char*)&value.id, sizeof(value.id));
            }
        }
    }
}