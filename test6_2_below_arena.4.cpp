#include <unistd.h>

#include <cstring>
#include <iostream>

#include "vm_app.h"

using std::cout;
using std::endl;

int main() {
    /* Allocate swap-backed page from the arena */
    cout << "start" << endl;
    char *page0 = (char *)vm_map(nullptr, 0);

    cout << "end of vm_map" << endl;
    /* Write the name of the file that will be mapped */

    strcpy(page0, "12345678");
    cout << "end of cpy 1" << endl;
    
    cout << "Illegal Map (read out of arena)" << endl;
    cout << "Addr: " << (uintptr_t)page0 + VM_ARENA_SIZE + 1 << endl;
    cout << "Arena upper bound: " << (uintptr_t)VM_ARENA_BASEADDR + VM_ARENA_SIZE << endl;

    // This line will cause app seg fault
    //cout << (char*) (page0 + VM_ARENA_SIZE + 1) << endl;

    // But this line will not
    char *page1 = (char *)vm_map((char *)VM_ARENA_BASEADDR - 2, 0);

    cout << "Map returns: " << (uintptr_t) page1 << endl;

    cout << "Try to read file" << endl;
    cout << page1[0];
    cout << page1[1];

    cout << "END" << endl;

    
}