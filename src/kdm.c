#include <kdm.h>
#include <nt/consts.h>
#include <nt/ntos.h>
#include <nt/ntstatus.h>
#include <winternl.h>

size_t kdmFileSize(FILE* file) {
  size_t size;
  if (fseek(file, 0, SEEK_END))
    return -1;

  size = ftell(file);
  if (size == -1L || fseek(file, 0, SEEK_SET))
    return -1;

  return size;
}

bool kdmWriteToFile(const char* name, char* buf, size_t size) {
  FILE* file = fopen(name, "w");
  if (!file) {
    printf("Failed writing to file: %s\n", name);
    return false;
  }

  if (fwrite(buf, 1, size, file) != size) {
    printf("Error: file written partially: %s\n", name);
    fclose(file);
    return false;
  }

  fclose(file);
  return true;
}

char* kdmReadTargetFile(const char* name) {
  FILE* file = fopen(name, "r");
  if (!file) {
    printf("File doesnt exist: %s\n", name);
    return NULL;
  }

  size_t size = kdmFileSize(file);
  if (size == -1) {
    printf("Failed to get driver file size\n");
    fclose(file);
    return NULL;
  }

  char* buf = malloc(size);
  if (!buf) {
    printf("Failed to allocate driver buffer\n");
    fclose(file);
    return NULL;
  }

  if (fread(buf, 1, size, file) != size) {
    printf("Failed to read driver file\n");
    free(buf);
    fclose(file);
    return NULL;
  }

  fclose(file);
  return buf;
}

void kdmFreeTarget(char* driverBuf) {
    free(driverBuf);
}

bool kdmDetectHypervisor(char** vendorString) {
  SYSTEM_HYPERVISOR_DETAIL_INFORMATION hdi;
  RtlZeroMemory(&hdi, sizeof(hdi));

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemHypervisorDetailInformation, &hdi, sizeof(hdi), &retlen))) {
    if (vendorString)
      *vendorString = ((PHV_VENDOR_AND_MAX_FUNCTION)&hdi.HvVendorAndMaxFunction.Data)->VendorName;
    return true;
  }
  else {
    int cpuInfo[4] = { -1, -1, -1, -1 };
    __cpuid(cpuInfo, 1);

    if ((cpuInfo[2] >> 31) & 1) {
      __cpuid(cpuInfo, 0x40000000);
      if (vendorString)
        *vendorString = cpuInfo + 1;
      return true;
    }
  }
}

int kdmGetFirmwareType(void) {
  SYSTEM_BOOT_ENVIRONMENT_INFORMATION sbei;
  RtlZeroMemory(&sbei, sizeof(sbei));

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemBootEnvironmentInformation, &sbei, sizeof(sbei), &retlen)))
    return sbei.FirmwareType;
  else
    return FirmwareTypeUnknown;
}

const char* kdmGetFirmwareTypeString(int type) {
  switch (type) {
  case FirmwareTypeBios:
    return "BIOS";
  case FirmwareTypeUefi:
    return "UEFI";
  default:
    return "Unknown";
  }
}

bool kdmQueryHVCI(bool* enabled, bool* strictMode, bool* IUMEnabled) {
  SYSTEM_CODEINTEGRITY_INFORMATION ci;
  ci.Length = sizeof(ci);

  ULONG retlen;
  if (NT_SUCCESS(NtQuerySystemInformation(SystemCodeIntegrityInformation, &ci, sizeof(ci), &retlen))) {
    bool hvciEnabled = (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_ENABLED) && (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED);

    if (enabled)
      *enabled = hvciEnabled;

    if (strictMode)
      *strictMode = (hvciEnabled == TRUE) && (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_STRICTMODE_ENABLED);

    if (IUMEnabled)
      *IUMEnabled = (ci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_IUM_ENABLED);

    return true;
  }

  return false;
}

PVOID kdmProcessHeap(void) {
  return NtCurrentPeb()->ProcessHeap;
}

