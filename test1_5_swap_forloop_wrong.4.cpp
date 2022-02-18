#include <iostream> 
#include <cstring> 
#include <unistd.h> 
#include "vm_app.h" 
#include <fstream> 
  
using std::cout; 
using std::ofstream; 
using std::endl;
  
   static const uintptr_t arena_size = 0x01000000; 
  
int main() { 
    /* Allocate swap-backed page from the arena */ 
    char *page0 = (char *) vm_map(nullptr, 0); 
    // char *page1 = (char *) vm_map(nullptr, 0); 
    // char *page2 = (char *) vm_map(nullptr, 0); 

    /* Write the name of the file that will be mapped */ 
    cout << "create pages 3" << endl; 
    for (int i = 0; i < VM_PAGESIZE - 1; i++) { 
        page0[i] = 'b';
    }
    page0[VM_PAGESIZE - 1] = '\0';
    cout << "write ends" << endl;

    for (int turn = 0; turn < 10; turn++) { 
        for (int i = 0; i < strlen(page0); i++) { 
            if (turn % 3 == 0) page0[i]++; 
            else if (turn % 3 == 1) page0[i]--; 
        } 
    }
} 
 
