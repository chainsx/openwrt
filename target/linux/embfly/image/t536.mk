define Device/embfly_stk-t536
  KERNEL_NAME := Image
  DEVICE_VENDOR := Embfly
  DEVICE_MODEL := STK-T536
  DEVICE_DTS = allwinner/sun55i-t536-stk-t536
  DEVICE_DTS_NAME = sun55i-t536-stk-t536
  DEVICE_PACKAGES := kmod-mac80211
  IMAGE/sysupgrade.img.gz := sunxi-uboot-img | gzip | append-metadata
endef
TARGET_DEVICES += embfly_stk-t536
