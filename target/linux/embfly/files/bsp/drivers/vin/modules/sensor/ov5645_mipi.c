#include "../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"


MODULE_AUTHOR("zyj");
MODULE_DESCRIPTION("driver_ov5645_mipi");
MODULE_LICENSE("GPL");

/*define module timing*/
#define MCLK				(24*1000*1000)
#define VREF_POL			V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define HREF_POL			V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL				V4L2_MBUS_PCLK_SAMPLE_RISING

#define DRIVER_NAME "ov5645_mipi"

//static int register_cci_cnt = 0;

/*
    * OV5645 register definitions
	 */
#define REG_SYSTEM_CTROL0			0x3008
#define REG_CHIP_ID_HIGH		0x300a
#define REG_CHIP_ID_LOW			0x300b


#define REG_NULL			0x0000	/* Array end token */

#define OV5645_SOFT_RESET			(1 << 7)

#define OV564X_ID(_msb, _lsb)		((_msb) << 8 | (_lsb))
#define OV5645_ID			0x5645

#define SENSOR_FRAME_RATE	30

#define I2C_ADDR  0x78

/*static struct delayed_work sensor_s_ae_ratio_work;*/
#define SENSOR_NAME "ov5645_mipi"
#define OV5645_SYSTEM_CTRL0     0x3008
#define     OV5645_SYSTEM_CTRL0_START   0x02
#define     OV5645_SYSTEM_CTRL0_STOP    0x42
#define OV5645_CHIP_ID_HIGH     0x300a
#define     OV5645_CHIP_ID_HIGH_BYTE    0x56
#define OV5645_CHIP_ID_LOW      0x300b
#define     OV5645_CHIP_ID_LOW_BYTE     0x45
#define OV5645_IO_MIPI_CTRL00       0x300e
#define OV5645_PAD_OUTPUT00     0x3019
#define OV5645_AWB_MANUAL_CONTROL   0x3406
#define     OV5645_AWB_MANUAL_ENABLE    BIT(0)
#define OV5645_AEC_PK_MANUAL        0x3503
#define     OV5645_AEC_MANUAL_ENABLE    BIT(0)
#define     OV5645_AGC_MANUAL_ENABLE    BIT(1)
#define OV5645_TIMING_TC_REG20      0x3820
#define     OV5645_SENSOR_VFLIP     BIT(1)
#define     OV5645_ISP_VFLIP        BIT(2)
#define OV5645_TIMING_TC_REG21      0x3821
#define     OV5645_SENSOR_MIRROR        BIT(1)
#define OV5645_MIPI_CTRL00      0x4800
#define OV5645_PRE_ISP_TEST_SETTING_1   0x503d
#define     OV5645_TEST_PATTERN_MASK    0x3
#define     OV5645_SET_TEST_PATTERN(x)  ((x) & OV5645_TEST_PATTERN_MASK)
#define     OV5645_TEST_PATTERN_ENABLE  BIT(7)
#define OV5645_SDE_SAT_U        0x5583
#define OV5645_SDE_SAT_V        0x5584

struct v4l2_subdev *g_ov5645_sd;

