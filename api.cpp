#include "api.h"

extern "C" kern_return_t mach_vm_read_overwrite(vm_map_t, mach_vm_address_t, mach_vm_size_t,
mach_vm_address_t, mach_vm_size_t*);

mach_port_t kernel_task_port;

int OpenProcess(int pid)
{
    task_for_pid(mach_task_self(), pid, &kernel_task_port);
    return pid;
}

int ReadProcessMemory(int hProcess, void *lpAddress, void *buffer, int size)
{
    mach_vm_size_t size_out;
	kern_return_t kr = mach_vm_read_overwrite(kernel_task_port, (mach_vm_address_t)lpAddress, size,(mach_vm_address_t)buffer, &size_out);
    if(kr != 0){
        return 0;
    }
    return size_out;
}