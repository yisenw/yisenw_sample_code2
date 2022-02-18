#include "../vm_pager_class.h"
#include <iostream>

using namespace std;

#define MEM_P 3
#define SLOT_B 10

void* const vm_physmem = new char[VM_PAGESIZE * MEM_P];
page_table_t* page_table_base_register;


int file_read(const char* filename, unsigned int block, void* buf){
}

int file_write(const char* filename, unsigned int block, const void* buf){
    if (filename)
    cout << "======== write to " << filename << ":" << block
         << "========" << endl;
}

int main() {
    pager_class pager(MEM_P, SLOT_B);
    pid_t p0 = 123;
    pager.fork(0x001, p0);
    pid_t p1 = 321;
    pager.fork(0x001, p1);
    pager.switch_process(p0);
    pager.alloc_swap_page();
    pager.alloc_swap_page();
    pager.print_vpt(p0);

    pager.switch_process(p1);
    pager.print_vpt(p1);
    pager.alloc_swap_page();
    pager.alloc_swap_page();
    pager.print_vpt(p1);

    pager.switch_process(p0);
    pager.alloc_swap_page();
    pager.alloc_swap_page();
    pager.print_vpt(p0);

    pager.fault_handler(1, false);
    /* write to a shared page */
    pager.fault_handler(2, true);
    cout << "write a shared page" << endl;
    pager.print_vpt(p0);
    pager.print_ppt();

    cout << "write another shared page" << endl;
    pager.switch_process(p1);
    pager.fault_handler(0, false);
    pager.fault_handler(0, true);
    pager.print_vpt(p1);
    pager.print_ppt();

    cout << "write another page to 321" << endl;
    pager.fault_handler(1, true);
    pager.print_vpt(p1);
    pager.print_vpt(p0);
    pager.print_ppt();
    pager.print_sst();

    cout << "access 123 2" << endl;
    pager.switch_process(p0);
    pager.fault_handler(2, false);
    pager.print_vpt(p1);
    pager.print_vpt(p0);
    pager.print_ppt();
    pager.print_sst();

    cout << "destroy 321" << endl;
    /* destroy p1 */
    pager.destroy_current_process();
    pager.print_ppt();
}