#include <unistd.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <cassert>
#include "vm_app.h"

using std::cout;
using std::endl;

int main() {
    /* Allocate swap-backed page from the arena */
    cout << "start" << endl;
    char *pages[1000];

    for (int i = 0; i < 1000; i++) {
        pages[i] = (char *) vm_map(nullptr, 0);
        assert(pages[i]);
        cout << "Num: " << i << "completed." << endl;
    }

    cout << "should not print" << endl;
    assert(0);

    
}