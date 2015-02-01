CC ?= gcc

TARGETS = utc_ApplicationFW_ug_init_func \
	utc_ApplicationFW_ug_create_func \
	utc_ApplicationFW_ug_pause_func \
	utc_ApplicationFW_ug_resume_func \
	utc_ApplicationFW_ug_destroy_func \
	utc_ApplicationFW_ug_destroy_all_func \
	utc_ApplicationFW_ug_get_layout_func \
	utc_ApplicationFW_ug_get_parent_layout_func \
	utc_ApplicationFW_ug_get_mode_func \
	utc_ApplicationFW_ug_get_window_func \
	utc_ApplicationFW_ug_get_conformant_func \
	utc_ApplicationFW_ug_is_installed_func \
	utc_ApplicationFW_ug_send_message_func \
	utc_ApplicationFW_ug_send_event_func \
	utc_ApplicationFW_ug_send_key_event_func \
	utc_ApplicationFW_ug_destroy_me_func \
	utc_ApplicationFW_ug_send_result_func \
	utc_ApplicationFW_ug_send_result_full_func \
	utc_ApplicationFW_ug_disable_effect_func


PKGS = elementary ui-gadget-1 ecore ecore-x evas capi-appfw-application

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall
CFLAGS += -DTESTUG=\"setting-time-efl\"
CFLAGS += -DTESTUG2=\"setting-memory-efl\"

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