static struct regval_list ov5645_init_regs_5645_mipi[] = {
	{ 0x3103, 0x11 }, { 0x3008, 0x82 }, { REG_DLY, 0x05 },
	{ 0x3008, 0x42 }, { 0x3103, 0x03 }, { 0x3503, 0x07 },
	{ 0x3002, 0x1c }, { 0x3006, 0xc3 },
	{ 0x3017, 0x00 }, { 0x3018, 0x00 },
	{ 0x302e, 0x0b }, { 0x3037, 0x13 }, { 0x3108, 0x01 },
	{ 0x3611, 0x06 }, { 0x3500, 0x00 }, { 0x3501, 0x01 },
	{ 0x3502, 0x00 }, { 0x350a, 0x00 }, { 0x350b, 0x3f },
	{ 0x3620, 0x33 }, { 0x3621, 0xe0 }, { 0x3622, 0x01 },
	{ 0x3630, 0x2e }, { 0x3631, 0x00 }, { 0x3632, 0x32 },
	{ 0x3633, 0x52 }, { 0x3634, 0x70 }, { 0x3635, 0x13 },
	{ 0x3636, 0x03 }, { 0x3703, 0x5a }, { 0x3704, 0xa0 },
	{ 0x3705, 0x1a }, { 0x3709, 0x12 }, { 0x370b, 0x61 },
	{ 0x370f, 0x10 },
	{ 0x3715, 0x78 },
	{ 0x3717, 0x01 },
	{ 0x371b, 0x20 },
	{ 0x3731, 0x12 },
	{ 0x3901, 0x0a },
	{ 0x3905, 0x02 },
	{ 0x3906, 0x10 },
	{ 0x3719, 0x86 },
	{ 0x3810, 0x00 },
	{ 0x3811, 0x10 },
	{ 0x3812, 0x00 },
	{ 0x3821, 0x01 },
	{ REG_DLY, 0x05 },
	{ 0x3824, 0x01 },
	{ REG_DLY, 0x05 },
	{ 0x3826, 0x03 },
	{ 0x3828, 0x08 },
	{ 0x3a19, 0xf8 },
	{ 0x3c01, 0x34 },
	{ 0x3c04, 0x28 },
	{ 0x3c05, 0x98 },
	{ 0x3c07, 0x07 },
	{ 0x3c09, 0xc2 },
	{ 0x3c0a, 0x9c },
	{ 0x3c0b, 0x40 },
	{ 0x3c01, 0x34 },
	{ 0x4001, 0x02 },
	{ 0x4514, 0x00 },
	{ 0x4520, 0xb0 },
	{ 0x460b, 0x37 },
	{ 0x460c, 0x20 },
	{ 0x4818, 0x01 },
	{ 0x481d, 0xf0 },
	{ 0x481f, 0x50 },
	{ 0x4823, 0x70 },
	{ 0x4831, 0x14 },
	{ 0x5000, 0xa7 },
	{ 0x5001, 0x83 },
	{ 0x501d, 0x00 },
	{ 0x501f, 0x00 },
	{ 0x503d, 0x00 },
	{ 0x505c, 0x30 },
	{ 0x5181, 0x59 },
	{ 0x5183, 0x00 },
	{ 0x5191, 0xf0 },
	{ 0x5192, 0x03 },
	{ 0x5684, 0x10 },
	{ 0x5685, 0xa0 },
	{ 0x5686, 0x0c },
	{ 0x5687, 0x78 },
	{ 0x5a00, 0x08 },
	{ 0x5a21, 0x00 },
	{ 0x5a24, 0x00 },
	{ 0x3008, 0x02 },
	{ 0x3503, 0x00 },
	{ 0x5180, 0xff },
	{ 0x5181, 0xf2 },
	{ 0x5182, 0x00 },
	{ 0x5183, 0x14 },
	{ 0x5184, 0x25 },
	{ 0x5185, 0x24 },
	{ 0x5186, 0x09 },
	{ 0x5187, 0x09 },
	{ 0x5188, 0x0a },
	{ 0x5189, 0x75 },
	{ 0x518a, 0x52 },
	{ 0x518b, 0xea },
	{ 0x518c, 0xa8 },
	{ 0x518d, 0x42 },
	{ 0x518e, 0x38 },
	{ 0x518f, 0x56 },
	{ 0x5190, 0x42 },
	{ 0x5191, 0xf8 },
	{ 0x5192, 0x04 },
	{ 0x5193, 0xfd },
	{ 0x5194, 0xa7 },
	{ 0x5195, 0xfc },
	{ 0x5196, 0x03 },
	{ 0x5197, 0x01 },
	{ 0x5198, 0x04 },
	{ 0x5199, 0x12 },
	{ 0x519a, 0x04 },
	{ 0x519b, 0x00 },
	{ 0x519c, 0x06 },
	{ 0x519d, 0x82 },
	{ 0x519e, 0x38 },
	{ 0x5381, 0x1e },
	{ 0x5382, 0x5b },
	{ 0x5383, 0x08 },
	{ 0x5384, 0x0a },
	{ 0x5385, 0x7e },
	{ 0x5386, 0x88 },
	{ 0x5387, 0x7c },
	{ 0x5388, 0x6c },
	{ 0x5389, 0x10 },
	{ 0x538a, 0x01 },
	{ 0x538b, 0x98 },
	{ 0x5300, 0x08 },
	{ 0x5301, 0x30 },
	{ 0x5302, 0x10 },
	{ 0x5303, 0x00 },
	{ 0x5304, 0x08 },
	{ 0x5305, 0x30 },
	{ 0x5306, 0x08 },
	{ 0x5307, 0x16 },
	{ 0x5309, 0x08 },
	{ 0x530a, 0x30 },
	{ 0x530b, 0x04 },
	{ 0x530c, 0x06 },
	{ 0x5480, 0x01 },
	{ 0x5481, 0x08 },
	{ 0x5482, 0x14 },
	{ 0x5483, 0x28 },
	{ 0x5484, 0x51 },
	{ 0x5485, 0x65 },
	{ 0x5486, 0x71 },
	{ 0x5487, 0x7d },
	{ 0x5488, 0x87 },
	{ 0x5489, 0x91 },
	{ 0x548a, 0x9a },
	{ 0x548b, 0xaa },
	{ 0x548c, 0xb8 },
	{ 0x548d, 0xcd },
	{ 0x548e, 0xdd },
	{ 0x548f, 0xea },
	{ 0x5490, 0x1d },
	{ 0x5580, 0x02 },
	{ 0x5583, 0x40 },
	{ 0x5584, 0x10 },
	{ 0x5589, 0x10 },
	{ 0x558a, 0x00 },
	{ 0x558b, 0xf8 },
	{ 0x5800, 0x3f },
	{ 0x5801, 0x16 },
	{ 0x5802, 0x0e },
	{ 0x5803, 0x0d },
	{ 0x5804, 0x17 },
	{ 0x5805, 0x3f },
	{ 0x5806, 0x0b },
	{ 0x5807, 0x06 },
	{ 0x5808, 0x04 },
	{ 0x5809, 0x04 },
	{ 0x580a, 0x06 },
	{ 0x580b, 0x0b },
	{ 0x580c, 0x09 },
	{ 0x580d, 0x03 },
	{ 0x580e, 0x00 },
	{ 0x580f, 0x00 },
	{ 0x5810, 0x03 },
	{ 0x5811, 0x08 },
	{ 0x5812, 0x0a },
	{ 0x5813, 0x03 },
	{ 0x5814, 0x00 },
	{ 0x5815, 0x00 },
	{ 0x5816, 0x04 },
	{ 0x5817, 0x09 },
	{ 0x5818, 0x0f },
	{ 0x5819, 0x08 },
	{ 0x581a, 0x06 },
	{ 0x581b, 0x06 },
	{ 0x581c, 0x08 },
	{ 0x581d, 0x0c },
	{ 0x581e, 0x3f },
	{ 0x581f, 0x1e },
	{ 0x5820, 0x12 },
	{ 0x5821, 0x13 },
	{ 0x5822, 0x21 },
	{ 0x5823, 0x3f },
	{ 0x5824, 0x68 },
	{ 0x5825, 0x28 },
	{ 0x5826, 0x2c },
	{ 0x5827, 0x28 },
	{ 0x5828, 0x08 },
	{ 0x5829, 0x48 },
	{ 0x582a, 0x64 },
	{ 0x582b, 0x62 },
	{ 0x582c, 0x64 },
	{ 0x582d, 0x28 },
	{ 0x582e, 0x46 },
	{ 0x582f, 0x62 },
	{ 0x5830, 0x60 },
	{ 0x5831, 0x62 },
	{ 0x5832, 0x26 },
	{ 0x5833, 0x48 },
	{ 0x5834, 0x66 },
	{ 0x5835, 0x44 },
	{ 0x5836, 0x64 },
	{ 0x5837, 0x28 },
	{ 0x5838, 0x66 },
	{ 0x5839, 0x48 },
	{ 0x583a, 0x2c },
	{ 0x583b, 0x28 },
	{ 0x583c, 0x26 },
	{ 0x583d, 0xae },
	{ 0x5025, 0x00 },
	{ 0x3a0f, 0x30 },
	{ 0x3a10, 0x28 },
	{ 0x3a1b, 0x30 },
	{ 0x3a1e, 0x26 },
	{ 0x3a11, 0x60 },
	{ 0x3a1f, 0x14 },
	{ 0x0601, 0x02 },
	{ 0x3008, 0x42 },
	{ 0x3008, 0x02 },
	{ OV5645_IO_MIPI_CTRL00, 0x40 },
	{ REG_DLY, 0x05 },
	{ OV5645_MIPI_CTRL00, 0x24 },
	{ OV5645_PAD_OUTPUT00, 0x70 }
};

