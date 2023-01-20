/*
 * @Author: CBH37
 * @Date: 2023-01-10 00:08:11
 * @Description: nl虚拟机nvm，执行nl字节码
 */
#include "nvm.hpp"

Nvm::Nvm(std::string inputFileName) {
    input.open(inputFileName, std::ios::binary | std::ios::in);
    if(! input.is_open()) {
        error(inputFileName + " open error");
    }

    loadFile();
    execute();
    input.close();
}

void Nvm::loadFile(void) {
    /* 1. file header */
    NlcFile::FileHeader fileHeader;
    input.read((char*)&fileHeader, sizeof(NlcFile::FileHeader));
    
    // check magic
    if(fileHeader.magic != NlcFile::magicNum) {
        error("file corruption");
    }

    /* 2. numbers */
    for(size_t i = 0; i < fileHeader.numNum; i ++) {
        NlcFile::Num num;
        input.read((char*)&num, sizeof(num));
        numTable.push_back(num);
    }

    /* strings */
    for(size_t i = 0; i < fileHeader.strNum; i ++) {
        int stringLength;
        input.read((char*)&stringLength, sizeof(stringLength));

        // `char*`字符串必须以`\0`结尾
        char* string = (char*)malloc(stringLength + 1);
        string[stringLength] = '\0';
        input.read(string, stringLength);
        stringTable.push_back(std::string(string));
    }
}

bool Nvm::objectToBool(NlObject object) {
    switch(object.type) {
        case NUM: {
            return !! object.num;
        }

        case STRING: {
            return !! (*object.string).length();
        }

        /*
         * `list`和`map`类型（为了方便起见，归为`pointer`类型）理应通过取反其中的元素数量得到目标布尔值，而直接取反得到结果只能是`true`，不符合要求
         * 但是`cpp`这样成熟的语言都不支持对`list`和`map`这样的复杂数据类型取反，那对于`nl`这样的小语言就更不必要支持取反`list`和`map`的操作了
         * 因此在前端就可以对取反`list`和`map`的操作进行报错，那么`nvm`也就不可能接收到取反`list`和`map`操作的代码，那么取反目标也就不可能为`list`和`map`
         * 只可能为使用`calle`调用外部函数时外部函数所制造到栈上的数据，可以对这些外部函数制造到栈上的数据进行判空，所以这样操作下的结果不可能不符合要求
         */
        case POINTER: {
            return !! object.pointer;
        }
    }

    return false;   // 虽然`Nlobject`只可能有上示三种类型，但是编译器报`warn`，只得写这一行冗余代码保证编译完美通过
}

