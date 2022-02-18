#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "vm_app.h"

using std::cout;
using std::endl;
using std::vector;

// swap back size 16

int main(){
    vector<char *> swap_pages(16);
    vector<char *> fileback_pages(16);

    for (size_t i = 0; i < 7; i++) swap_pages[i] = (char *)vm_map(nullptr, 0);
    for (size_t i = 6; i > 0; i--) swap_pages[i][i + 3] = i + 'a';
    for (size_t i = 7; i < 10; i++) swap_pages[i] = (char *)vm_map(nullptr, 0);
    for (size_t i = 7; i < 10; i++) swap_pages[i][i + 3] = i + 'a';
}