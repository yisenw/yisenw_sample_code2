#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

#include "vm_app.h"

using std::cout;
using std::endl;


int main(){
    // 7 swap pages
    // 6 maps to zero page, 1 maps to another place
    char *swap1 = (char *)vm_map(nullptr, 0);
    char *swap2 = (char *)vm_map(nullptr, 0);
    char *swap3 = (char *)vm_map(nullptr, 0);
    char *swap4 = (char *)vm_map(nullptr, 0);
    char *swap5 = (char *)vm_map(nullptr, 0);
    char *swap6 = (char *)vm_map(nullptr, 0);
    char *swap7 = (char *)vm_map(nullptr, 0);
    swap1[0] = '1';  // ppn 1
    swap1[1] = 'c';
    swap2[0] = '2';  // ppn 2
    swap3[0] = '3'; // ppn 3
    swap4[0] = '4'; // evict swap 1 to swap_back_slot 0, ppn 1
    if (fork()){
        swap1[1] = 'p'; // restore parent to ppn 2. (evict swap2 to slot 1.) copy swap1 to ppn 3 (evict swap3 to slot 2. )
        /* a lot of access to evict both pages */
        swap5[0] = '5';
        swap6[0] = '6';
        swap1[2] = 'q';
    } else {
        swap5[0] = '5';
        swap6[0] = '6';
        swap7[0] = '7';
        
        /* get back swap1 child ediction */
        cout << swap1[0] << endl;
        cout << "something" << endl;
    }
}