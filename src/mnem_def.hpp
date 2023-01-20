/*
 * @Author: CBH37
 * @Date: 2023-01-09 21:17:01
 * @Description: nl汇编助记符定义
 */
#pragma once

#define MNEM_GROUP \
    DEF_X(LOAD_LOCAL)   \
    DEF_X(LOAD_GLOBAL)  \
    DEF_X(LOAD_NUM) \
    DEF_X(LOAD_STRING)  \
    DEF_X(LOAD_ADDR)    \
    DEF_X(STORE_LOCAL)  \
    DEF_X(STORE_GLOBAL) \
    \
    DEF_X(ADD)  \
    DEF_X(SUB)  \
    DEF_X(MUL)  \
    DEF_X(DIV)  \
    DEF_X(MOD)  \
    DEF_X(POW)  \
    \
    DEF_X(NOT)  \
    DEF_X(COMPARE)  \
    \
    DEF_X(JMP)  \
    DEF_X(JMPC) \
    DEF_X(CALL) \
    DEF_X(CALLE)    \
    DEF_X(RET)  \
    \
    DEF_X(MAKE_LIST)    \
    DEF_X(ACTION_LIST)  \
    DEF_X(MAKE_MAP) \
    DEF_X(ACTION_MAP)   \
    \
    DEF_X(POP_TOP)  \
    DEF_X(IMPORT)   \
    DEF_X(EXIT) \
    DEF_X(NOP)

#define DEF_X(x) x,
enum Mnem {
    MNEM_GROUP
};
#undef DEF_X