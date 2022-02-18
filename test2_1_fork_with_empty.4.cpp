#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main() {
    if (fork()) {
        // This is the parent!
        cout << "parent starts." << endl;
        char *page0 = (char *) vm_map(nullptr, 0);
        page0[0] = 'a';  // Copy-on-write
        cout << "parent yields." << endl;
        vm_yield();  // Let the child run
        cout << "parent ends." << endl;
    } else {
        // This is the child!
        cout << "child starts and ends." << endl;
    }
}
