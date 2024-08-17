#include <log.h>
#include <kdm.h>
#include <winio.h>

int main(int argc, const char** argv) {
  if (argc != 2) {
    log_info("Usage:\n");
    log_info("  %s driver.sys\n", argv[0]);
    return 0;
  }

  bool hvci_enabled;
  if (kdm_query_hvci(&hvci_enabled, NULL, NULL) && hvci_enabled) {
    log_error("HVCI is enabled\n");
    return -1;
  }

  char* driver_buffer = kdm_read_target_file(argv[1]);

  char* vendor_string;
  if (kdm_detect_hypervisor(&vendor_string)) {
    char name[13] = { 0 };
    memcpy(name, vendor_string, 12);
    log_info("Hypervisor: %s\n", name);
  }

  int firmware_type = kdm_get_firmware_type();
  log_info("Firmware: %s\n", kdm_get_firmware_type_str(firmware_type));

  if (!kdm_system_object_exist(L"\\Device", L"MsIo")) {
    if (!kdm_write_to_file("WinIo.sys", winio, sizeof(winio))) {
      log_error("Cant extract vuln driver");
      kdm_free_target(driver_buffer);
      return -1;
    }
    kdm_load_driver(L"MsIo64", L"WinIo.sys", true);

    // do stuff

    kdm_unload_driver(L"MsIo64", true);
  }

  kdm_free_target(driver_buffer);
  return 0;
}
