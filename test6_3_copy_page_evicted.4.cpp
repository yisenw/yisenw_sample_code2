#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "vm_app.h"

using std::cout;
using std::endl;

int main() { 
    char* p0 = (char*)vm_map(nullptr, 0); // shared swap back
    strcpy(p0, "data1.bin"); // write: copy_and_change_pointer ppn = 1
    char* f0 = (char*)vm_map(p0, 0);
    cout << "read from f0 " << f0[3] << endl; // read file to ppn = 2
    char* f1 = (char*)vm_map(p0, 1);
    cout << "read from f1" << f1[5] << endl; // read file to ppn = 3
    if (fork()){
        vm_yield();
        cout << "somthing" << endl;
        p0[30] = 'p';
        cout << "parent written " << p0[30] << endl;
    } else {
        p0[20] = 'c';
        vm_yield();
        cout << "just written " << p0[20] << endl;
        cout << "copyed data " << p0 << endl;
    }
}