static struct regval_list ov5645_2592_1944[] = {
	{ 0x3612, 0xab },
	{ 0x3614, 0x50 },
	{ 0x3618, 0x04 },
	{ 0x3034, 0x18 },
	{ 0x3035, 0x11 },
	{ 0x3036, 0x54 },
	{ 0x3600, 0x08 },
	{ 0x3601, 0x33 },
	{ 0x3708, 0x63 },
	{ 0x370c, 0xc0 },
	{ 0x3800, 0x00 },
	{ 0x3801, 0x00 },
	{ 0x3802, 0x00 },
	{ 0x3803, 0x00 },
	{ 0x3804, 0x0a },
	{ 0x3805, 0x3f },
	{ 0x3806, 0x07 },
	{ 0x3807, 0x9f },
	{ 0x3808, 0x0a },
	{ 0x3809, 0x20 },
	{ 0x380a, 0x07 },
	{ 0x380b, 0x98 },
	{ 0x380c, 0x0b },
	{ 0x380d, 0x1c },
	{ 0x380e, 0x07 },
	{ 0x380f, 0xb0 },
	{ 0x3813, 0x06 },
	{ 0x3814, 0x11 },
	{ 0x3815, 0x11 },
	{ 0x3820, 0x47 },
	{ 0x4514, 0x88 },
	{ 0x3a02, 0x07 },
	{ 0x3a03, 0xb0 },
	{ 0x3a08, 0x01 },
	{ 0x3a09, 0x27 },
	{ 0x3a0a, 0x00 },
	{ 0x3a0b, 0xf6 },
	{ 0x3a0e, 0x06 },
	{ 0x3a0d, 0x08 },
	{ 0x3a14, 0x07 },
	{ 0x3a15, 0xb0 },
	{ 0x3a18, 0x01 },
	{ 0x4004, 0x06 },
	{ 0x4005, 0x18 },
	{ 0x4300, 0x32 },
	{ 0x4837, 0x0b },
	{ 0x4202, 0x00 }
};

