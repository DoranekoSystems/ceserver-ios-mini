#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/select.h>

#include <zlib.h>

#include <errno.h>
#include <signal.h>

#include <thread>

#include "ceserver.h"
#include "api.h"

ssize_t recvall (int s, void *buf, size_t size, int flags)
{
  ssize_t totalreceived=0;
  ssize_t sizeleft=size;
  unsigned char *buffer=(unsigned char*)buf;

  // enter recvall
  flags=flags | MSG_WAITALL;

  while (sizeleft>0)
  {
    ssize_t i=recv(s, &buffer[totalreceived], sizeleft, flags);

    if (i==0)
    {
      printf("recv returned 0\n");
      return i;
    }

    if (i==-1)
    {
      printf("recv returned -1\n");
      if (errno==EINTR)
      {
        printf("errno = EINTR\n");
        i=0;
      }
      else
      {
        printf("Error during recvall: %d. errno=%d\n",(int)i, errno);
        return i; //read error, or disconnected
      }

    }

    totalreceived+=i;
    sizeleft-=i;
  }

  // leave recvall
  return totalreceived;
}

ssize_t sendall (int s, void *buf, size_t size, int flags)
{
  ssize_t totalsent=0;
  ssize_t sizeleft=size;
  unsigned char *buffer=(unsigned char*)buf;

  while (sizeleft>0)
  {
    ssize_t i=send(s, &buffer[totalsent], sizeleft, flags);

    if (i==0)
    {
      return i;
    }

    if (i==-1)
    {
      if (errno==EINTR)
        i=0;
      else
      {
        printf("Error during sendall: %d. errno=%d\n",(int)i, errno);
        return i;
      }
    }

    totalsent+=i;
    sizeleft-=i;
  }

  return totalsent;
}

