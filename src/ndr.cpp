/*
 * @Author: CBH37
 * @Date: 2023-01-16 21:26:03
 * @Description: 提供一系列接口方便生成`nl`汇编，类似于将底层操作抽象为`API`使操作更方便的驱动，故命名为：Nl DriveR(NDR)
 */
#include "ndr.hpp"

/* 构建nl汇编的内存形式 */
std::shared_ptr<Ndr::Block> Ndr::newBlock(std::string blockName) {
    std::shared_ptr<Block> block = std::make_shared<Block>(blockName + std::to_string(rand()));   // 为了防止重名,在传入参数名字的基础上增加随机值
    program.push_back(block);

    return block;
}

void Ndr::newInstrLoadLocal(std::shared_ptr<Block> block, std::string localName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("LOAD_LOCAL", { std::make_shared<StrVal>(localName) })));
}

void Ndr::newInstrLoadGlobal(std::shared_ptr<Block> block, std::string globalName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("LOAD_GLOBAL", { std::make_shared<StrVal>(globalName) })));
}

void Ndr::newInstrLoadNum(std::shared_ptr<Block> block, NlcFile::Num num) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("LOAD_NUM", { std::make_shared<NumVal>(num) })));
}

void Ndr::newInstrLoadString(std::shared_ptr<Block> block, std::string string) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("LOAD_STRING", { std::make_shared<StrVal>(string) })));
}

void Ndr::newInstrLoadAddr(std::shared_ptr<Block> block, std::string labelName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("LOAD_ADDR", { std::make_shared<LabelNameVal>(labelName) })));
}

void Ndr::newInstrStoreLocal(std::shared_ptr<Block> block, std::string localName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("STORE_LOCAL", { std::make_shared<StrVal>(localName) })));
}

void Ndr::newInstrStoreGlobal(std::shared_ptr<Block> block, std::string globalName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("STORE_GLOBAL", { std::make_shared<StrVal>(globalName) })));
}


void Ndr::newInstrAdd(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("ADD", {})));
}

void Ndr::newInstrSub(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("SUB", {})));
}

void Ndr::newInstrMul(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("MUL", {})));
}

void Ndr::newInstrDiv(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("DIV", {})));
}

void Ndr::newInstrMod(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("MOD", {})));
}

void Ndr::newInstrPow(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("POW", {})));
}

void Ndr::newInstrNot(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("NOT", {})));
}

void Ndr::newInstrCompare(std::shared_ptr<Block> block, std::string op) {
    newInstrLoadString(block, op);
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("COMPARE", {})));
}


void Ndr::newInstrJmp(std::shared_ptr<Block> block, std::string labelName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("JMP", { std::make_shared<LabelNameVal>(labelName) })));
}

void Ndr::newInstrJmpc(std::shared_ptr<Block> block, std::string labelName) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("JMPC", { std::make_shared<LabelNameVal>(labelName) })));
}

void Ndr::newInstrCall(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("CALL", {})));
}

void Ndr::newInstrCalle(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("CALLE", {})));
}

void Ndr::newInstrRet(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("RET", {})));
}


void Ndr::newInstrMakeList(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("MAKE_LIST", {})));
}

// `ACTION`系列所操作的对象的一系列操作在前端直接转译为汇编，所以模式是固定的，即`LOAD_STRING`加`ACTION_LIST`，因此为了方便操作，这里直接将二者合二为一入作为一个伪指令
void Ndr::newInstrActionList(std::shared_ptr<Block> block, std::string actionName) {
    newInstrLoadString(block, actionName);
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("ACTION_LIST", {})));
}

void Ndr::newInstrMakeMap(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("MAKE_MAP", {})));
}

void Ndr::newInstrActionMap(std::shared_ptr<Block> block, std::string actionName) {
    newInstrLoadString(block, actionName);
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("ACTION_MAP", {})));
}


void Ndr::newInstrPopTop(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("POP_TOP", {})));
}

void Ndr::newInstrImport(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("IMPORT", {})));
}

void Ndr::newInstrExit(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("EXIT", {})));
}

void Ndr::newInstrNop(std::shared_ptr<Block> block) {
    block -> instrs.push_back(std::shared_ptr<Instr>(new Instr("NOP", {})));
}


/* 生成代码 */
void Ndr::setBeginBlock(std::shared_ptr<Block> block) {
    if(beginBlock != nullptr) {
        error("begin block can only have one");
    }

    beginBlock = block;
}

std::string Ndr::codegen(void) {
    std::string code = "";
    if(beginBlock) {
        code = "JMP $" + beginBlock -> name + "\n";
    }

    for(auto block : program) {
        code += block -> codegen();
    }

    return code;
}

std::string Ndr::Block::codegen() {
    std::string code = name + ":\n";
    for(auto instr : instrs) {
        code += "\t" + instr -> codegen();
    }

    return code;
}

std::string Ndr::Instr::codegen() {
    std::string code = name + " ";
    for(auto arg : args) {
        code += arg -> codegen();
    }

    return code + "\n";
}

std::string Ndr::NumVal::codegen() {
    return " " + std::to_string(num) + " ";  // 数字两侧需要有空格分隔
}

std::string Ndr::StrVal::codegen() {
    return " \"" + string + "\" ";
}

std::string Ndr::LabelNameVal::codegen() {
    return  " $" + labelName + " ";
}