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
    strcpy(page0 + strlen(page0), "12345678");
    strcpy(page0 + strlen(page0), "12345678");
    cout << "end of cpy 2" << endl;

    cout << page0 << endl;
//     printf("%s", page0);

    int *a_num = (int *)vm_map(nullptr, 0);
    *a_num = 100;
    cout << "end of int" << endl;
    // *(a_num + sizeof(int)) = 200;

    // int *a_num + sizeof(int) = (int *) vm_map(nullptr, 0);
}