#ifndef DR42_TRACE_H
#define DR42_TRACE_H

/* --------------------------------------------------------------------------
Copyright 2023 Dr. Spandan Roy

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

   --------------------------------------------------------------------------
  * Trace
  * Simple stack trace for C programs
  * --------------------------------------------------------------------------
  *  Prerequisites:
  *  1. Make sure you have addr2line installed on linux
  *  --------------------------------------------------------------------------
  *  Usage:
  *  0. #define DR42_TRACE_IMPLEMENTATION before including this file
  *     STB style include
  *     #define DR42_TRACE_IMPLEMENTATION
  *     #include "trace.h"
  *
  *  1. Call print_trace() to print the stack trace
  *  2. Call fprint_trace(fp) to print the stack trace to a file
  *  3. Call sprint_trace(buffer) to print the stack trace to a buffer
  * --------------------------------------------------------------------------
  *  Compilation stuff:
  *  1. Compile with -g
  *  2. Link with -ldl -rdynamic on linux
  *  3. Compile with -g -gcodeview on windows
  *  4. Link with "-l dbghelp -fuse-ld=lld -Wl,--pdb="
  * 
  *  Note: I have only tested this with the clang compiler
  *
* -------------------------------------------------------------------------- */

#include <stdio.h>

#ifdef _WIN64
#define print_trace() _print_trace(2);
#define fprint_trace(fp) _fprint_trace(fp, 1);
#define sprint_trace(buffer) _sprint_trace(buffer, 0);
#endif
#ifdef __linux__
#define print_trace() _print_trace(3);
#define fprint_trace(fp) _fprint_trace(fp, 2);
#define sprint_trace(buffer) _sprint_trace(buffer, 1);
#endif

int get_intermediate_trace(void** stack, size_t size);
int sprint_intermediate_trace(void** stack, char* buffer, size_t offset, size_t size);

// Private functions
void _print_trace(size_t offset);
int _fprint_trace(FILE* fp, size_t offset);
int _sprint_trace(char* buffer, size_t offset);

#endif

//#define DR42_TRACE_IMPLEMENTATION
#ifdef DR42_TRACE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char prg_name[1024];

void _print_trace(size_t offset){
  _fprint_trace(stdout, offset);
}

int _fprint_trace(FILE* fp, size_t offset){
  char buffer[1024];
  int len = _sprint_trace(buffer, offset);
  if (len < 0) {
    return -1;
  }
  #ifdef __linux__
  fprintf(fp, "%s\n", buffer);
  #endif
  #ifdef _WIN64
  fprintf(fp, "%s", buffer);
  #endif
  return len;
}

#ifdef __linux__
#include <execinfo.h>
#include <unistd.h>
#include <sys/wait.h>
#define __USE_GNU
#include <dlfcn.h>

#define MAX_STACK_FRAMES 64
void* buffer[MAX_STACK_FRAMES];

int get_intermediate_trace(void** stack, size_t size){
   return backtrace(stack, size);
}
int sprint_intermediate_trace(void** stack, char* buffer, size_t offset, size_t size){
  readlink("/proc/self/exe", prg_name, 1024);
  buffer[0] = '\0';
  char tmp[1024];
  for (size_t i = offset + 1; i < size; i++) {
    // Execute addr2line and get prettified names
    char addr2line_cmd[512];
    void* addr = stack[i] - 1;
    Dl_info info;
    if (dladdr(addr, &info) != 0) {
      void* offset = (void*)((char*)addr - (char*)info.dli_fbase);
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", info.dli_fname, offset);
    } else {
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", prg_name, addr);
    }
    char line[512];
    FILE* addr2line = popen(addr2line_cmd, "r");
    fgets(line, 512, addr2line);
    pclose(addr2line);
    if(line[0] == '?') {
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", prg_name, addr);
      FILE* addr2line = popen(addr2line_cmd, "r");
      fgets(line, 512, addr2line);
      pclose(addr2line);
    }
    // Remove pwd from path
    char* pwd = getenv("PWD");
    char* pwd_pos = strstr(line, pwd);
    if (pwd_pos != NULL) {
      // Preserve text before pwd
      char* line_pos = line;
      char* pwd_pos = strstr(line, pwd);
      while (line_pos != pwd_pos) {
	strncat(buffer, line_pos, 1);
	line_pos++;
      }
      // Skip pwd
      line_pos += strlen(pwd);
      // Skip slash
      line_pos++;
      // Print rest of line
      sprintf(tmp, "%s", line_pos);
    } else {
      sprintf(tmp, "%s", line);
    }
    strcat(buffer, tmp);
    // Stop at main
    if (strstr(line, "main at") != NULL) {
      break;
    }
  }
  buffer[strlen(buffer) - 1] = '\0';
  return strlen(buffer);
}

