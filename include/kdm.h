#ifndef KDM_KDM_H
#define KDM_KDM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>

size_t kdm_get_file_size(FILE* file);
bool   kdm_write_to_file(const char* filename, char* buffer, size_t size);

char* kdm_read_target_file(const char* filename);
void  kdm_free_target(char* driver_buffer);

bool kdm_detect_hypervisor(char** vendor_string);

FIRMWARE_TYPE kdm_get_firmware_type(void);
const char*   kdm_get_firmware_type_str(FIRMWARE_TYPE type);

bool kdm_query_hvci(bool* enabled, bool* strict_mode, bool* ium_enabled);

PVOID kdm_get_heap(void);
PVOID kdm_alloc_heap(SIZE_T size);
void  kdm_free_heap(PVOID base_address);

wchar_t kdm_lowercase_w(wchar_t c);
int     kdm_wstrcmpi(const wchar_t *s1, const wchar_t *s2);

bool kdm_system_object_exist(LPCWSTR root_directory, LPCWSTR object_name);

bool kdm_create_driver_entry(LPCWSTR driver_path, LPCWSTR key_name);
bool kdm_load_driver(LPCWSTR driver_name, LPCWSTR driver_path, bool unload_prev_driver);

#endif