static struct regval_list ov5645_1080p[] = {
	{ 0x3612, 0xab },
	{ 0x3614, 0x50 },
	{ 0x3618, 0x04 },
	{ 0x3034, 0x18 },
	{ 0x3035, 0x11 },
	{ 0x3036, 0x54 },
	{ 0x3600, 0x08 },
	{ 0x3601, 0x33 },
	{ 0x3708, 0x63 },
	{ 0x370c, 0xc0 },
	{ 0x3800, 0x01 },
	{ 0x3801, 0x50 },
	{ 0x3802, 0x01 },
	{ 0x3803, 0xb2 },
	{ 0x3804, 0x08 },
	{ 0x3805, 0xef },
	{ 0x3806, 0x05 },
	{ 0x3807, 0xf1 },
	{ 0x3808, 0x07 },
	{ 0x3809, 0x80 },
	{ 0x380a, 0x04 },
	{ 0x380b, 0x38 },
	{ 0x380c, 0x09 },
	{ 0x380d, 0xc4 },
	{ 0x380e, 0x04 },
	{ 0x380f, 0x60 },
	{ 0x3813, 0x04 },
	{ 0x3814, 0x11 },
	{ 0x3815, 0x11 },
	{ 0x3820, 0x47 },
	{ 0x4514, 0x88 },
	{ 0x3a02, 0x04 },
	{ 0x3a03, 0x60 },
	{ 0x3a08, 0x01 },
	{ 0x3a09, 0x50 },
	{ 0x3a0a, 0x01 },
	{ 0x3a0b, 0x18 },
	{ 0x3a0e, 0x03 },
	{ 0x3a0d, 0x04 },
	{ 0x3a14, 0x04 },
	{ 0x3a15, 0x60 },
	{ 0x3a18, 0x00 },
	{ 0x4004, 0x06 },
	{ 0x4005, 0x18 },
	{ 0x4300, 0x32 },
	{ 0x4202, 0x00 },
	{ 0x4837, 0x0b }
};