int _sprint_trace(char* buff, size_t offset){
  readlink("/proc/self/exe", prg_name, 1024);
  buff[0] = '\0';
  int size = backtrace(buffer, MAX_STACK_FRAMES);
  char tmp[1024];
  for (int i = offset; i < size; i++) {
    // Execute addr2line and get prettified names
    char addr2line_cmd[512];
    void* addr = buffer[i] - 1;
    Dl_info info;
    if (dladdr(addr, &info) != 0) {
      void* offset = (void*)((char*)addr - (char*)info.dli_fbase);
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", info.dli_fname, offset);
    } else {
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", prg_name, addr);
    }
    char line[512];
    FILE* addr2line = popen(addr2line_cmd, "r");
    fgets(line, 512, addr2line);
    pclose(addr2line);
    if(line[0] == '?') {
      sprintf(addr2line_cmd, "addr2line -p -f -e %s %p", prg_name, addr);
      FILE* addr2line = popen(addr2line_cmd, "r");
      fgets(line, 512, addr2line);
      pclose(addr2line);
    }
    // Remove pwd from path
    char* pwd = getenv("PWD");
    char* pwd_pos = strstr(line, pwd);
    if (pwd_pos != NULL) {
      // Preserve text before pwd
      char* line_pos = line;
      char* pwd_pos = strstr(line, pwd);
      while (line_pos != pwd_pos) {
	strncat(buff, line_pos, 1);
	line_pos++;
      }
      // Skip pwd
      line_pos += strlen(pwd);
      // Skip slash
      line_pos++;
      // Print rest of line
      sprintf(tmp, "%s", line_pos);
    } else {
      sprintf(tmp, "%s", line);
    }
    strcat(buff, tmp);
    // Stop at main
    if (strstr(line, "main at") != NULL) {
      break;
    }
  }
  buff[strlen(buff) - 1] = '\0';
  return strlen(buff);
}

#endif
#ifdef _WIN64
#include <windows.h>
#include <dbghelp.h>

int get_intermediate_trace(void** stack, size_t size){
    HANDLE       	process;
    process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );
    int ret = CaptureStackBackTrace(1, size, stack, NULL);
    return ret;
}

int sprint_intermediate_trace(void** stack, char* buffer, size_t offset, size_t size){
    HANDLE       	process;
    process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );
  SYMBOL_INFO  * symbol;
  symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
  symbol->MaxNameLen   = 255;
  symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

  IMAGEHLP_LINE64 line;
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for(size_t i = offset; i < size; i++ ){
    char tmp[1024];
    SymFromAddr( process, ( DWORD64 )( stack[ i ]  - 1), 0, symbol );
    // Get filename and line number from the address of the function call
    DWORD displacement;
    if (SymGetLineFromAddr64(process, (DWORD64)(stack[i] - 1), &displacement, &line)) {
      char filename[1024];
      // Remove cwd from filename
      GetCurrentDirectory(1024, filename);
      char* cwd_ptr = strstr(line.FileName, filename);
      if (cwd_ptr != NULL) {
	strcpy(filename, cwd_ptr + strlen(filename) + 1);
      }
      // Print the stack trace
      snprintf(tmp, 1024, "%s at %s:%lu\n", symbol->Name, filename, line.LineNumber );
    } else {
      // Unable to get source file information
      snprintf(tmp, 1024, "%s at (unknown)\n", symbol->Name);
    }
    strcat(buffer, tmp);
    if (strcmp(symbol->Name, "main") == 0) {
      // Stop printing the stack trace after reaching main
      break;
    }
  }
  strcat(buffer, "\n");
  free( symbol );
  buffer[strlen(buffer) - 1] = '\0';
  return strlen(buffer);
}

int _sprint_trace(char* buffer, size_t offset){
  buffer[0] = '\0';
  void         * stack[ 100 ];
  unsigned short frames;
  SYMBOL_INFO  * symbol;
  HANDLE       	process;

  process = GetCurrentProcess();

  SymInitialize( process, NULL, TRUE );

  frames               = CaptureStackBackTrace( 1, 100, stack, NULL ); // Start from index 1 to skip the current function (print_trace)
  symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
  symbol->MaxNameLen   = 255;
  symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

  IMAGEHLP_LINE64 line;
  line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  for(size_t i = offset; i < frames; i++ ){
    char tmp[1024];
    SymFromAddr( process, ( DWORD64 )( stack[ i ]  - 1), 0, symbol );
    // Get filename and line number from the address of the function call
    DWORD displacement;
    if (SymGetLineFromAddr64(process, (DWORD64)(stack[i] - 1), &displacement, &line)) {
      char filename[1024];
      // Remove cwd from filename
      GetCurrentDirectory(1024, filename);
      char* cwd_ptr = strstr(line.FileName, filename);
      if (cwd_ptr != NULL) {
	strcpy(filename, cwd_ptr + strlen(filename) + 1);
      }
      // Print the stack trace
      snprintf(tmp, 1024, "%s at %s:%lu\n", symbol->Name, filename, line.LineNumber );
    } else {
      // Unable to get source file information
      snprintf(tmp, 1024, "%s at (unknown)\n", symbol->Name);
    }
    strcat(buffer, tmp);
    if (strcmp(symbol->Name, "main") == 0) {
      // Stop printing the stack trace after reaching main
      break;
    }
  }
  strcat(buffer, "\n");
  free( symbol );
  buffer[strlen(buffer) - 1] = '\0';
  return strlen(buffer);
}
#endif
#endif
