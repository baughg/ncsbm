#include "myriad_usb.h"
#include "file_util.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>




//location  variable
libusb_context *mCtx = NULL;
libusb_device **mDevs;
libusb_device_handle *mUsbHandle; //a device handle

uint16_t idVendor;
/** USB-IF product ID */
uint16_t idProduct;

uint16_t  valid_device_list[MAX_NUM_DEVICES][2] =
{
  {M2_VID,M2_PID},
  {NCS_VID, NCS_PID},   
  {0,0}
};

static unsigned int bulk_chunk_len = DEFAULT_CHUNK_SZ;
static int write_timeout = DEFAULT_WRITE_TIMEOUT;
static int connect_timeout = DEFAULT_CONNECT_TIMEOUT;
static const int mvnc_loglevel = 2;

typedef struct timespec highres_time_t;

static inline void highres_gettime(highres_time_t *ptr)
{
  clock_gettime(CLOCK_REALTIME, ptr);
}

static inline double highres_elapsed_ms(highres_time_t *start, highres_time_t *end)
{
  struct timespec temp;
  if ((end->tv_nsec - start->tv_nsec) < 0) {
    temp.tv_sec = end->tv_sec - start->tv_sec - 1;
    temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
  } else {
    temp.tv_sec = end->tv_sec - start->tv_sec;
    temp.tv_nsec = end->tv_nsec - start->tv_nsec;
  }
  return (double)(temp.tv_sec * 1000) + (((double)temp.tv_nsec) * 0.000001);
}

int main()
{
  printf("Searching for Myriad 2 device...\n");

  if(!myriad_connect())
  {
    std::vector<uint8_t> mvcmd_image;
    if(read_file("MvNCAPI.mvcmd",mvcmd_image)) {
      printf("Boot image size: %lu bytes\n",mvcmd_image.size());

      send_file(
        mUsbHandle, 
        (uint8_t)USB_ENDPOINT_OUT,
        (const uint8_t *)&mvcmd_image[0], 
        (unsigned)mvcmd_image.size());

      usleep(100000);
      myriad_close();
      usleep(1000000);
      
      if(!myriad_connect()) {
        myriadStatus_t status = MYRIAD_INITIALIZED;
        printf("Connected to NCS!\n");

        do {
          if (get_myriad_status(&status))
            break;
          usleep(10000);
        } while (status != MYRIAD_WAITING && status != MYRIAD_FINISHED);

        printf("status: 0x%.2X\n",(uint32_t)status);
        reset_myriad();
        usleep(100000);
        myriad_close();
      }

    }
    
    
  }
  return 0;
}

//public function
int  myriad_connect()
{
  int rv = LDV_SUCCESS;
  int i;
  //for return values
  int r;
  //holding number of devices in list
  ssize_t cnt;

  r = libusb_init(&mCtx);
  if(r < 0)
  	return LDV_ERROR_IO;

  //set verbosity level to 3, as suggested in the documentation
  libusb_set_debug(mCtx, 3);
  //get the list of devices
  cnt = libusb_get_device_list(mCtx, &mDevs);
  printf("USB device count: %lu\n",cnt);
  if(cnt < 0) {
  	return cnt;
  }
  rv = get_match_device(&idVendor,&idProduct,mDevs,cnt);
  if(rv != LDV_SUCCESS)
  	return rv;

  //these are vendorID and productID I found for my usb device
  mUsbHandle = libusb_open_device_with_vid_pid(mCtx, idVendor, idProduct); 
  if(mUsbHandle == NULL)
  	printf("Cannot open device\n");
  else
  	printf("Device Opened\n");

  if(mUsbHandle)
  {
  	libusb_claim_interface(mUsbHandle,0);
  	return LDV_SUCCESS;
  }
  return rv;
}

int  myriad_close()
{
  if(mUsbHandle)
  {
    libusb_release_interface(mUsbHandle,0);
    libusb_close(mUsbHandle);
  }
  libusb_free_device_list(mDevs, 1); //free the list, unref the devices in it
  libusb_exit(mCtx); //close the session
  mCtx = NULL;
}


int get_match_device(uint16_t *vid,uint16_t *pid,libusb_device **dev,uint16_t cnt)
{
	int i,j;
	struct libusb_device_descriptor desc;

	for(i = 0; i < cnt; i++)
	{
		int r = libusb_get_device_descriptor(dev[i], &desc);
		if (r < 0)
		{
			printf("failed to get device descriptor\n");
			return LDV_ERROR_IO;
		}
		for(j=0;j<MAX_NUM_DEVICES;j++)
		{
			if(desc.idVendor == valid_device_list[j][0] && desc.idProduct == valid_device_list[j][1])
			{
				*vid = valid_device_list[j][0];
				*pid = valid_device_list[j][1];
        print_device(dev[i]);
				return LDV_SUCCESS;
			}
		}
	}
	return LDV_ERROR_NO_DEVICE;
}

