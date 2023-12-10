define Device/jetson_tx2
  DEVICE_VENDOR := NVIDIA
  DEVICE_MODEL := Jetson_TX2
  DEVICE_DTS := nvidia/tegra186-p2771-0000
  IMAGE/sysupgrade.img.gz := boot-common | tegra-sdcard | gzip | append-metadata
  DEVICE_PACKAGES := kmod-usb-storage wpad-basic-mbedtls
endef
TARGET_DEVICES += jetson_tx2
