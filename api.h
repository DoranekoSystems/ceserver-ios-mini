#include <mach/mach.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <stdbool.h>
#include <stdio.h>

int OpenProcess(int pid);
int ReadProcessMemory(int hProcess, void *lpAddress, void *buffer, int size);