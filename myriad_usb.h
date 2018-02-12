#ifndef MYRIAD_USB_H
#define MYRIAD_USB_H
#include "common.h"
#include "USBLinkDefines.h"
#include <stdint.h>
#include <libusb-1.0/libusb.h>

#define LDV_SUCCESS 			0
#define LDV_ERROR_IO 			1
#define LDV_ERROR_NO_DEVICE 	2
#define MAX_NUM_DEVICES 		3

#define M2_VID 0x03e7
#define M2_PID 0x2150
#define NCS_VID 0x03e7
#define NCS_PID 0xf63b

#define USB_ENDPOINT_IN 	0x81
#define USB_ENDPOINT_OUT 	0x01
#define USB_TIMEOUT 		10000
#define USB_MAX_PACKET_SIZE	1024 * 1024

#define DEFAULT_WRITE_TIMEOUT		2000
#define DEFAULT_CONNECT_TIMEOUT		20	// in 100ms units
#define DEFAULT_CHUNK_SZ			1024 * 1024

#define MVNC_TIMEOUT 		1
#define MVNC_ERROR			2

int myriad_connect();
int myriad_close();
int get_match_device(uint16_t *vid,uint16_t *pid,libusb_device **dev,uint16_t cnt);
void print_device(libusb_device *dev);
int usb_read(libusb_device_handle *f, unsigned char *p_data, size_t size);
int usb_write(libusb_device_handle *f, unsigned char *p_data, size_t size);
int send_file(libusb_device_handle * h, uint8_t endpoint,
         const uint8_t * tx_buf, unsigned file_size);
int reset_myriad();
int get_myriad_status(myriadStatus_t* myriad_state);
#endif