PVOID kdmHeapAlloc(SIZE_T size) {
  return RtlAllocateHeap(kdmProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void kdmHeapFree(PVOID baseAddress) {
  RtlFreeHeap(kdmProcessHeap(), 0, baseAddress);
}

wchar_t kdmLowerW(wchar_t c) {
	if (c >= 'A' && c <= 'Z')
		return c + 0x20;
	else
		return c;
}

int kdmWstrcmpi(const wchar_t *s1, const wchar_t *s2) {
	if (s1 == s2) return 0;
	if (s1 == 0)  return -1;
	if (s2 == 0)  return 1;

	wchar_t c1, c2;
	do {
		c1 = kdmLowerW(*s1);
		c2 = kdmLowerW(*s2);
		s1++;
		s2++;
	} while (c1 != 0 && c1 == c2);
	
	return (int)(c1 - c2);
}

bool kdmIsSystemObjectExist(LPCWSTR rootDirectory, LPCWSTR objectName) {
  UNICODE_STRING name;
  RtlZeroMemory(&name, sizeof(name));
  RtlInitUnicodeString(&name, rootDirectory);

  OBJECT_ATTRIBUTES attr;
  InitializeObjectAttributes(&attr, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

  HANDLE dir;
  if (!NT_SUCCESS(NtOpenDirectoryObject(&dir, DIRECTORY_QUERY, &attr)))
    return false;
  
  ULONG ctx = 0;
  while (true) {
    ULONG retlen = 0;
    if (NtQueryDirectoryObject(dir, NULL, 0, TRUE, FALSE, &ctx, &retlen) != STATUS_BUFFER_TOO_SMALL)
      break;

    POBJECT_DIRECTORY_INFORMATION odi = (POBJECT_DIRECTORY_INFORMATION)kdmHeapAlloc(retlen);
    if (!odi)
      break;

    if (!NT_SUCCESS(NtQueryDirectoryObject(dir, odi, retlen, TRUE, FALSE, &ctx, &retlen))) {
      kdmHeapFree(odi);
      break;
    }

    if (kdmWstrcmpi(odi->Name.Buffer, objectName) == 0) {
      kdmHeapFree(odi);
      NtClose(dir);
      return true;
    }
    kdmHeapFree(odi);
  }

  NtClose(dir);
  return false;
}

bool kdmCreateDriverEntry(LPCWSTR driverPath, LPCWSTR keyName) {
  UNICODE_STRING driverImagePath;
  RtlInitEmptyUnicodeString(&driverImagePath, NULL, 0);

  if (!RtlDosPathNameToNtPathName_U(driverPath, &driverImagePath, NULL, NULL))
    return false;

  HKEY keyHandle = NULL;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &keyHandle, NULL)) {
    RtlFreeUnicodeString(&driverImagePath);
    return false;
  }

  DWORD data = SERVICE_ERROR_NORMAL;
  if (!RegSetValueEx(keyHandle, TEXT("ErrorControl"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))) {
    data = SERVICE_KERNEL_DRIVER;

    if (!RegSetValueEx(keyHandle, TEXT("Type"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))) {
      data = SERVICE_DEMAND_START;

      if (!RegSetValueEx(keyHandle, TEXT("Start"), 0, REG_DWORD, (BYTE*)&data, sizeof(data))
       && !RegSetValueEx(keyHandle, TEXT("ImagePath"), 0, REG_EXPAND_SZ, (BYTE*)driverImagePath.Buffer, (DWORD)driverImagePath.Length + sizeof(UNICODE_NULL))) {
        RegCloseKey(keyHandle);
        RtlFreeUnicodeString(&driverImagePath);

        return true;
      }
    }
  }

  RegCloseKey(keyHandle);
  RtlFreeUnicodeString(&driverImagePath);

  return false;
}

bool kdmLoadDriver(LPCWSTR driverName, LPCWSTR driverPath, bool unloadPrevDriver) {
  wchar_t buf[MAX_PATH + 1];
  RtlZeroMemory(buf, sizeof(buf));

  if (FAILED(StringCchPrintf(buf, MAX_PATH, DRIVER_REGKEY, NT_REG_PREP, driverName)))
    return false;
  
  if (!kdmCreateDriverEntry(driverPath, &buf[RTL_NUMBER_OF(NT_REG_PREP)]))
    return false;
  
  UNICODE_STRING driverServiceName;
  RtlInitUnicodeString(&driverServiceName, buf);
  
  NTSTATUS status = NtLoadDriver(&driverServiceName);
  if (status == STATUS_IMAGE_ALREADY_LOADED ||
      status == STATUS_OBJECT_NAME_COLLISION ||
      status == STATUS_OBJECT_NAME_EXISTS)
  {
      if (unloadPrevDriver
       && NT_SUCCESS(NtUnloadDriver(&driverServiceName))
       && NT_SUCCESS(NtLoadDriver(&driverServiceName)))
       return true;
  }
  else if (status == STATUS_OBJECT_NAME_EXISTS)
    return true;

  return false;
}