static struct regval_list ov5645_960p[] = {
	{ 0x3612, 0xa9 },
	{ 0x3614, 0x50 },
	{ 0x3618, 0x00 },
	{ 0x3034, 0x18 },
	{ 0x3035, 0x21 },
	{ 0x3036, 0x70 },
	{ 0x3600, 0x09 },
	{ 0x3601, 0x43 },
	{ 0x3708, 0x66 },
	{ 0x370c, 0xc3 },
	{ 0x3800, 0x00 },
	{ 0x3801, 0x00 },
	{ 0x3802, 0x00 },
	{ 0x3803, 0x06 },
	{ 0x3804, 0x0a },
	{ 0x3805, 0x3f },
	{ 0x3806, 0x07 },
	{ 0x3807, 0x9d },
	{ 0x3808, 0x05 },
	{ 0x3809, 0x00 },
	{ 0x380a, 0x03 },
	{ 0x380b, 0xc0 },
	{ 0x380c, 0x07 },
	{ 0x380d, 0x68 },
	{ 0x380e, 0x03 },
	{ 0x380f, 0xd8 },
	{ 0x3813, 0x06 },
	{ 0x3814, 0x31 },
	{ 0x3815, 0x31 },
	{ 0x3820, 0x47 },
	{ 0x3a02, 0x03 },
	{ 0x3a03, 0xd8 },
	{ 0x3a08, 0x01 },
	{ 0x3a09, 0xf8 },
	{ 0x3a0a, 0x01 },
	{ 0x3a0b, 0xa4 },
	{ 0x3a0e, 0x02 },
	{ 0x3a0d, 0x02 },
	{ 0x3a14, 0x03 },
	{ 0x3a15, 0xd8 },
	{ 0x3a18, 0x00 },
	{ 0x4004, 0x02 },
	{ 0x4005, 0x18 },
	{ 0x4300, 0x32 },
	{ 0x4202, 0x00 }
};

static struct regval_list ov5645_reg_stop_stream[] = {
	{OV5645_IO_MIPI_CTRL00, 0x40},
	{OV5645_SYSTEM_CTRL0, OV5645_SYSTEM_CTRL0_STOP},
};

static struct regval_list ov5645_reg_start_stream[] = {
	{ OV5645_IO_MIPI_CTRL00, 0x45},
	{ OV5645_SYSTEM_CTRL0, OV5645_SYSTEM_CTRL0_START},
};

static struct regval_list sensor_fmt_yuv422_yuyv[] = {
	{0x4300, 0x30},
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
	{0x4300, 0x32},
};

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	return 0;
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
		case STBY_ON:
			sensor_s_sw_stby(sd, ON);
			break;
		case STBY_OFF:
			sensor_s_sw_stby(sd, OFF);
			break;

		case PWR_ON:
			cci_lock(sd);
			vin_gpio_set_status(sd, RESET, 1);
			vin_gpio_set_status(sd, PWDN, 1);

			vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
			vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
			usleep_range(5000, 6000);

			vin_set_mclk_freq(sd, MCLK);
			vin_set_mclk(sd, ON);
			vin_set_pmu_channel(sd, CAMERAVDD, ON);
			vin_set_pmu_channel(sd, IOVDD, ON);
			vin_set_pmu_channel(sd, DVDD, ON);
			vin_set_pmu_channel(sd, AVDD, ON);
			usleep_range(5000, 6000);
			vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
			usleep_range(5000, 5100);
			vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
			usleep_range(5000, 5100);
			vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
			usleep_range(30000, 31000);
			cci_unlock(sd);
			break;
		case PWR_OFF:
			cci_lock(sd);
			vin_set_mclk(sd, OFF);

			vin_set_pmu_channel(sd, CAMERAVDD, OFF);
			vin_set_pmu_channel(sd, IOVDD, OFF);
			vin_set_pmu_channel(sd, DVDD, OFF);
			vin_set_pmu_channel(sd, AVDD, OFF);

			vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
			usleep_range(1000, 2000);
			vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
			usleep_range(1000, 2000);
			cci_unlock(sd);

			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

int ov5645_read(struct v4l2_subdev *sd, data_type reg, data_type *val)
{
	return cci_read(sd, reg, val);
}

