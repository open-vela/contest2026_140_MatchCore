############################################################################
# MC Watchdog AI — Makefile
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = mc_watchdog
PRIORITY  = 100
STACKSIZE = 32768
MODULE    = $(CONFIG_LVX_USE_DEMO_MC_WATCHDOG)

CSRCS = mc_protocol.c ui_dashboard.c ui_alert.c ai_integration.c
MAINSRC = mc_watchdog_main.c

include $(APPDIR)/Application.mk
