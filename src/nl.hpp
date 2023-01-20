/*
 * @Author: CBH37
 * @Date: 2023-01-16 15:51:34
 * @Description: 为了实现外部函数功能需要传给外部函数类型为`Nlthread`的参数，为了使参数中含有`Nlthread`类型的外部函数顺利通过编译
                 将有关`Nlthread`类的提出来像`Python.h`一样提供给外部函数供它们引用
 */
#pragma once

#include <vector>
#include <map>

/*
 * 为了方便实现，`nl`没有多线程功能，唯一的线程由栈、外部函数表、全局变量表和`SP`指针组成
 * 栈由多个栈帧组成，栈帧由局部变量表、操作数栈（类JVM）组成
 */

// 延续`Python VM`传统，将值的结构称为`xxObject`
enum Type {
    STRING, NUM, POINTER,
};

struct NlObject {
    Type type;
    union {
        std::string* string;    // `union`联合体中不能出现非平凡类型，所以只能使用指针引用，且为了方便操作使用`string`而非`char*`
        long double num;     // 为了不多余引进对`nlc`文件格式的定义头文件，将`NlcFile::Num`替换为其原值
        void* pointer;    // `void*`用于其他特殊类型
    };
};

struct StackFrame {
    size_t returnAddress;   // 记录返回地址以便`RET`
    std::map<size_t, NlObject> localVarTable;   // 局部变量表，因为变量名都在代码中出现过，因此都有对应的`size_t`类型的`id`（变量名必须为字符串，否则出现例外概不负责）
    std::vector<NlObject> opStack;  // 操作数栈，由多个值组成
};

// 为了方便外部函数获取虚拟机整体信息及以后实现多线程，特定义线程类
struct Nlthread {
    StackFrame* sp;    // 方便写代码而设定
    std::map<std::string, void*> externFNTable; // 外部函数表
    std::map<size_t, NlObject> globalVarTable;  // 全局变量表
    std::vector<StackFrame> stack;  // 函数栈
};

// 虚拟机中复杂数据类型实际类型的定义，为了区别于其他普通类型，统一命名为`xxxObject`
using ListObject = std::vector<NlObject>;   // nl汇编中的`list`指的就是长度可以伸缩的数组，为了速度使用`vector`
using MapObject = std::map<std::string, NlObject>;  // `map`的`key`为了实现简便只能为字符串，前端可以将多种类型的`key`化为字符串类型传入后端以实现多种类型的`key`

// 为了不引起一些不必要的麻烦和节省内存空间，在传参和返回值时统一使用指针
typedef std::vector<std::string>*(*NlEDTemplate)(void);    // NlExternDriverTemplate 外部驱动函数模板
typedef NlObject*(*NlEFNTemplate)(Nlthread*, ListObject*);   // NlExternFunctionTemplate 外部函数模板
