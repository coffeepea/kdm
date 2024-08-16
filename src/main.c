#include <stdio.h>
#include <kdm.h>
#include <winio.h>

int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Usage:\n");
    printf("  %s driver.sys\n", argv[0]);
    return 0;
  }

  bool hvciEnabled;
  if (kdmQueryHVCI(&hvciEnabled, NULL, NULL) && hvciEnabled) {
    printf("Error: HVCI is enabled\n");
    return -1;
  }

  char* driverBuf = kdmReadTargetFile(argv[1]);

  char* vendorString;
  if (kdmDetectHypervisor(&vendorString)) {
    char name[13] = { 0 };
    memcpy(name, vendorString, 12);
    printf("Hypervisor: %s\n", name);
  }

  int firmwareType = kdmGetFirmwareType();
  printf("Firmware: %s\n", kdmGetFirmwareTypeString(firmwareType));

  if (!kdmIsSystemObjectExist(L"\\Device", L"MsIo")) {
    kdmWriteToFile("WinIo.sys", winio, sizeof(winio));
    kdmLoadDriver(L"MsIo64", L"WinIo.sys", true);
    
  }

  kdmFreeTarget(driverBuf);

  return 0;
}
