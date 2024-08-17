#include <kdm.h>
#include <nt/consts.h>
#include <nt/ntos.h>
#include <nt/ntstatus.h>
#include <winternl.h>

size_t kdm_get_file_size(FILE* file) {
  size_t size;
  if (fseek(file, 0, SEEK_END))
    return -1;

  size = ftell(file);
  if (size == -1L || fseek(file, 0, SEEK_SET))
    return -1;

  return size;
}

bool kdm_write_to_file(const char* filename, char* buffer, size_t size) {
  FILE* file = fopen(filename, "w");
  if (!file)
    return false;

  if (fwrite(buffer, 1, size, file) != size) {
    fclose(file);
    remove(filename);
    return false;
  }

  fclose(file);
  return true;
}

char* kdm_read_target_file(const char* filename) {
  FILE* file = fopen(filename, "r");
  if (!file)
    return NULL;

  size_t size = kdm_get_file_size(file);
  if (size == -1) {
    fclose(file);
    return NULL;
  }

  char* buffer = malloc(size);
  if (!buffer) {
    fclose(file);
    return NULL;
  }

  if (fread(buffer, 1, size, file) != size) {
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);
  return buffer;
}

void kdm_free_target(char* driver_buffer) {
  free(driver_buffer);
}

bool kdm_detect_hypervisor(char** vendor_string) {
  SYSTEM_HYPERVISOR_DETAIL_INFORMATION hdi;
  RtlZeroMemory(&hdi, sizeof(hdi));

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemHypervisorDetailInformation, &hdi, sizeof(hdi), &retlen))) {
    if (vendor_string)
      *vendor_string = ((PHV_VENDOR_AND_MAX_FUNCTION)&hdi.HvVendorAndMaxFunction.Data)->VendorName;

    return true;
  }
  else {
    int cpu_info[4] = { -1, -1, -1, -1 };
    __cpuid(cpu_info, 1);

    if ((cpu_info[2] >> 31) & 1) {
      __cpuid(cpu_info, 0x40000000);

      if (vendor_string)
        *vendor_string = cpu_info + 1;

      return true;
    }
  }

  return false;
}

FIRMWARE_TYPE kdm_get_firmware_type(void) {
  SYSTEM_BOOT_ENVIRONMENT_INFORMATION sbei;
  RtlZeroMemory(&sbei, sizeof(sbei));

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemBootEnvironmentInformation, &sbei, sizeof(sbei), &retlen)))
    return sbei.FirmwareType;
  else
    return FirmwareTypeUnknown;
}

const char* kdm_get_firmware_type_str(FIRMWARE_TYPE type) {
  switch (type) {
  case FirmwareTypeBios:
    return "BIOS";
  case FirmwareTypeUefi:
    return "UEFI";
  default:
    return "Unknown";
  }
}

bool kdm_query_hvci(bool* enabled, bool* strict_mode, bool* ium_enabled) {
  SYSTEM_CODEINTEGRITY_INFORMATION ci;
  ci.Length = sizeof(ci);

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemCodeIntegrityInformation, &ci, sizeof(ci), &retlen))) {
    bool hvci = (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_ENABLED) && (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED);

    if (enabled)
      *enabled = hvci;

    if (strict_mode)
      *strict_mode = hvci && (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED);

    if (ium_enabled)
      *ium_enabled = ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED;

    return true;
  }

  return false;
}

PVOID kdm_get_heap(void) {
  return NtCurrentPeb()->ProcessHeap;
}

PVOID kdm_alloc_heap(SIZE_T size) {
  return RtlAllocateHeap(kdm_get_heap(), HEAP_ZERO_MEMORY, size);
}

void kdm_free_heap(PVOID base_address) {
  RtlFreeHeap(kdm_get_heap(), 0, base_address);
}

wchar_t kdm_lowercase_w(wchar_t c) {
	if (c >= 'A' && c <= 'Z')
		return c + 0x20;
	else
		return c;
}

int kdm_wstrcmpi(const wchar_t *s1, const wchar_t *s2) {
	if (s1 == s2) return 0;
	if (s1 == 0)  return -1;
	if (s2 == 0)  return 1;

	wchar_t c1, c2;
	do {
		c1 = kdm_lowercase_w(*s1);
		c2 = kdm_lowercase_w(*s2);
		s1++;
		s2++;
	} while (c1 != 0 && c1 == c2);
	
	return (int)(c1 - c2);
}

