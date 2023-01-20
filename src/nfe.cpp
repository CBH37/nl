/*
 * @Author: CBH37
 * @Date: 2023-01-17 17:14:21
 * @Description: Nl编译器前端（Nl compiler's Front End）
 */
#include "nfe.hpp"

Nfe::Nfe(std::string input) {
    src = input;
    parser();
}

/************ tokenizer 部分 ************/
// `token`可能来源于连接符、运算符、关键字、数字、字符串、符号六部分
void Nfe::getToken() {
    // 初始化
    token = tok_other;
    tokVal = "";
    while(true) {
        // 单行注释
        if(index <= src.length() - 2 
        && src[index] == '/' && src[index + 1] == '/') {
            index += 2;
            while(index < src.length() && src[index] != '\n') {
                index ++;
            }

            index ++;
        }

        // 多行注释
        if(index <= src.length() - 2
        && src[index] == '/' && src[index + 1] == '*') {
            index += 2;
            while(index <= src.length() - 2
               && src[index] != '*' && src[index + 1] != '/') {
                index ++;
            }

            index += 2;
        }

        // 因为处理注释后不会返回还会继续处理出其他`Token`，为了不产生范围溢出，所以判断是否到达文件末尾的代码必须在处理注释的代码之后
        if(index >= src.length()) {
            token = tok_eof;
            return;
        }

        // 处理数字（支持二、八、十六进制）
        if(isdigit(src[index])
        || src[index] == '+'
        || src[index] == '-'
        || (src[index] == '.' && index <= src.length() - 2 && isdigit(src[index + 1]))) {
            // 1. symbol
            if(src[index] == '+' || src[index] == '-') {
                tokVal += src[index];
                index ++;
            }

            // 2. 处理二、八、十六进制
            int base = 10;
            if(src[index] == '0') {
                tokVal += src[index];
                index ++;

                if(src[index] == 'b') {
                    base = 2;
                } else if(src[index] == 'x') {
                    base = 16;
                } else {
                    base = 8;
                }

                tokVal += src[index];
                index ++;
            }

            // 3. 处理数字
            bool hasDot = false;
            while(index < src.length()
               && ((base == 16 && isxdigit(src[index]))
               || (src[index] >= '0' && src[index] <= ('0' + base - 1))
               || src[index] == '.')) {
                if(src[index] == '.') {
                    if(hasDot) {
                        error("number can only have one decimal point");
                    }

                    hasDot = true;
                }

                tokVal += src[index];
                index ++;
            }

            if(base != 10 && tokVal.length() == 2) {
                error("unexpected number:" + tokVal);
            }

            // 5. 处理指数
            if(index <= src.length() - 2 && tolower(src[index]) == 'e') {
                tokVal += src[index];
                index ++;

                while(index < src.length() && isdigit(src[index])) {
                    tokVal += src[index];
                    index ++;
                }
            }

            token = tok_num;
            return;
        }

        // 处理字符串（默认为多行不转义字符串）
        if(src[index] == '"') {
            index ++;
            while(src[index] != '"') {
                if(src[index] == '\\') {
                    index ++;
                }

                tokVal += src[index];
                index ++;
            }

            index ++;
            token = tok_str;
            return;
        }

        // 处理符号、关键字
        if(isalpha(src[index]) || src[index] == '_') {
            while(isalpha(src[index])
              || isdigit(src[index])
              || src[index] == '_') {
                tokVal += src[index];
                index ++;
            }

            if(keywordTable.count(tokVal)) {
                return;
            }

            token = tok_ident;
            return;
        }

        if(isspace(src[index])) {
            index ++;
        } else {
            // 通过贪心算法处理连接符和运算符
            // 在有可能符合条件的非空字符区间不断获取最新的当前最长的符合条件的作为结果
            // 从而实现最优解解决分割`token`时出现的包含较短的较长符号被分割为较短的和去除较短符号的较长符号两部分的问题
            std::string target = "";
            while(! isspace(src[index])) {
                tokVal += src[index];
                if(connectSymbolTable.count(tokVal) 
                || binaryOpTable.count(tokVal)
                || unaryOpTable.count(tokVal)) {
                    target = tokVal;
                }

                index ++;
            }

            // 目标与初始化值相同说明没有一个符合条件的，说明这些字符都是非法字符，直接报错
            if(target == "") {
                error("unexpected token: " + tokVal);
            }

            // 因为最优解是最终结果，所以将索引设置到最优解之后处，使下次解析于最终结果之后
            index = index - tokVal.length() + target.length();
            tokVal = target;
            return;
        }
    } 
}

/************ Expr 部分 ************/
std::shared_ptr<Nfe::ExprAST> Nfe::parseNumberExpr() {
    NlcFile::Num number = std::stold(tokVal);
    getToken(); // eat number

    return std::make_shared<NumberExprAST>(number);
}

