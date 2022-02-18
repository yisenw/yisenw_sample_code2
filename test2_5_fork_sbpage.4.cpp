#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <cassert>
#include <vector>

using std::cout;
using std::endl;

int main() { /* 4 pages of physical memory in the system */
    char* page2 =  (char *) vm_map(nullptr, 0);
    char* page2_5 =  (char *) vm_map(nullptr, 0);
    char* sb_page_ptr = page2 + VM_PAGESIZE - 4;
    strcpy(sb_page_ptr, "12345678");
    if (fork()) {
        cout << "parent begins" << endl;
        sb_page_ptr[3] = '?';
        sb_page_ptr[6] = '?';
        cout << sb_page_ptr << " parent yields" << endl;
        vm_yield();
        assert(sb_page_ptr[4] == '?');
        cout << "parent_2 ends." << endl;
    }
    else {
        cout << "child_2 begins " << sb_page_ptr << endl;
        assert(sb_page_ptr[3] == sb_page_ptr[6]);
        sb_page_ptr[4] = '?';
        cout << sb_page_ptr << " child_2 ends" << endl;
    }


}

