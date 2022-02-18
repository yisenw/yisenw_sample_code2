#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <cassert>
#include "vm_app.h"

using std::cout;
using std::endl;


int main()
{
    /* Allocate swap-backed page from the arena */
    char *page0 = (char *) vm_map(nullptr, 0);
    char *page0_5 = (char *) vm_map(nullptr, 0);

    char *filename = page0 + VM_PAGESIZE - 2;

    /* Write the name of the file that will be mapped */
    strcpy(filename, "can_not_find.bin");
    // cout << "file name received: " << filename << endl;

    char* p = (char *) vm_map (filename, 0);

    p[0] = '!';

    cout << "read file successfull." << endl; // should not print

    cout << "end 4_1" << endl;

}