#ifndef KDM_KDM_H
#define KDM_KDM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

size_t kdmFileSize(FILE* file);
bool   kdmWriteToFile(const char* name, char* buf, size_t size);

char* kdmReadTargetFile(const char* name);
void  kdmFreeTarget(char* driverBuf);

bool kdmDetectHypervisor(char** vendorString);

int         kdmGetFirmwareType(void);
const char* kdmGetFirmwareTypeString(int type);

bool kdmQueryHVCI(bool* enabled, bool* strictMode, bool* IUMEnabled);

PVOID kdmProcessHeap(void);
PVOID kdmHeapAlloc(SIZE_T size);
void  kdmHeapFree(PVOID baseAddress);

wchar_t kdmLowerW(wchar_t c);
int     kdmWstrcmpi(const wchar_t *s1, const wchar_t *s2);

bool kdmIsSystemObjectExist(LPCWSTR rootDirectory, LPCWSTR objectName);

bool kdmCreateDriverEntry(LPCWSTR driverPath, LPCWSTR keyName);
bool kdmLoadDriver(LPCWSTR driverName, LPCWSTR driverPath, bool unloadPrevDriver);

#endif
