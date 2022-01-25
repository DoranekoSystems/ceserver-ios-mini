
#include <stdint.h>
#include <sys/types.h>

#define CMD_GETVERSION 0
#define CMD_CLOSECONNECTION 1
#define CMD_TERMINATESERVER 2
#define CMD_OPENPROCESS 3
#define CMD_CREATETOOLHELP32SNAPSHOT 4
#define CMD_PROCESS32FIRST 5
#define CMD_PROCESS32NEXT 6
#define CMD_CLOSEHANDLE 7
#define CMD_VIRTUALQUERYEX 8
#define CMD_READPROCESSMEMORY 9
#define CMD_WRITEPROCESSMEMORY 10
#define CMD_STARTDEBUG 11
#define CMD_STOPDEBUG 12
#define CMD_WAITFORDEBUGEVENT 13
#define CMD_CONTINUEFROMDEBUGEVENT 14
#define CMD_SETBREAKPOINT 15
#define CMD_REMOVEBREAKPOINT 16
#define CMD_SUSPENDTHREAD 17
#define CMD_RESUMETHREAD 18
#define CMD_GETTHREADCONTEXT 19
#define CMD_SETTHREADCONTEXT 20
#define CMD_GETARCHITECTURE 21
#define CMD_MODULE32FIRST 22
#define CMD_MODULE32NEXT 23

#define CMD_GETSYMBOLLISTFROMFILE 24
#define CMD_LOADEXTENSION         25

#define CMD_ALLOC                   26
#define CMD_FREE                    27
#define CMD_CREATETHREAD            28
#define CMD_LOADMODULE              29
#define CMD_SPEEDHACK_SETSPEED      30

#define CMD_VIRTUALQUERYEXFULL      31
#define CMD_GETREGIONINFO           32
#define CMD_GETABI                  33

#define CMD_AOBSCAN					200

//just in case I ever get over 255 commands this value will be reserved for a secondary command list (FF 00 -  FF 01 - ... - FF FE - FF FF 01 - FF FF 02 - .....
#define CMD_COMMANDLIST2            255

#pragma pack(1)
typedef struct {
  uint32_t handle;
  uint64_t address;
  uint32_t size;
  uint8_t  compress;
} CeReadProcessMemoryInput, *PCeReadProcessMemoryInput;

typedef struct {
  int read;
} CeReadProcessMemoryOutput, *PCeReadProcessMemoryOutput;
#pragma pack()