bool kdm_system_object_exist(LPCWSTR root_directory, LPCWSTR object_name) {
  UNICODE_STRING root_us;
  RtlZeroMemory(&root_us, sizeof(root_us));
  RtlInitUnicodeString(&root_us, root_directory);

  OBJECT_ATTRIBUTES root_attr;
  InitializeObjectAttributes(&root_attr, &root_us, OBJ_CASE_INSENSITIVE, NULL, NULL);

  HANDLE dir_handle;
  if (!NT_SUCCESS(NtOpenDirectoryObject(&dir_handle, DIRECTORY_QUERY, &root_attr)))
    return false;
  
  ULONG ctx = 0;
  while (true) {
    ULONG retlen = 0;
    if (NtQueryDirectoryObject(dir_handle, NULL, 0, TRUE, FALSE, &ctx, &retlen) != STATUS_BUFFER_TOO_SMALL)
      break;

    POBJECT_DIRECTORY_INFORMATION odi = (POBJECT_DIRECTORY_INFORMATION)kdm_alloc_heap(retlen);
    if (!odi)
      break;

    if (!NT_SUCCESS(NtQueryDirectoryObject(dir_handle, odi, retlen, TRUE, FALSE, &ctx, &retlen))) {
      kdm_free_heap(odi);
      break;
    }

    if (kdm_wstrcmpi(odi->Name.Buffer, object_name) == 0) {
      kdm_free_heap(odi);
      NtClose(dir_handle);
      return true;
    }
    kdm_free_heap(odi);
  }

  NtClose(dir_handle);
  return false;
}

bool kdm_create_driver_entry(LPCWSTR driver_path, LPCWSTR key_name) {
  UNICODE_STRING driver_image_path;
  RtlInitEmptyUnicodeString(&driver_image_path, NULL, 0);

  if (!RtlDosPathNameToNtPathName_U(driver_path, &driver_image_path, NULL, NULL))
    return false;

  HKEY key_handle = NULL;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, key_name, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key_handle, NULL)) {
    RtlFreeUnicodeString(&driver_image_path);
    return false;
  }

  DWORD data = SERVICE_ERROR_NORMAL;
  if (!RegSetValueEx(key_handle, TEXT("ErrorControl"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))) {
    data = SERVICE_KERNEL_DRIVER;

    if (!RegSetValueEx(key_handle, TEXT("Type"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))) {
      data = SERVICE_DEMAND_START;

      if (!RegSetValueEx(key_handle, TEXT("Start"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))
       && !RegSetValueEx(key_handle, TEXT("ImagePath"), 0, REG_EXPAND_SZ, (BYTE*)driver_image_path.Buffer, (DWORD)driver_image_path.Length + sizeof(UNICODE_NULL))) {
        RegCloseKey(key_handle);
        RtlFreeUnicodeString(&driver_image_path);

        return true;
      }
    }
  }

  RegCloseKey(key_handle);
  RtlFreeUnicodeString(&driver_image_path);

  return false;
}

bool kdm_load_driver(LPCWSTR driver_name, LPCWSTR driver_path, bool unload_prev_driver) {
  wchar_t buffer[MAX_PATH + 1];
  RtlZeroMemory(buffer, sizeof(buffer));

  if (FAILED(StringCchPrintf(buffer, MAX_PATH, DRIVER_REGKEY, NT_REG_PREP, driver_name)))
    return false;
  
  if (!kdm_create_driver_entry(driver_path, &buffer[RTL_NUMBER_OF(NT_REG_PREP)]))
    return false;
  
  UNICODE_STRING driver_service_name;
  RtlInitUnicodeString(&driver_service_name, buffer);
  
  NTSTATUS status = NtLoadDriver(&driver_service_name);
  if (status == STATUS_IMAGE_ALREADY_LOADED ||
      status == STATUS_OBJECT_NAME_COLLISION ||
      status == STATUS_OBJECT_NAME_EXISTS)
  {
      if (unload_prev_driver
       && NT_SUCCESS(NtUnloadDriver(&driver_service_name))
       && NT_SUCCESS(NtLoadDriver(&driver_service_name)))
       return true;
  }
  else if (status == STATUS_OBJECT_NAME_EXISTS)
    return true;

  return false;
}
