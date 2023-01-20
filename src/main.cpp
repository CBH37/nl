/*
 * @Author: CBH37
 * @Date: 2023-01-09 13:48:47
 * @Description: 主程序
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include "nas.hpp"
#include "nvm.hpp"
#include "ndr.hpp"
#include "nfe.hpp"

int main(int argc, char** argv) {
    Nfe nfe("if()=> {}");
    while(true) {
        nfe.getToken();
        if(nfe.token == Nfe::Token::tok_eof) {
            break;
        }

        std::cout << nfe.tokVal << ' ';
    }
    
    return 0;
}