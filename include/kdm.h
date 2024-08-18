#ifndef KDM_KDM_H
#define KDM_KDM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

#define FILE_DEVICE_WINIO       (DWORD)0x00008010
#define FILE_DEVICE_ASUSIO      (DWORD)0x0000A040

#define WINIO_IOCTL_INDEX       (DWORD)0x810

#define WINIO_MAP_FUNCID        (DWORD)0x810
#define WINIO_UNMAP_FUNCID      (DWORD)0x811
#define WINIO_READMSR           (DWORD)0x816

#define IOCTL_WINIO_MAP_USER_PHYSICAL_MEMORY   CTL_CODE(FILE_DEVICE_WINIO, WINIO_MAP_FUNCID, METHOD_BUFFERED, FILE_ANY_ACCESS) //0x80102040
#define IOCTL_WINIO_UNMAP_USER_PHYSICAL_MEMORY CTL_CODE(FILE_DEVICE_WINIO, WINIO_UNMAP_FUNCID, METHOD_BUFFERED, FILE_ANY_ACCESS) //0x80102044
#define IOCTL_WINIO_READMSR                    CTL_CODE(FILE_DEVICE_WINIO, WINIO_READMSR, METHOD_BUFFERED, FILE_ANY_ACCESS)

PVOID kdm_get_heap(void);
PVOID kdm_alloc_heap(SIZE_T size);
void  kdm_free_heap(PVOID base_address);

wchar_t  kdm_wclower(wchar_t c);
int      kdm_wstrcmpi(const wchar_t *s1, const wchar_t *s2);
wchar_t* kdm_wstrncpy(wchar_t *dst, size_t sz_dst, const wchar_t *src, size_t sz_src);
wchar_t *kdm_wstrend(const wchar_t *s);

size_t kdm_get_file_size(FILE* file);
bool   kdm_write_to_file(const char* filename, char* buffer, size_t size);

bool kdm_reg_delete_key_recursive(HKEY root, LPCWSTR subkey);
bool kdm_reg_delete_key_recursive_intrnl(HKEY root, LPCWSTR subkey);

bool kdm_system_object_exist(LPCWSTR root_directory, LPCWSTR object_name);

bool kdm_query_hvci(bool* enabled, bool* strict_mode, bool* ium_enabled);
bool kdm_detect_hypervisor(char** vendor_string);

FIRMWARE_TYPE kdm_get_firmware_type(void);
const char*   kdm_get_firmware_type_str(FIRMWARE_TYPE type);

char* kdm_read_target_file(const char* filename);
void  kdm_free_target(char* buffer);

bool kdm_create_driver_entry(LPCWSTR driver_path, LPCWSTR key_name);
bool kdm_load_driver(LPCWSTR driver_name, LPCWSTR driver_path, bool unload_prev_driver);
bool kdm_unload_driver(LPCWSTR driver_name, bool remove);

bool kdm_open_driver(LPCWSTR driver_name, ACCESS_MASK desired_access, PHANDLE device_handle);
bool kdm_create_system_admin_access_sd(PSECURITY_DESCRIPTOR* security_descriptor, PACL* default_acl);

#endif
