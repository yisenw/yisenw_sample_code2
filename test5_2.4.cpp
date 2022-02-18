#include <unistd.h>
#include <iostream>
#include <string.h>

#include "vm_app.h"

using std::cout;
using std::endl;
int main() {
    if (fork()){
        char* p0 = (char*)vm_map(nullptr, 10);
        strcpy(p0+10, "lampson83.txt");
        char* f0 = (char*)vm_map(p0+10, 0);
        vm_yield();
        f0[11] = 'a';
        cout << f0[11] << endl;
    } else {
        char* p0 = (char*)vm_map(nullptr, 10);
        strcpy(p0 + 15, "lampson83.txt");
        char* f1 = (char*)vm_map(p0+15, 0);
        cout << f1[10] << endl;
    }
}