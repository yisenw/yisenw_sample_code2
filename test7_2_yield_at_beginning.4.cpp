#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>
#include <vector>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    char *fb_page = (char *) vm_map(filename, 0);
    
    vm_yield();
    cout << "shoud not? print" << endl;
    if (fork()) {
        cout << "aleilei" << endl;
    }
    if (fork()) {
        vm_yield();
        cout << "shoud not??? print" << endl;
    }
}

