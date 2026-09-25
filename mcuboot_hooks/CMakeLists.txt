# Only built into the mcuboot image (injected via sysbuild.cmake)
if(CONFIG_MCUBOOT AND CONFIG_BOOT_GO_HOOKS)
  zephyr_library()
  zephyr_library_sources(boot_hooks.c)
  zephyr_library_include_directories(
    ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/include
    ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/include

    # For WORKAROUND(mcuboot#2848)
    ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/src
  )
  if(CONFIG_NRF_SECURITY)
    zephyr_library_link_libraries(mbedcrypto)
  endif()
endif()
