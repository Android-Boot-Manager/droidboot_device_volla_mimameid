LOCAL_DIR := $(GET_LOCAL_DIR)

ifeq ($(MTK_BACKLIGHT_SUPPORT_AW99703),yes)
OBJS += $(LOCAL_DIR)/aw99703/aw99703_lk_driver.o
endif
ifeq ($(CFG_BACKLIGHT_SUPPORT_LM3632),yes)
OBJS += $(LOCAL_DIR)/lm3632/lm3632_lk_driver.o
endif
