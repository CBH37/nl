/*
 * @Author: CBH37
 * @Date: 2023-01-17 17:07:16
 * @Description: nfe的头文件
 */
#pragma once

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <cmath>
#include <memory>

#include "global.hpp"
#include "ndr.hpp"
#include "nlc_def.hpp"

class Nfe {
public:
    Nfe(std::string input);

//private:
    /************ tokenizer 部分 ************/
    size_t index = 0;
    std::string src;

    enum Token {
        tok_num,
        tok_str,
        tok_ident,
        tok_other,  // 可能为连接符、运算符或关键字
        tok_eof,
    };

    int token = - 1;
    std::string tokVal;

    // 连接符
    std::set<std::string> connectSymbolTable = {
        ":", "=", ";", ",",
        "(", ")", "{", "}",
        "=>",
    };

    // 为了防止一发而牵全身且`JS`作为一种成熟的语言该有的运算符应有尽有，所以直接使用`JS`现成的优先级
    // 运算符优先级参考：https://developer.mozilla.org/zh-CN/docs/Web/JavaScript/Reference/Operators/Operator_Precedence
    std::map<std::string, int> binaryOpTable = {
        { "**" , 14 },
        { "*" , 13 }, { "/" , 13 }, { "%" , 13 },
        { "+" , 12 }, { "-" , 12 },
        
        { ">" , 10 }, { ">=" , 10 }, { "<" , 10 }, { "<=" , 10 },
        { "==" , 9 }, { "!=" , 9 },
        { "&&" , 5 }, { "||" , 4 },
    };

    // 一元运算符人人平等所以不需要优先级，直接使用`set`即可
    std::set<std::string> unaryOpTable = {
        "!", "+", "-",
    };

    std::set<std::string> keywordTable = {
        "let", "const",
        "for", "while", "loop",
        "if", "else", "elif", "match",
        "class", "fn",
        "import", "extern",
    };
    void getToken();


    /************ Expr 部分 ************/
    enum ExprType {
        expr_number,
        expr_string,
        expr_ident,
        expr_unary,
        expr_binary,

        expr_list,
        expr_map,
        expr_access,
        expr_call,
    };

    /*
     * `ExprAST`因为含有纯虚函数所以为抽象类无法实例化，因此不能使用`ExprAST xxx`的方式定义一个`ExprAST`的实例
     * 但为了将`xxxExprAST`各类归一化便于返回值及实现多态，必须使用他们的基类`ExprAST`，为了在不生成其实例的基础上使用它，只得使用`ExprAST`指针
     * 为了防止内存泄漏以及便于`delete`，使用智能指针，因为`shared_ptr`和`unique_ptr`都可以实现同样的功能
     * 但使用`shared_ptr`不用担心使用`unique_ptr`时排他性导致的需要不断使用`std::move`将左值转为右值的繁琐，因此使用`shared_ptr`
     */
    class ExprAST {
    public:
        virtual std::string codegen() = 0;
        ExprType exprType;  // 用以明确表达式形式
        std::string valueType;  // 表达式经过类型推导得出其值的类型
    };

    class NumberExprAST : public ExprAST {
        NlcFile::Num num;

    public:
        ExprType exprType = expr_number;
        NumberExprAST(NlcFile::Num _num) : num(_num) {}
        std::string codegen();
    };

    class StringExprAST : public ExprAST {
        std::string string;
    
    public:
        ExprType exprType = expr_string;
        StringExprAST(std::string& _string) : string(_string) {}
        std::string codegen();
    };

    // 名字
    class IdentExprAST : public ExprAST {
        std::string ident;
    
    public:
        ExprType exprType = expr_ident;
        IdentExprAST(std::string& _ident) : ident(_ident) {}
        std::string codegen();
    };

    // 一元运算符
    class UnaryExprAST : public ExprAST {
        std::string op;
        std::shared_ptr<ExprAST> value;
    
    public:
        ExprType exprType = expr_unary;
        UnaryExprAST(std::string& _op, std::shared_ptr<ExprAST> _value)
            : op(_op), value(_value) {}
        std::string codegen();
    };

    // 二元运算符
    class BinaryExprAST : public ExprAST {
        std::string op;
        std::shared_ptr<ExprAST> LHS, RHS;  // Left/ Right Hand Side 左/ 右侧（参考`LLVM`教程中的万花筒语言关于AST的部分代码）
    
    public:
        ExprType exprType = expr_binary;
        BinaryExprAST(std::string& _op, std::shared_ptr<ExprAST> _LHS,
                                        std::shared_ptr<ExprAST> _RHS)
            : op(_op), LHS(_LHS), RHS(_RHS) {}
        std::string codegen(); 
    };

    // 列表
    class ListExprAST : public ExprAST {
        std::vector<std::shared_ptr<ExprAST>> items;
    
    public:
        ExprType exprType = expr_list;
        ListExprAST(std::vector<std::shared_ptr<ExprAST>> _items) : items(_items) {}
        std::string codegen();
    };

    class MapExprAST : public ExprAST {
        std::vector<std::shared_ptr<ExprAST>> keys, values;
    
    public:
        ExprType exprType = expr_map;
        MapExprAST(std::vector<std::shared_ptr<ExprAST>> _keys, std::vector<std::shared_ptr<ExprAST>> _values)
            : keys(_keys), values(values) {}
        std::string codegen();
    };

