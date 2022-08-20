LOCAL_DIR := $(GET_LOCAL_DIR)

# prize wyq 20191018 add for chargepump backlight-start
MODULES += \
	$(LOCAL_DIR)/video \
	$(LOCAL_DIR)/lcm \
	$(LOCAL_DIR)/gic \
	$(LOCAL_DIR)/md_com \
	$(LOCAL_DIR)/chargepump
# prize wyq 20191018 add for chargepump backlight-end

OBJS += \
	$(LOCAL_DIR)/dev.o
