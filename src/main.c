#include <stdio.h>
#include <kdm.h>
#include <winio.h>

int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Usage:\n");
    printf("  %s driver.sys\n", argv[0]);
    return 0;
  }

  bool hvci_enabled;
  if (kdm_query_hvci(&hvci_enabled, NULL, NULL) && hvci_enabled) {
    printf("Error: HVCI is enabled\n");
    return -1;
  }

  char* driver_buffer = kdm_read_target_file(argv[1]);

  char* vendor_string;
  if (kdm_detect_hypervisor(&vendor_string)) {
    char name[13] = { 0 };
    memcpy(name, vendor_string, 12);
    printf("Hypervisor: %s\n", name);
  }

  int firmware_type = kdm_get_firmware_type();
  printf("Firmware: %s\n", kdm_get_firmware_type_str(firmware_type));

  if (!kdm_system_object_exist(L"\\Device", L"MsIo")) {
    kdm_write_to_file("WinIo.sys", winio, sizeof(winio));
    kdm_load_driver(L"MsIo64", L"WinIo.sys", true);
    
  }

  kdm_free_target(driver_buffer);

  return 0;
}
