#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "vm_app.h"

using std::cout;
using std::endl;
using std::vector;

// swap back size 4
int main() { 
    char* s0 = (char *)vm_map(nullptr, 0);
    char* s1 = (char *)vm_map(nullptr, 0);
    strcpy(s0 + 5, "lampson83.txt");
    char* p0 = (char*)vm_map(s0 + 5, 1);
    if (fork()){
        char* p1 = (char *)vm_map(s0 + 6, 0);
        cout << s1[12] << endl;
        strcpy(p0 + VM_PAGESIZE - 3, "asdf ");
        p1[7] = 'a';
    } else {
        char* p2 = (char*)vm_map(s0 + 6, 0);
        cout << p2[5] << endl;
    }
}