std::shared_ptr<Nfe::ExprAST> Nfe::parseStringExpr() {
    std::string string = tokVal;
    getToken(); // eat string

    return std::make_shared<StringExprAST>(string);
}

/*
 * computedMemberAccess ::= [ expr ]
 * memberAccess ::= .IDENT
 * callFunction ::= ( expr* )
 *
 * accessExpr ::= computedMemberAccess | memberAccess | callFunction
 * parseIdentExpr ::= IDENT (accessExpr)*
 */

// TODO: 实现符号表
std::shared_ptr<Nfe::ExprAST> Nfe::parseIdentExpr() {
    std::string symbol = tokVal;
    getToken(); // eat symbol

    // 因为函数可能来自于`Map`/ `List`或类，`Map`/ `List`或类也可能来自于函数，所以`Map`/ `List`和类访问对象的操作与调用函数的操作可能会彼此交织在一起
    // 因此`parseSymbolExpr`所解析的就是由最开始的符号名和由访问`Map`/ `List`/ 类的成员或调用函数的操作序列组成（纯`symbol`除外）
    // 而这二者的`AST`结构都是由得出对象的`Map`/ `List`/ 类或函数的`AST`和该对象中需要访问的成员名组成，因此构成的`AST`呈现出`IDENT (accessExpr(accessExpr(...)))`的嵌套结构，即我们所需要的
    // 而不管是嵌套结构还是纯`symbol`都由`IDENT`开始或组成，因此先从构建`Symbol`开始
    std::shared_ptr<ExprAST> target =  std::make_shared<IdentExprAST>(symbol);
    while(tokVal != "[" && tokVal != "." && tokVal != "(") {
        if(tokVal == "(") {
            // 调用函数
            getToken(); // eat `(`

            std::vector<std::shared_ptr<ExprAST>> args;
            while(tokVal != ")") {
                args.push_back(expr());

                if(tokVal != ")" && tokVal != ",") {
                    error("function parameters end with `,` or `)`");
                }

                getToken(); // eat `)` or `,`
            }

            target = std::make_shared<CallExprAST>(target, args);  // 构建嵌套结构
        } else if(tokVal == "[") {
            getToken(); // eat `[`

            target = std::make_shared<AccessExprAST>(target, expr());
            if(tokVal != "]") {
                error("`[` and `]` must appear in pairs");
            }

            getToken(); // eat `]`
        } else {
            getToken(); // eat `.`

            if(token != tok_ident) {
                error("`.` must be followed by a symbol");
            }

            target =  std::make_shared<AccessExprAST>(target, expr());
        }
    }

    return target;
}

/*
 * parseValueExpr ::= NUM
 *                ::= STRING
 *                ::= SYMBOL ...
 *                ::= list_def
 *                ::= map_def
 *                ::= unary_op parseValueExpr
 */
std::shared_ptr<Nfe::ExprAST> Nfe::parseValueExpr() {
    switch(token) {
        case tok_num: {
            return parseNumberExpr();
        }

        case tok_str: {
            return parseStringExpr();
        }

        case tok_ident: {
            return parseIdentExpr();
        }

        case tok_other: {
            // ( expr )
            if(tokVal == "(") {
                getToken(); // eat `(`
                std::shared_ptr<ExprAST> value = expr();

                if(tokVal != ")") {
                    error("`(` and `)` must appear in pairs");
                }

                getToken(); // eat `)`
                return value;
            } else if(unaryOpTable.count(tokVal)) {
                return std::make_shared<UnaryExprAST>(tokVal, parseValueExpr());
            }
        }
    }

    return nullptr; // 为了解除编译器`non-void`错误
}

/*
 * expr ::= parseValueExpr
 *      ::= parseValueExpr (binary_op parseValueExpr)*
 */