    // 调用函数
    class CallExprAST : public ExprAST {
        std::shared_ptr<ExprAST> callee; // 被呼叫函数可能来自`Map`/ `List`/ 类中的成员或其他函数调用后的返回值，所以使用`ExprAST`类型
        std::vector<std::shared_ptr<ExprAST>> args;
    
    public:
        ExprType exprType = expr_call;
        CallExprAST(std::shared_ptr<ExprAST> _callee, std::vector<std::shared_ptr<ExprAST>> _args)
            : callee(_callee), args(_args) {}
        std::string codegen();
    };

    // 访问类/ `Map`/ `List`成员/
    // 由于类访问类成员的方式与`Map`和`List`相似，且类最终会映射为`Map`并不会存在语义上的差异，因此三者访问成员对象的代码统一使用`AccessExprAST`来表示
    class AccessExprAST : public ExprAST {
        std::shared_ptr<ExprAST> accessName;    // 同`CallExprAST`
        std::shared_ptr<ExprAST> indexs;

    public:
        ExprType exprType = expr_access;
        AccessExprAST(std::shared_ptr<ExprAST> _accessName, std::shared_ptr<ExprAST> _indexs)
            : accessName(_accessName), indexs(_indexs) {}
        std::string codegen();
    };

    std::shared_ptr<Nfe::ExprAST> parseNumberExpr();
    std::shared_ptr<Nfe::ExprAST> parseStringExpr();
    std::shared_ptr<Nfe::ExprAST> parseIdentExpr();
    std::shared_ptr<Nfe::ExprAST> parseValueExpr();
    std::shared_ptr<Nfe::ExprAST> expr();

    /************ Parser 部分 ************/
    enum StateMentType {
        sm_let,    // 定义变量语句 let IDENT: TYPE (= VALUE);
        sm_const,  // 定义常量语句 const IDENT: TYPE (= VALUE);
        sm_assign, // 赋值语句 IDENT = VALUE

        sm_fn,     // 定义函数语句 fn IDENT(IDENT: TYPE, ...) => TYPE {}

        sm_loop,   // `loop`循环  loop {}
        sm_while,  // `while`循环 while(COND) {}
        sm_for,    // `for`循环   for(INIT_STATEMENT; COND, INCREMENT) {}
        sm_break,
        sm_continue,

        sm_match,  // `match`判断，类似`Rust match`，生成的代码与多层`if...elif...else`相似    match[(COND)] { EXPR => {} ... default => {} }
        sm_if,     // `if`判断    if(COND) {} elif {} else {}

        sm_class,  // 类定义   class IDENT (: IDENT) { let/const IDENT: TYPE; fn IDENT(IDENT: TYPE, ...) => TYPE {} }
    };

    class BaseAST {
        virtual std::string codegen() = 0;
    };

    std::set<std::string> typeTable = {
        "number", "string", "map", "list", "fn",
    };

    struct SymbolMessage {
        bool isConst;
        std::string type;
        std::map<std::string, std::shared_ptr<SymbolMessage>> subSymbolTable; // 虽然`Map`/ `List`或类中各成员的类型是随意的，但为了防止对`Map`/ `List`或类中的成员作不符合其值的类型的操作，仍需将其成员的值的类型记录到符号表中

        SymbolMessage(std::string& _type, bool _isConst = false) : type(_type), isConst(_isConst) {}
    };

    // 因为语句块中可以套语句块，而每个语句块都对应一个专属的作用域，作用域中的符号又要使用符号表进行记录
    // 所以当解析代码时遇到语句块就要创建一个符号表，语句块结束，作用域也就结束，符号表也随之删除，这样的过程类似于后进先出的栈，因此使用`std::vector`实现支持作用域的符号表
    std::vector<std::map<std::string, std::shared_ptr<SymbolMessage>>> symbolTable;
    void insertSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> message);   // 在当前作用域所对应的符号表中插入符号及符号信息
    void insertSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> message, std::shared_ptr<SymbolMessage> father); // 将符号及符号信息插入至父类的子符号表中，适用于将符号插入`Map`/ `List`或类等嵌套结构
    Nfe::SymbolMessage getSymbol(std::string symbol);  // 从当前作用域逐步向外横向搜索期望符号，因为除了重复定义以外，无法修改符号的类型，所以直接返回符号信息的副本即可
    std::shared_ptr<Nfe::SymbolMessage> getSymbol(std::string symbol, std::shared_ptr<Nfe::SymbolMessage> father);    // 在父类的子符号表中查找期望符号，适用于获取`Map`/ `List`或类等嵌套结构中的期望符号
    
    class LetAST {
        std::string name;
        std::shared_ptr<ExprAST> value;

    public:
        LetAST(std::string& _name, std::shared_ptr<ExprAST> _value)
            : name(_name), value(_value) {}
        std::string codegen();
    };

    class BreakAST : public BaseAST {
    public:
        std::string codegen();
    };

    class ContinueAST : public BaseAST {
    public:
        std::string codegen();
    };

    using Block = std::vector<BaseAST>;
    bool isGlobal = true;

    std::shared_ptr<Nfe::BaseAST> parseLet();
    void parser();

    /************ 代码生成部分 ************/
    std::string codegen();
};