void print_device(libusb_device *dev) {
  struct libusb_device_descriptor desc;
  int i,j,k;
  int r = libusb_get_device_descriptor(dev, &desc);
  if (r < 0) {
    printf("failed to get device descriptor\n");
    return;
  }
  struct libusb_config_descriptor *config;
  libusb_get_config_descriptor(dev, 0, &config);
  //printf("%s-%d\n",__FUNCTION__,__LINE__);
    if(desc.idVendor == M2_VID && desc.idProduct == M2_PID)
  {
    printf("Number of possible configurations: %d\n",(int)desc.bNumConfigurations);
    printf("Device Class: %d\n",(int)desc.bDeviceClass);
    printf("VendorID: %x\n",desc.idVendor);
    printf("ProductID: %x\n",desc.idProduct);
    printf("Interfaces: %d\n",(int)config->bNumInterfaces);
    const struct libusb_interface *inter;
    const struct libusb_interface_descriptor *interdesc;
    const struct libusb_endpoint_descriptor *epdesc;

    for( i=0; i<(int)config->bNumInterfaces; i++) {
      inter = &config->interface[i];
      printf("Number of alternate settings: %d\n",inter->num_altsetting);
      for(j=0; j<inter->num_altsetting; j++) {
        interdesc = &inter->altsetting[j];
        
        printf("Interface Number: %d\n",(int)interdesc->bInterfaceNumber);
        printf("Number of endpoints:%d\n ",(int)interdesc->bNumEndpoints);

        for(k=0; k<(int)interdesc->bNumEndpoints; k++) {
          epdesc = &interdesc->endpoint[k];
          printf("Enpoint : %d\n",k);
          printf("Descriptor Type: %d\n",(int)epdesc->bDescriptorType);
          printf("EP Address: 0x%x\n",(int)epdesc->bEndpointAddress);
        
        }
    }
  }
}
libusb_free_config_descriptor(config);
}

int usb_write(libusb_device_handle *f, unsigned char *p_data, size_t size)
{
  while (size > 0) {
    int bt, ss = size;

    if (ss > USB_MAX_PACKET_SIZE)
      ss = USB_MAX_PACKET_SIZE;
   
    if (libusb_bulk_transfer(f, USB_ENDPOINT_OUT, (unsigned char *) p_data, ss, &bt,USB_TIMEOUT))
      return -1;

    p_data = (unsigned char *) p_data + bt;
    size -= bt;
  }

  printf("usb_write success!\n");
  return 0;
}

int usb_read(libusb_device_handle *f, unsigned char *p_data, size_t size)
{
  while (size > 0) {
    int bt, ss = size;
    if (ss > USB_MAX_PACKET_SIZE)
      ss = USB_MAX_PACKET_SIZE;

    if (libusb_bulk_transfer(f, USB_ENDPOINT_IN, p_data, ss, &bt, USB_TIMEOUT))
      return -1;
    p_data = (unsigned char *) p_data + bt;
    size -= bt;
  }

  printf("usb_read success!\n");
  return 0;
}

int send_file(libusb_device_handle * h, uint8_t endpoint,
         const uint8_t * tx_buf, unsigned file_size)
{
  const uint8_t *p;
  int rc;
  int wb, twb, wbr;
  double elapsed_time;
  highres_time_t t1, t2;

  elapsed_time = 0;
  twb = 0;
  p = tx_buf;
  PRINT_DEBUG(stderr, "Performing bulk write of %u bytes...\n",
        file_size);

  while (twb < file_size) {
    highres_gettime(&t1);
    wb = file_size - twb;
    if (wb > bulk_chunk_len)
      wb = bulk_chunk_len;
    wbr = 0;
    rc = libusb_bulk_transfer(h, endpoint, (unsigned char *) p, wb, &wbr,
            write_timeout);

    if (rc || (wb != wbr)) {
      if (rc == LIBUSB_ERROR_NO_DEVICE)
        break;

      PRINT_INFO(stderr,
           "bulk write: %s (%d bytes written, %d bytes to write)\n",
           libusb_strerror((libusb_error)rc), wbr, wb);
      if (rc == LIBUSB_ERROR_TIMEOUT)
        return MVNC_TIMEOUT;
      else
        return MVNC_ERROR;
    }
    highres_gettime(&t2);
    elapsed_time += highres_elapsed_ms(&t1, &t2);
    twb += wbr;
    p += wbr;
  }
  PRINT_DEBUG(stderr,
        "Successfully sent %u bytes of data in %lf ms (%lf MB/s)\n",
        file_size, elapsed_time,
        ((double) file_size / 1048576.) / (elapsed_time * 0.001));
  return 0;
}

int reset_myriad()
{
  usbHeader_t header;
  memset(&header, 0, sizeof(header));
  header.cmd = USB_LINK_RESET_REQUEST;
  if (usb_write(mUsbHandle, (unsigned char*)&header, sizeof(header)))
    return -1;
  return 0;
}

int get_myriad_status(myriadStatus_t* myriad_state)
{
  usbHeader_t header;
  memset(&header, 0, sizeof(header));
  header.cmd = USB_LINK_GET_MYRIAD_STATUS;

  if (usb_write(mUsbHandle, (unsigned char*)&header, sizeof(header))) {     
    return -1;
  }
  
  return usb_read(mUsbHandle, (unsigned char*)myriad_state, sizeof(*myriad_state));
}