int DispatchCommand(int currentsocket, unsigned char command)
{

    int r;
    
    //printf("command:%d\n",command);
    switch (command)
    {
        case CMD_OPENPROCESS:
        {
            int pid=0;

            r=recvall(currentsocket, &pid, sizeof(int), MSG_WAITALL);
            if (r>0)
            {
                int processhandle;

                printf("OpenProcess(%d)\n", pid);
                processhandle=OpenProcess(pid);

                printf("processhandle=%d\n", processhandle);
                sendall(currentsocket, &processhandle, sizeof(int), 0);
            }
            else
            {
                printf("Error\n");
                fflush(stdout);
                close(currentsocket);
                return 0;
            }
            break;
        }
        case CMD_READPROCESSMEMORY:
        {
            CeReadProcessMemoryInput c;
            r=recvall(currentsocket, &c, sizeof(c), MSG_WAITALL);
            if (r>0)
            {
                PCeReadProcessMemoryOutput o=NULL;
                o=(PCeReadProcessMemoryOutput)malloc(sizeof(CeReadProcessMemoryOutput)+c.size);
                o->read=ReadProcessMemory((int)c.handle, (void *)(uintptr_t)c.address, &o[1], c.size);
                if (c.compress)
                {
                  //compress the output
                  #define COMPRESS_BLOCKSIZE (64*1024)
                  int i;
                  unsigned char *uncompressed=(unsigned char *)&o[1];
                  uint32_t uncompressedSize=o->read;
                  uint32_t compressedSize=0;
                  int maxBlocks=1+(c.size / COMPRESS_BLOCKSIZE);

                  unsigned char **compressedBlocks=(unsigned char **)malloc(maxBlocks*sizeof(unsigned char *) ); //send in blocks of 64kb and reallocate the pointerblock if there's not enough space
                  int currentBlock=0;

                  z_stream strm;
                  strm.zalloc = Z_NULL;
                  strm.zfree = Z_NULL;
                  strm.opaque = Z_NULL;
                  deflateInit(&strm, c.compress);

                  compressedBlocks[currentBlock]=(unsigned char*)malloc(COMPRESS_BLOCKSIZE);
                  strm.avail_out=COMPRESS_BLOCKSIZE;
                  strm.next_out=compressedBlocks[currentBlock];

                  strm.next_in=uncompressed;
                  strm.avail_in=uncompressedSize;

                  while (strm.avail_in)
                  {
                    r=deflate(&strm, Z_NO_FLUSH);
                    if (r!=Z_OK)
                    {
                      if (r==Z_STREAM_END)
                        break;
                      else
                      {
                        printf("Error while compressing\n");
                        break;
                      }
                    }

                    if (strm.avail_out==0)
                    {
                      //new output block
                      currentBlock++;
                      if (currentBlock>=maxBlocks)
                      {
                        //list was too short, reallocate
                        printf("Need to realloc the pointerlist (p1)\n");

                        maxBlocks*=2;
                        compressedBlocks=(unsigned char**)realloc(compressedBlocks, maxBlocks*sizeof(unsigned char*));
                      }
                      compressedBlocks[currentBlock]=(unsigned char*)malloc(COMPRESS_BLOCKSIZE);
                      strm.avail_out=COMPRESS_BLOCKSIZE;
                      strm.next_out=compressedBlocks[currentBlock];
                    }
                  }
                  // finishing compressiong
                  while (1)
                  {

                    r=deflate(&strm, Z_FINISH);

                    if (r==Z_STREAM_END)
                      break; //done

                    if (r!=Z_OK)
                    {
                      printf("Failure while finishing compression:%d\n", r);
                      break;
                    }

                    if (strm.avail_out==0)
                    {
                      //new output block
                      currentBlock++;
                      if (currentBlock>=maxBlocks)
                      {
                        //list was too short, reallocate
                        printf("Need to realloc the pointerlist (p2)\n");
                        maxBlocks*=2;
                        compressedBlocks=(unsigned char**)realloc(compressedBlocks, maxBlocks*sizeof(unsigned char*));
                      }
                      compressedBlocks[currentBlock]=(unsigned char*)malloc(COMPRESS_BLOCKSIZE);
                      strm.avail_out=COMPRESS_BLOCKSIZE;
                      strm.next_out=compressedBlocks[currentBlock];
                    }
                  }
                  deflateEnd(&strm);

                  compressedSize=strm.total_out;
                  // Sending compressed data
                  #define MSG_MORE 0x10
                  sendall(currentsocket, &uncompressedSize, sizeof(uncompressedSize), MSG_MORE); //followed by the compressed size
                  sendall(currentsocket, &compressedSize, sizeof(compressedSize), MSG_MORE); //the compressed data follows
                  for (i=0; i<=currentBlock; i++)
                  {
                    if (i!=currentBlock)
                      sendall(currentsocket, compressedBlocks[i], COMPRESS_BLOCKSIZE, MSG_MORE);
                    else
                      sendall(currentsocket, compressedBlocks[i], COMPRESS_BLOCKSIZE-strm.avail_out, 0); //last one, flush

                    free(compressedBlocks[i]);
                  }
                  free(compressedBlocks);
                }
                else{           
                  sendall(currentsocket, o, sizeof(CeReadProcessMemoryOutput)+o->read, 0);
                }
                if (o)
                    free(o);
            }
            break;
        }
    }
    return 1;
}

int newconnection(int currentsocket)
{
  int r;
  unsigned char command;
  while(true){
    r=recvall(currentsocket, &command, 1, MSG_WAITALL);
    if (r==1)
    {
      DispatchCommand(currentsocket, command);
    }else if(r==0){
      printf("Peer has disconnected\n");
      fflush(stdout);
      close(currentsocket);
      return 0;
    }else if(r==-1){
      printf("read error on socket\n");
      fflush(stdout);
      close(currentsocket);
      return 0;
    }
  }

  return 0;
}

int main(){
    int sock0;
    struct sockaddr_in addr;
    struct sockaddr_in client;
    socklen_t len;
    int sock;

    printf("CEServer. Waiting for client connection\n");
    sock0 = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(52734);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    int yes = 1;
    setsockopt(sock0,SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
    bind(sock0, (struct sockaddr *)&addr, sizeof(addr));

    int l = listen(sock0, 32);
    if (l==0)
      printf("Listening success\n");
    else
      printf("listen=%d (error)\n", l);
    len = sizeof(client);
    while(true){
        sock = accept(sock0, (struct sockaddr *)NULL, NULL);
        printf("accept=%d\n", sock);

        fflush(stdout);
        setsockopt(sock,IPPROTO_TCP, TCP_NODELAY,(const char *)&yes, sizeof(yes));
        std::thread th1(newconnection, sock);
        th1.detach();
    }
}