std::shared_ptr<Nfe::ExprAST> Nfe::expr() {
    std::shared_ptr<ExprAST> value = parseValueExpr();
    
    // 二元运算中不能有`list`或`map`（没有`map` * `map`或是`list` * `list`之类的操作），所以`list`和`map`只能作为普通的值直接返回
    // 上一步获得值，如果是二元运算的话，第二步应该是操作符，所以如果第二步不是操作数，则不属于二元运算，只是普通的值，直接返回即可
    if(! binaryOpTable.count(tokVal)
      || value -> exprType == expr_list
      || value -> exprType == expr_map) {
        return value;
    }

    std::vector<std::string> opStack;   // 操作符栈
    std::vector<std::shared_ptr<ExprAST>> valueStack;   // 操作数栈
    opStack.push_back(tokVal);
    valueStack.push_back(value);

    // (op opValue)*
    while(token != tok_eof
    && (! connectSymbolTable.count(tokVal))
    && (! keywordTable.count(tokVal))) {
        if(! binaryOpTable.count(tokVal)) {
            error("input expression error");
        }

        // 必须有两个及以上的值才能开始运算
        if(valueStack.size() >= 2) {
            // 若栈末尾操作符的优先级大于等于当前操作符的优先级，说明栈末尾操作符所对应的运算需要及时处理，因此要一直计算至栈末尾操作符的优先级小于当前操作符不需要处理为止
            while(binaryOpTable[opStack[opStack.size() - 1]] >= binaryOpTable[tokVal]) {
                valueStack[valueStack.size() - 2] = std::make_shared<BinaryExprAST>(opStack[opStack.size() - 1],
                                                                                    valueStack[valueStack.size() - 2],
                                                                                    valueStack[valueStack.size() - 1]);
                                                                                            
                valueStack.pop_back();
                opStack.pop_back();
            }

            opStack.push_back(tokVal);
        }

        valueStack.push_back(parseValueExpr());

        // 二元运算中永远不能出现`map`和`list`
        if(value -> exprType == expr_list || value -> exprType == expr_map) {
            error("map or list cannot appear in binary operation");
        }
    }

    // 由前面的代码可得知：若栈末尾操作符的优先级大于等于当前操作符的优先级就立即处理
    // 但如果栈末尾操作符的优先级小于当前操作符的优先级这样的情况一直持续到结束，就会因为迟迟得到不处理导致解析结束操作符栈内还有操作符的情况
    // 这种情况意味着操作符栈中的操作符的优先级由开头到末尾递增，按照优先级越大越先处理的原则，自末尾至开头依序不断处理直到操作符栈被清空即可
    while(opStack.size() > 0) {
        valueStack[valueStack.size() - 2] = std::make_shared<BinaryExprAST>(opStack[opStack.size() - 1],
                                                                            valueStack[valueStack.size() - 2],
                                                                            valueStack[valueStack.size() - 1]);

        valueStack.pop_back();
        opStack.pop_back();
    }

    // 计算一个算式最终会得到一个结果，同理处理二元运算最终也只能得到一个结果，将其返回即可
    return valueStack[0];
}

std::string Nfe::NumberExprAST::codegen() {
    return std::to_string(num);
}

std::string Nfe::StringExprAST::codegen() {
    return string;
}

std::string Nfe::IdentExprAST::codegen() {
    return ident;
}

std::string Nfe::UnaryExprAST::codegen() {
}

std::string Nfe::BinaryExprAST::codegen() {

}

std::string Nfe::ListExprAST::codegen() {

}

std::string Nfe::MapExprAST::codegen() {

}

std::string Nfe::CallExprAST::codegen() {

}

std::string Nfe::AccessExprAST::codegen() {

}

/************ Parser 部分 ************/
void Nfe::insertSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> message) {
    symbolTable[symbolTable.size() - 1][symbol] = message;  // `nl`支持多次定义变量或常量，因此不用判断符号是否存在
}

void Nfe::insertSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> message, std::shared_ptr<Nfe::SymbolMessage> father) {
    father -> subSymbolTable[symbol] = message;
}

Nfe::SymbolMessage Nfe::getSymbol(std::string symbol) {
    // 自内而外横向搜索
    for(int i = symbolTable.size() - 1; i >= 0; i --) {
        if(symbolTable[i].count(symbol)) {
            return *(symbolTable[i][symbol]);
        }
    }

    error("use undefined symbols"); // 没找到符号直接报错
}

std::shared_ptr<Nfe::SymbolMessage> getSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> father) {
    if(! (father -> subSymbolTable.count(symbol))) {
        error("use undefined symbols");
    }

    return father -> subSymbolTable[symbol];
}

// let IDENT: TYPE (= VALUE);
std::shared_ptr<Nfe::BaseAST> Nfe::parseLet() {
    getToken(); // eat `let`

    if(token != tok_ident) {
        error("Let must be followed by a name");
    }

    std::string symbol = tokVal;
    getToken(); // eat IDENT

    if(tokVal != ":") {
        error("the type of variable must be specified");
    }
    getToken(); // eat ':'

    if(! typeTable.count(tokVal)) {
        error("type " + tokVal + " does not exist");
    }
    insertSymbol(symbol, std::make_shared<SymbolMessage>(tokVal));
    getToken(); // eat TYPE

    // 2023/ 1/ 20 TODO: 将符号表改为抽象类+（不同类 + 类type）形式
    // 同时完成`map/ list/ fn`的解析
    // 如果`let`语句只有声明没有定义，根据其值赋予初始值
    // 同时实现类型推导系统，如果`let`语句有声明也有定义，则判断赋予其的值是否能被其数据类型所接受
    if(tokVal == ";") {
        
    }
}

void Nfe::parser() {
    
}

std::string Nfe::LetAST::codegen() {

}

std::string Nfe::BreakAST::codegen() {

}

std::string Nfe::ContinueAST::codegen() {
    
}

/************ 代码生成部分 ************/
std::string Nfe::codegen() {

}