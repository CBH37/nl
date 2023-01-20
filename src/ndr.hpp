/*
 * @Author: CBH37
 * @Date: 2023-01-16 21:25:39
 * @Description: `ndr`的头文件
 */
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "global.hpp"
#include "nlc_def.hpp"

/*
 * `Ndr`用法：
 * `Ndr`提供一种汇编的内存表示形式，在内存中，一个汇编程序由多个基本块组成，每个基本块由多条指令组成
 * 用户可以通过`Ndr`提供的一系列函数构建这样的汇编内存表示形式，同时也可以将内存表示形式转换为文本表示形式提供给汇编器进行处理
 */
class Ndr {
private:
    // 参数值
    struct Value {
        virtual std::string codegen() = 0;
    };

    struct NumVal : public Value {
        NlcFile::Num num;
        
        NumVal(NlcFile::Num _num) : num(_num) {}
        std::string codegen();
    };

    struct StrVal : public Value {
        std::string string;

        StrVal(std::string _string) : string(_string) {}
        std::string codegen();
    };

    struct LabelNameVal : public Value {
        std::string labelName;

        LabelNameVal(std::string _labelName) : labelName(_labelName) {}
        std::string codegen();
    };

    // 指令
    struct Instr {
        std::string name;
        std::vector<std::shared_ptr<Value>> args;

        Instr(std::string _name, std::vector<std::shared_ptr<Value>> _args) : name(_name), args(_args) {}
        std::string codegen();
    };

public:
    // 基本块
    struct Block {
        std::string name;
        std::vector<std::shared_ptr<Instr>> instrs;

        Block(std::string _name) : name(_name) {}
        std::string codegen();
    };

    /************ 构建nl汇编的内存形式 ************/
    std::shared_ptr<Ndr::Block> newBlock(std::string blockName);

    void newInstrLoadLocal(std::shared_ptr<Block> block, std::string localName);
    void newInstrLoadGlobal(std::shared_ptr<Block> block, std::string globalName);
    void newInstrLoadNum(std::shared_ptr<Block> block, NlcFile::Num num);
    void newInstrLoadString(std::shared_ptr<Block> block, std::string string);
    void newInstrLoadAddr(std::shared_ptr<Block> block, std::string labelName);
    void newInstrStoreLocal(std::shared_ptr<Block> block, std::string localName);
    void newInstrStoreGlobal(std::shared_ptr<Block> block, std::string globalName);

    void newInstrAdd(std::shared_ptr<Block> block);
    void newInstrSub(std::shared_ptr<Block> block);
    void newInstrMul(std::shared_ptr<Block> block);
    void newInstrDiv(std::shared_ptr<Block> block);
    void newInstrMod(std::shared_ptr<Block> block);
    void newInstrPow(std::shared_ptr<Block> block);

    void newInstrNot(std::shared_ptr<Block> block);
    void newInstrCompare(std::shared_ptr<Block> block, std::string op);

    void newInstrJmp(std::shared_ptr<Block> block, std::string labelName);
    void newInstrJmpc(std::shared_ptr<Block> block, std::string labelName);
    void newInstrCall(std::shared_ptr<Block> block);
    void newInstrCalle(std::shared_ptr<Block> block);
    void newInstrRet(std::shared_ptr<Block> block);

    void newInstrMakeList(std::shared_ptr<Block> block);
    void newInstrActionList(std::shared_ptr<Block> block, std::string actionName);
    void newInstrMakeMap(std::shared_ptr<Block> block);
    void newInstrActionMap(std::shared_ptr<Block> block, std::string actionName);

    void newInstrPopTop(std::shared_ptr<Block> block);
    void newInstrImport(std::shared_ptr<Block> block);
    void newInstrExit(std::shared_ptr<Block> block);
    void newInstrNop(std::shared_ptr<Block> block);

    /************ 生成nl汇编 ************/
    std::shared_ptr<Block>beginBlock = nullptr;
    void setBeginBlock(std::shared_ptr<Block> block);

    std::vector<std::shared_ptr<Block>> program;
    std::string codegen(void);
};