void Nvm::execute(void) {
    Nlthread thread;
    StackFrame baseStackFrame;
    thread.stack.push_back(baseStackFrame);
    thread.sp = &thread.stack[thread.stack.size() - 1];
    
    // Note: 使用`input.eof()`会出现问题
    while(input.peek() != EOF) {
        char mnem;
        input.read(&mnem, sizeof(mnem));
        switch(mnem) {
            case LOAD_LOCAL: {
                // 预热
                size_t id;
                input.read((char*)&id, sizeof(id));

                if(! thread.sp -> localVarTable.count(id)) {
                    error(stringTable[id] + " variable does not exist in the local variable table");
                }

                // 将变量对应的值加载到栈上
                thread.sp -> opStack.push_back(thread.sp -> localVarTable[id]);
                break;
            }

            case LOAD_GLOBAL: {
                // 预热
                size_t id;
                input.read((char*)&id, sizeof(id));

                if(! thread.globalVarTable.count(id)) {
                    error(stringTable[id] + " variable does not exist in the global variable table");
                }

                // 将变量对应的值加载到栈上
                thread.sp -> opStack.push_back(thread.globalVarTable[id]);
                break;
            }

            case LOAD_NUM: {
                size_t numId;
                input.read((char*)&numId, sizeof(numId));

                NlObject object;
                object.type = NUM;
                object.num = numTable[numId];
                thread.sp -> opStack.push_back(object);
                break;
            }

            case LOAD_STRING: {
                size_t strId;
                input.read((char*)&strId, sizeof(strId));

                NlObject object;
                object.type = STRING;
                object.string = &stringTable[strId];

                thread.sp -> opStack.push_back(object);
                break;
            }

            case LOAD_ADDR: {
                // 必须使用`malloc`在堆上分配空间，否则数据默认放在栈上，从`opStack`取出时就可能会因为不在同一个栈而出错
                size_t* addr = (size_t*)malloc(sizeof(size_t));
                input.read((char*)addr, sizeof(addr));

                NlObject object;
                object.type = POINTER;
                object.pointer = addr;
                thread.sp -> opStack.push_back(object);
                break;
            }

            case STORE_LOCAL: {
                // 预热
                size_t id;
                input.read((char*)&id, sizeof(id));

                if(! thread.sp -> opStack.size()) {
                    error("the STORE_LOCAL instruction requires one operand");
                }

                // 将栈顶值存储至局部变量表
                thread.sp -> localVarTable[id] = thread.sp -> opStack[thread.sp -> opStack.size() - 1];
                thread.sp -> opStack.pop_back();

                break;
            }

            case STORE_GLOBAL: {
                // 预热
                size_t id;
                input.read((char*)&id, sizeof(id));
                if(! thread.sp -> opStack.size()) {
                    error("the STORE_GLOBAL instruction requires one operand");
                }

                // 将栈顶值存储至全局变量表
                thread.globalVarTable[id] = thread.sp -> opStack[thread.sp -> opStack.size() - 1];
                thread.sp -> opStack.pop_back();

                break;
            }

            case ADD: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the ADD command parameter is incorrect");
                }

                // 获取操作数栈顶端两个数字并将其相加
                NlcFile::Num target = thread.sp -> opStack[thread.sp -> opStack.size() - 1].num
                                + thread.sp -> opStack[thread.sp -> opStack.size() - 2].num;
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case SUB: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the SUB command parameter is incorrect");
                }

                // 获取操作数栈顶端两个数字并将其相减
                NlcFile::Num target = thread.sp -> opStack[thread.sp -> opStack.size() - 1].num
                                - thread.sp -> opStack[thread.sp -> opStack.size() - 2].num;
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case MUL: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the MUL command parameter is incorrect");
                }

                // 获取操作数栈顶端两个数字并将其相乘
                NlcFile::Num target = thread.sp -> opStack[thread.sp -> opStack.size() - 1].num
                                * thread.sp -> opStack[thread.sp -> opStack.size() - 2].num;
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case DIV: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the DIV command parameter is incorrect");
                }

                // 除`0`错误
                if(thread.sp -> opStack[thread.sp -> opStack.size() - 2].num == 0) {
                    error("the second operand (dividend) in the DIV instruction cannot be 0");
                }
                // 获取操作数栈顶端两个数字并将其相除
                NlcFile::Num target = thread.sp -> opStack[thread.sp -> opStack.size() - 1].num
                                / thread.sp -> opStack[thread.sp -> opStack.size() - 2].num;
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case MOD: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the MOD command parameter is incorrect");
                }

                NlcFile::Num target = std::fmod(thread.sp -> opStack[thread.sp -> opStack.size() - 1].num,
                                                thread.sp -> opStack[thread.sp -> opStack.size() - 2].num);
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case POW: {
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != NUM) {
                    error("the MOD command parameter is incorrect");
                }

                NlcFile::Num target = std::pow(thread.sp -> opStack[thread.sp -> opStack.size() - 1].num,
                                        thread.sp -> opStack[thread.sp -> opStack.size() - 2].num);
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = target;

                break;
            }

            case NOT: {
                if(thread.sp -> opStack.size() < 1) {
                    error("the NOT instruction requires an operand");
                }

                // 取出栈顶值按类型将其取反，并将得到的布尔值作为数字存入`object.num`
                NlcFile::Num boolVal = (NlcFile::Num)! objectToBool(thread.sp -> opStack[thread.sp -> opStack.size() - 1]);
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].type = NUM;
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = boolVal;

                break;
            }
            
            // `AND`和`OR`类指令常与大于小于等比较指令在一起，且都为二个操作数，为了简化代码实现，都在`COMPARE`指令中集中实现
            case COMPARE: {
                // COMPARE [op1] [op2] [COMPARE ACTION]
                if(thread.sp -> opStack.size() < 3
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING) {
                    error("the COMPARE command parameter is incorrect");
                }

                // 与`ACTION_LIST`指令实现相同
                std::string actionName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                std::transform(actionName.begin(), actionName.end(), actionName.begin(), ::toupper);

                NlObject op1 = thread.sp -> opStack[thread.sp -> opStack.size() - 3];
                NlObject op2 = thread.sp -> opStack[thread.sp -> opStack.size() - 2];

                thread.sp -> opStack.pop_back(); 
                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1].type = NUM;   // 保留一个操作数不`pop_back`用于存放最后比较得到的布尔值

                bool boolVal;
                if(actionName == "AND") {
                    boolVal = objectToBool(op1) && objectToBool(op2);
                } else if(actionName == "OR") {
                    boolVal = objectToBool(op1) || objectToBool(op2);
                } else {
                    // 除`AND`和`OR`以外其他指令两个操作数类型必须一致
                    if(op1.type != op2.type) {
                        error("COMPARE: the prerequisite for comparison is that the types of two operands must be consistent");
                    }

                    if(actionName == "EQU") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num == op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) == *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer == op1.pointer);
                                break;
                            }
                        }
                    } else if(actionName == "NE") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num != op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) != *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer != op1.pointer);
                                break;
                            }
                        }
                    } else if(actionName == "GRE") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num > op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) > *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer > op1.pointer);
                                break;
                            }
                        }
                    } else if(actionName == "LES") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num < op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) < *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer < op1.pointer);
                                break;
                            }
                        }
                    } else if(actionName == "GE") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num >= op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) >= *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer >= op1.pointer);
                                break;
                            }
                        }
                    } else if(actionName == "LE") {
                        switch(op1.type) {
                            case NUM: {
                                boolVal = (op1.num <= op2.num);
                                break;
                            }

                            case STRING: {
                                boolVal = (*(op1.string) <= *(op2.string));
                                break;
                            }

                            case POINTER: {
                                boolVal = (op1.pointer <= op1.pointer);
                                break;
                            }
                        }
                    } else {
                        error("there is no " + actionName + " operation in the COMPARE instruction");
                    }
                }

                thread.sp -> opStack[thread.sp -> opStack.size() - 1].num = boolVal;
                break;
            }

            case JMP: {
                size_t offset;
                input.read((char*)&offset, sizeof(offset));
                input.seekg(offset, std::ios::beg);
                break;
            }

            // JuMP Conditional 若参数值为`TRUE`则跳转（有条件跳转）
            case JMPC: {
                if(thread.sp -> opStack.size() < 1) {
                    error("the JMPC instruction requires an operand");
                }

                if(objectToBool(thread.sp -> opStack[thread.sp -> opStack.size() - 1])) {
                    size_t offset;
                    input.read((char*)&offset, sizeof(offset));
                    input.seekg(offset, std::ios::beg);
                }
                
                break;
            }

            // 为了之后通过实现原型链的`Map`的语法糖来实现面向对象，`Map`中必须能存储函数地址且`CALL`指令必须能通过所存储的函数地址来调用对应的函数
            // 因此`CALL`的参数只能存于栈中（使用`LOAD_ADDR`将函数地址加载到栈上）
            case CALL: {
                // CALL [Args(List)] [Address]
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != POINTER
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                    error("the CALL command parameter is incorrect");
                }

                NlObject object = thread.sp -> opStack[thread.sp -> opStack.size() - 2];
                size_t addr = *(size_t*)thread.sp -> opStack[thread.sp -> opStack.size() - 1].pointer;
                input.seekg(addr, std::ios::beg);

                StackFrame stackFrame;
                thread.stack.push_back(stackFrame); // 创建新栈帧
                thread.sp = &thread.stack[thread.stack.size() - 1]; // 修改`sp`指向最新帧
                thread.sp -> returnAddress = input.tellg();
                thread.sp -> opStack.push_back(object);

                break;
            }

            // CALL Extern 调用外部函数
            case CALLE: {
                // CALLE [Args(List)] [Extern Function Name]
                if(thread.sp -> opStack.size() < 2
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING
                || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                    error("the CALLE instruction requires an operand");
                }

                std::string externFNName= *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                if(! thread.externFNTable.count(externFNName)) {
                    error("CALLE: External function " + externFNName + " does not exist");
                }

                ListObject* args = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                NlEFNTemplate externFN = (NlEFNTemplate)(thread.externFNTable[externFNName]);
                NlObject returnValue = *(externFN(&thread, args));

                thread.sp -> opStack.pop_back();
                thread.sp -> opStack[thread.sp -> opStack.size() - 1] = returnValue;

                break;
            }

            case RET: {
                // RET [Return Value]
                if(thread.sp -> opStack.size() < 1) {
                    error("the RET instruction requires an operand");
                }

                // 因为`RET`的前提必须是调用过`CALL`， 所以当`sp`指向基栈帧或因外部函数操作错误导致栈空时不能使用`RET`指令
                if(thread.stack.size() <= 1) {
                    error("the RET instruction cannot be used when sp points to the base stack frame or the stack is empty");
                }

                NlObject object = thread.sp -> opStack[thread.sp -> opStack.size() - 1];
                input.seekg(thread.sp -> returnAddress, std::ios::beg);
                thread.stack.pop_back();
                thread.sp = &thread.stack[thread.stack.size() - 1];
                thread.sp -> opStack.push_back(object);

                break;
            }

            case MAKE_LIST: {
                NlObject object;
                object.type = POINTER;
                object.pointer = new ListObject();

                thread.sp -> opStack.push_back(object);
                break;
            }

            // 首先根据单一职责原则，为了使代码结构更清晰，不适用类`lua`的`table`结构
            // 因为`List`有众多操作，为了简化指令集增加灵活性，统一使用`ACTION_LIST`指令处理`List`
            case ACTION_LIST: {
                // 首先通过获取栈顶的字符串得到处理`List`的方式，再根据情况获取处理`List`的方式所需的参数，而参数之下则是需要处理的目标`LIST`
                if(thread.sp -> opStack.size() < 1
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING) {
                    error("the ACTION_LIST command parameter is incorrect");
                }
            
                std::string actionName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                std::transform(actionName.begin(), actionName.end(), actionName.begin(), ::toupper);    // 将操作名转为大写实现大小写无关
                thread.sp -> opStack.pop_back();
                if(actionName == "PUSH") {
                    if(thread.sp -> opStack.size() < 2
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                        error("the ACTION_LIST(PUSH ACTION) command parameter is incorrect");
                    }

                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                    list -> push_back(thread.sp -> opStack[thread.sp -> opStack.size() - 1]);
                    thread.sp -> opStack.pop_back();
                } else if(actionName == "POP") {
                    if(thread.sp -> opStack.size() < 1
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != POINTER) {
                        error("the ACTION_LIST(POP ACTION) command parameter is incorrect");
                    }

                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 1].pointer;
                    if((*list).size() < 1) {
                        error("ACTION_LIST(POP ACTION): there must be one or more elements in the list to pop");
                    }

                    thread.sp -> opStack.push_back((*list)[(*list).size() - 1]);    // 将`list`的最后一个元素压入栈
                    list -> pop_back();
                } else if(actionName == "ASSIGN") {
                    // ASSIGN op1[op2] = op3    赋值
                    if(thread.sp -> opStack.size() < 3
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 3].type != POINTER) {
                        error("the ACTION_LIST(ASSIGN ACTION) command parameter is incorrect");
                    }

                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 3].pointer;
                    if((*list).size() <= thread.sp -> opStack[thread.sp -> opStack.size() - 2].num) {
                        error("ACTION_LIST(ASSIGN ACTION): input index is out of list range");
                    }

                    (*list)[thread.sp -> opStack[thread.sp -> opStack.size() - 2].num] = thread.sp -> opStack[thread.sp -> opStack.size() - 1];
                    thread.sp -> opStack.pop_back();
                    thread.sp -> opStack.pop_back();
                } else if(actionName == "GET") {
                    if(thread.sp -> opStack.size() < 2
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                        error("the ACTION_LIST(GET ACTION) command parameter is incorrect");
                    }

                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                    if((*list).size() <= thread.sp -> opStack[thread.sp -> opStack.size() - 1].num) {
                        error("ACTION_LIST(GET ACTION): input index is out of list range");
                    }

                    NlObject object = (*list)[thread.sp -> opStack[thread.sp -> opStack.size() - 1].num];
                    thread.sp -> opStack[thread.sp -> opStack.size() - 1] = object;
                } else if(actionName == "DEL") {
                    if(thread.sp -> opStack.size() < 2
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != NUM
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                        error("the ACTION_LIST(DEL ACTION) command parameter is incorrect");
                    }

                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                    if((*list).size() <= thread.sp -> opStack[thread.sp -> opStack.size() - 1].num) {
                        error("ACTION_LIST(DEL ACTION): input index is out of list range");
                    }

                    (*list).erase((*list).begin() + thread.sp -> opStack[thread.sp -> opStack.size() - 1].num);
                    thread.sp -> opStack.pop_back();
                } else if(actionName == "LEN") {
                    // 得到`List`的长度并放入栈中
                    if(thread.sp -> opStack.size() < 1
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != POINTER) {
                        error("the ACTION_LIST(LEN ACTION) command parameter is incorrect");
                    }
                    ListObject* list = (ListObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 1].pointer;
                    NlObject object;
                    object.type = NUM;
                    object.num = (*list).size();
                    thread.sp -> opStack.push_back(object);
                } else {
                    error("there is no " + actionName + " operation in the ACTION_LIST instruction");
                }

                break;
            }

            case MAKE_MAP: {
                NlObject object;
                object.type = POINTER;
                object.pointer = new MapObject();

                thread.sp -> opStack.push_back(object);
                break;
            }

            case ACTION_MAP: {
                // 实现方式与`ACTION_LIST`相同
                if(thread.sp -> opStack.size() < 1
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING) {
                    error("the ACTION_MAP command parameter is incorrect");
                }
            
                std::string actionName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                std::transform(actionName.begin(), actionName.end(), actionName.begin(), ::toupper);
                thread.sp -> opStack.pop_back();

                // A[B] = C
                if(actionName == "ASSIGN") {
                    if(thread.sp -> opStack.size() < 3
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != STRING
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 3].type != POINTER) {
                        error("the ACTION_MAP(ASSIGN ACTION) command parameter is incorrect");
                    }

                    MapObject* map = (MapObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 3].pointer;
                    std::string keyName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 2].string);

                    (*map)[keyName] = thread.sp -> opStack[thread.sp -> opStack.size() - 1];
                    thread.sp -> opStack.pop_back();
                    thread.sp -> opStack.pop_back();
                } else if(actionName == "DEL") {
                    if(thread.sp -> opStack.size() < 2
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                        error("the ACTION_MAP(DEL ACTION) command parameter is incorrect");
                    }

                    MapObject* map = (MapObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                    std::string keyName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                    if(! map -> count(keyName)) {
                        error("ACTION_MAP(DEL ACTION): " + keyName + " key does not exist in the map");
                    }

                    map -> erase(keyName);
                    thread.sp -> opStack.pop_back();
                } else if(actionName == "GET") {
                    if(thread.sp -> opStack.size() < 2
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 2].type != POINTER) {
                        error("the ACTION_MAP(GET ACTION) command parameter is incorrect");
                    }

                    MapObject* map = (MapObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 2].pointer;
                    std::string keyName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);

                    /*
                     * nl实现基于对象（类JS/ Lua）：
                     * nl中的对象实际就是实现了原型链的`Map`，对象的`protype`属性（即原型）是对象提供的公共信息,其他对象可以通过一个特殊的属性`__proto__`指向对象的原型，
                     * 当使用`GET`操作时`GET`会先查找当前对象中是否有目标`key`，当前对象中若没有找到目标`key`，则可以查找当前对象中`__proto__`属性所指向的原型中是否有目标`key`
                     * 只有当前对象和指向原型中都没有目标`key`，`GET`才会因找不到目标`key`而报错，相当于当前对象基于原型，基于提供原型的对象，从而实现了基于对象
                     * 同理对象指向的原型也可以指向另一个原型，直到原型没有指向的原型为止，从而形成一条原型链，`GET`沿着原型链直到找到目标`key`或到原型链末尾找不到报错为止
                     */
                    
                    // 首先查找当前对象
                    if(! map -> count(keyName)) {
                        while(true) {
                            // 查看是否有`__proto__`对象，没有`__proto__`对象说明已经到达原型链尽头还未找到目标`key`，因此直接报错
                            if(! map -> count("__proto__")) {
                                error("ACTION_MAP(GET ACTION): " + keyName + " key does not exist in the map");
                            }

                            // `__proto__`属性必须为`map`
                            if((*map)["__proto__"].type != POINTER) {
                                error("ACTION_MAP(GET ACTION): __ proto__ property must be map");
                            }

                            // 在原型中找到目标`key`就停止
                            map = (MapObject*)((*map)["__proto__"].pointer);
                            if(map -> count(keyName)) {
                                break;
                            }
                        }
                    }

                    thread.sp -> opStack[thread.sp -> opStack.size() - 1] = (*map)[keyName];
                } else if(actionName == "LEN") {
                    if(thread.sp -> opStack.size() < 1
                    || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != POINTER) {
                        error("the ACTION_MAP(LEN ACTION) command parameter is incorrect");
                    }

                    MapObject* map = (MapObject*)thread.sp -> opStack[thread.sp -> opStack.size() - 1].pointer;
                    NlObject object;
                    object.type = NUM;
                    object.num = (*map).size();
                    thread.sp -> opStack.push_back(object);
                }

                break;
            }

            case POP_TOP: {
                // 操作数栈.pop_back()  用于清除栈上无用的值
                if(thread.sp -> opStack.size() < 1) {
                    error("the POP_TOP instruction requires an operand");
                }

                thread.sp -> opStack.pop_back();
                break;
            }

            // IMPORT [SHARE FILE NAME] 加载共享文件以导入外部函数
            case IMPORT: {
                if(thread.sp -> opStack.size() < 1
                || thread.sp -> opStack[thread.sp -> opStack.size() - 1].type != STRING) {
                    error("the IMPORT instruction requires an operand");
                }

                // 不同平台处理方式也不同
                // 首先获取`driver`函数，通过调用其返回的列表得到外部共享库提供的所有外部函数名，再一一通过函数名找到对应函数将其存储至外部函数表以供`CALLE`调用外部函数使用
                #if(defined __linux__)
                    std::string soFileName = *(thread.sp -> opStack[thread.sp -> opStack.size() - 1].string);
                    void* handler = dlopen(soFileName.c_str(), RTLD_LAZY);  // 需要时再加载
                    if(! handler) {
                        error("IMPORT: " + std::string(dlerror()));  // 使用`dlerror`获取详细报错信息
                    }

                    void* driver = dlsym(handler, "driver");
                    if(driver == NULL) {
                        error("IMPORT: driver: " + std::string(dlerror()));
                    }

                    std::vector<std::string> externFNNameTable = *(((NlEDTemplate)driver)());
                    for(auto externFNName : externFNNameTable) {
                        // 出现重名现象立即报错，以防止多个链接库重名难以排查的问题
                        if(thread.externFNTable.count(externFNName)) {
                            error("IMPORT: " + soFileName + ": " + "External function " + externFNName + " already exists");
                        }

                        void* externFN = dlsym(handler, externFNName.c_str());
                        if(externFN == NULL) {
                            error("IMPORT: " + std::string(dlerror()));
                        }

                        thread.externFNTable[externFNName] = externFN;
                    }
                #elif(defined _WIN32 || defined _WIN64)
                #endif
                
                thread.sp -> opStack.pop_back();
                break;
            }

            case EXIT: {
                exit(0);
            }

            case NOP: {
                break;
            }
        }
    }
}