int ov5645_write(struct v4l2_subdev *sd, data_type reg, data_type val)
{
	return cci_write(sd, reg, val);
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret = 0;
	unsigned short id;
	data_type pid, ver;

	sensor_write_array(sd, ov5645_init_regs_5645_mipi, \
			sizeof(ov5645_init_regs_5645_mipi) / sizeof(ov5645_init_regs_5645_mipi[0]));
	
	usleep_range(500, 1000);

	ov5645_read(sd, REG_CHIP_ID_HIGH, &pid);
	ov5645_read(sd, REG_CHIP_ID_LOW, &ver);

	id = OV564X_ID(pid, ver);
	printk("ov5645 id:%x\n", id);
	if (id != OV5645_ID) {
		ret = -ENODEV;
	} 
	return ret;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0; 
	info->low_speed = 0; 
	info->width = 1920; 
	info->height = 1080; 
	info->brightness = 0; 
	info->contrast = 0; 
	info->saturation = 0; 
	info->hue = 0; 
	info->hflip = 0; 
	info->vflip = 0; 
	info->gain = 0; 
//	info->autogain = 1; 
//	info->exp_bias = 0; 
//	info->autoexp = 1; 
//	info->autowb = 1; 

//	info->wb = V4L2_WHITE_BALANCE_AUTO;
//	info->clrfx = V4L2_COLORFX_NONE;
//	info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;
//	info->preview_first_flag = 1;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
		struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "YUYV 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
		.regs = sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp = 2,
	}, {
		.desc = "UYVY 4:2:2",
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.regs = sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp = 2,
	} 
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	{
		.width = 1280,
		.height = 960,
		.hoffset = 0, 
		.voffset = 0, 
		.fps_fixed = 30, 
		.regs = ov5645_960p,
		.regs_size = ARRAY_SIZE(ov5645_960p),
		.set_size = NULL,
	},
	{
		.width = 1920,
		.height = 1080,
		.hoffset = 0,
		.voffset = 0,
		.hts        = 2500,
		.vts        = 1120,
		.pclk       = 84 * 1000 * 1000,	//hts * vts * fps
		.mipi_bps   = 210 * 1000 * 1000,	// pclk * bit / lanes /2
		.fps_fixed = 30, 
		.regs = ov5645_1080p,
		.regs_size = ARRAY_SIZE(ov5645_1080p),
		.set_size = NULL,
	},
	{
		.width = 2592,
		.height = 1944,
		.hoffset = 0,
		.voffset = 0,
		.fps_fixed = 30, 
		.regs = ov5645_2592_1944,
		.regs_size = ARRAY_SIZE(ov5645_2592_1944),
		.set_size = NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))


static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
		struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_2_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;

	return 0;
}


static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_subdev *ov5645_sd = sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	if (!enable) {
		sensor_write_array(ov5645_sd, ov5645_reg_stop_stream, \
				sizeof(ov5645_reg_stop_stream) / sizeof(ov5645_reg_stop_stream[0]));
	} else {
		printk("===========%s width: %d height:%d\n", __func__, wsize->width, wsize->height);
		if (wsize->regs)
			sensor_write_array(ov5645_sd, wsize->regs, wsize->regs_size);

		if (wsize->set_size)
			wsize->set_size(ov5645_sd);

		if (sensor_fmt->regs) 
			sensor_write_array(ov5645_sd, sensor_fmt->regs, sensor_fmt->regs_size);

		sensor_write_array(ov5645_sd, ov5645_reg_start_stream, \
				sizeof(ov5645_reg_start_stream) / sizeof(ov5645_reg_start_stream[0]));
	}

	return 0;
}


static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	switch (ctrl->id) {
		case V4L2_CID_TEST_PATTERN:
			break;
	}
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	return -EINVAL;
}


static const struct v4l2_ctrl_ops sensor_ctrl_ops = { 
	.s_ctrl = sensor_s_ctrl,
	.g_volatile_ctrl = sensor_g_ctrl,

};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,	//override
	.init = sensor_init,	//override
	.s_power = sensor_power,	// override
	.ioctl = sensor_ioctl,		//override
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,	//override
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,	// override
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,		//芯片寄存器地址位数
	.data_width = CCI_BITS_8,		//芯片寄存器数据位数
};
#if 0
static void set_cci_driver(struct cci_driver *cci_drv, int index)
{
	snprintf(cci_drv->name, sizeof(cci_drv->name), "%s_%d", SENSOR_NAME, index);
	cci_drv->addr_width = CCI_BITS_16;
	cci_drv->data_width = CCI_BITS_8;
}
#endif

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 1);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_PIXEL_RATE,
			1,
			0xFFFFFFFF, 1, 1);

	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}    

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}
	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

//	register_cci_cnt++;
	sd = &info->sd;

	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
/*
	set_cci_driver(&info->cci_drv, register_cci_cnt);
	cci_dev_probe_helper(sd, client, &sensor_ops, &info->cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
*/

	g_ov5645_sd = sd;

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->fmt_num = N_FMTS;
	info->win_pt = &sensor_win_sizes[0];
	info->win_size_num = N_WIN_SIZES;

#ifdef CONFIG_SAME_I2C
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif

	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE; //CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->time_hs = 0x30; //0xf6;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
