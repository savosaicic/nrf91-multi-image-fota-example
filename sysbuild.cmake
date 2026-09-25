# Add app1 as SECONDARY_APP
ExternalZephyrProject_Add(
  APPLICATION app1
  SOURCE_DIR  ${APP_DIR}/app1
)
UpdateableImage_Add(APPLICATION app1 GROUP "SECONDARY_APP")

# Sysbuild only does this for the default image
set_config_bool(app1 CONFIG_BOOTLOADER_MCUBOOT y)
set_config_string(app1 CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
  "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")

# boot_go_hook for MCUboot and its s1 variant, which does
# not inherit mcuboot's EXTRA_ZEPHYR_MODULES.
foreach(_img mcuboot mcuboot_s1_variant)
  set(${_img}_EXTRA_ZEPHYR_MODULES "${APP_DIR}/mcuboot_hooks"
      CACHE INTERNAL "" FORCE)
endforeach()
