#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main() {
    // map two swap-backed pages
    char *page0 = (char *) vm_map(nullptr, 0);
    char *page1 = (char *) vm_map(nullptr, 0);
    // write the filename into virtual memory
    char *filename01 = page0 + VM_PAGESIZE - 4;
    strcpy(filename01, "12345678");
    cout << filename01 << endl;
    
    // map a file-backed page
    char *page2 = (char *) vm_map(nullptr, 0);
    char *page3 = (char *) vm_map(nullptr, 0);
    char *filename23 = page2 + VM_PAGESIZE - 4;
    strcpy(filename23, "asdfghjk");
    cout << filename23 << endl;

    char *page4 = (char *) vm_map(nullptr, 0);
    char *page5 = (char *) vm_map(nullptr, 0);
    char *filename45 = page4 + VM_PAGESIZE - 4;
    strcpy(filename45, "[][][][]");
    cout << filename45 << endl;

    filename01[3] = '!';
    cout << filename01 << endl;

    cout << "end of test 3" << endl;
}
