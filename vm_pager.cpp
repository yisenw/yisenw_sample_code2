#include "vm_pager_class.h"
#include <iostream>

using std::vector;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

pager_class* pager;

void vm_init(unsigned int memory_pages, unsigned int swap_blocks){
    pager = new pager_class(memory_pages, swap_blocks);
#ifdef DEBUG
    log_st << std::hex;
    log_st<<"\n>>VM_INIT is called with " << memory_pages << "," << swap_blocks << endl;
    pager->print_ppt();
    pager->print_sst();
#endif
}

int vm_create(pid_t parent_pid, pid_t child_pid) {
#ifdef DEBUG
    log_st << "\n>>VM CREATE " << parent_pid << "," << child_pid <<endl;
    pager->print_ppt();
    pager->print_sst();
#endif
    return pager->fork(parent_pid, child_pid);
}

void *vm_map(const char *filename, unsigned int block) {
#ifdef DEBUG
    log_st << "\n>>VM_MAP calls with nullptr: " << (filename == nullptr) << " block " << block << endl;
#endif
    // swap-backed: map to zero page
    if (filename == nullptr) {
        unsigned int vpn = pager->alloc_swap_page();
        #ifdef DEBUG
        pager->print_vpt(pager->current_pid);
        pager->print_ppt();
        pager->print_sst();
        #endif
        if (vpn == UINT_MAX) {
            #ifdef DEBUG
            log_st << "Map returns NULL" << endl;
            #endif
            return nullptr;
        }
        else 
            return (void *) (vpn*VM_PAGESIZE + (uintptr_t)VM_ARENA_BASEADDR);
    }
    else {
        if (pager->arena_full()) return nullptr;
        string filename_str;
        const char * read_head = filename;
        while (true) {
            // vpn index into the page tagle
            unsigned int vpn = get_vpn_index(read_head); 
            // exceed arena
            if (vpn == UINT_MAX) return nullptr;
            page_table_entry_t & temp_pte = page_table_base_register->ptes[vpn];
            // if the page is not readable, call the fault_handler
            if (!temp_pte.read_enable) {
                int fault_handler_result = pager->fault_handler(vpn, false);
                // unable to handle the read request, fail
                if (fault_handler_result == -1)
                    return nullptr;
            }
            // physical_addr = ppage + offset
            unsigned int physical_addr = (temp_pte.ppage << 16) + get_vpn_offset(read_head);
            if (((char*)vm_physmem)[physical_addr] == '\0') break;
            filename_str += ((char*)vm_physmem)[physical_addr];
            // increase read_head by 1 byte
            read_head++;
        }
        #ifdef DEBUG
        log_st << "File-backed filename: " << filename_str << endl;
        #endif
        unsigned int vpn = pager->alloc_fileback_page(filename_str, block);
        if (vpn == UINT_MAX) {
            #ifdef DEBUG
            log_st << "Map returns NULL" << endl;
            #endif
            return nullptr;
        }
        else 
            return (void *) (vpn*VM_PAGESIZE + (uintptr_t)VM_ARENA_BASEADDR);
    }
    
}

int vm_fault(const void* addr, bool write_flag) {
    #ifdef DEBUG
        log_st << "\n>>VM FAULT called with write?" << write_flag << " addr " << std::hex << (uintptr_t) addr << endl;
    #endif
    unsigned int vpn = get_vpn_index(addr);
    if (vpn == UINT_MAX) return -1;

    return pager->fault_handler(vpn, write_flag);

}

void vm_switch(pid_t pid){
#ifdef DEBUG
        log_st
    << "VM SWITCH: from " << pager->current_pid << " to " << pid << endl;
#endif
    pager->switch_process(pid);
}

void vm_destroy() {
#ifdef DEBUG
    log_st << ">>VM_DESTROY" << endl;
#endif
    pager->destroy_current_process();
}