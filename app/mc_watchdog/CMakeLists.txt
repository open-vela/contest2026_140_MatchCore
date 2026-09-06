# MC Watchdog AI — CMakeLists.txt

if(CONFIG_LVX_USE_DEMO_MC_WATCHDOG)
  nuttx_add_application(
    NAME mc_watchdog
    PRIORITY 100
    STACKSIZE 32768
    MODULE ${CONFIG_LVX_USE_DEMO_MC_WATCHDOG}
    SRCS
      mc_watchdog_main.c
      mc_protocol.c
      ui_dashboard.c
      ui_alert.c
      ai_integration.c
    INCLUDE_DIRECTORIES
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${NUTTX_APPS_DIR}/packages/ai_agent/include
  )
endif()
