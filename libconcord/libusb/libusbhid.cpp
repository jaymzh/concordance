/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Kevin Timmerman 2007
 *  (C) Copyright Phil Dibowitz 2007
 */

#include "../harmony.h"

#ifdef LIBUSB

#include "../hid.h"
#ifdef WIN32
#include "win/usb.h"
#else
#include <usb.h>
#endif

static usb_dev_handle *h_hid=NULL;
static unsigned int irl;
static unsigned int orl;
static int ep_read=-1;
static int ep_write=-1;

int InitUSB(void)
{
	usb_init();
	return 0;
}

void ShutdownUSB(void)
{
	if (h_hid) {
		usb_release_interface(h_hid,0);
	}
}

void check_ep(usb_endpoint_descriptor &ued)
{
	printf("address %02X attrib %02X max_length %i\n", ued.bEndpointAddress,
		ued.bmAttributes, ued.wMaxPacketSize);
	if ((ued.bmAttributes & USB_ENDPOINT_TYPE_MASK) ==
	    USB_ENDPOINT_TYPE_INTERRUPT) {
		if (ued.bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
			if (ep_read == -1) {
				ep_read=ued.bEndpointAddress;
				// hack! todo: get from HID report descriptor
				irl=ued.wMaxPacketSize;
			}
		} else {
			if (ep_write == -1) {
				ep_write=ued.bEndpointAddress;
				// hack! todo: get from HID report descriptor
				orl=ued.wMaxPacketSize;
			}
		}
	}
}

/*
 * Find a HID device with VendorID == 0x046D ||
 *    (VendorID == 0x0400 && ProductID == 0xC359)
 */
int FindRemote(THIDINFO &hid_info)
{

	usb_find_busses();
	usb_find_devices();

	struct usb_device *h_dev;
	bool found=false;
	for (usb_bus *bus = usb_busses; bus && !found; bus = bus->next)
		for (h_dev = bus->devices; h_dev; h_dev = h_dev->next)
			if (h_dev->descriptor.idVendor == 0x046D
			    || (h_dev->descriptor.idVendor == 0x0400
			       && h_dev->descriptor.idProduct == 0xC359)) {
				found=true;
				break;
			}

	if (h_dev) {
		h_hid=usb_open(h_dev);
	}
	if (!h_hid) {
		printf("Failed to establish communication with remote\n");
		return 2;
	}
	usb_set_configuration(h_hid,1);

	int err;
	if ((err=usb_claim_interface(h_hid,0)))
		return err;

	unsigned char maxconf = h_dev->descriptor.bNumConfigurations;
	for (unsigned char j = 0; j < maxconf; ++j) {
		usb_config_descriptor &uc=h_dev->config[j];
		unsigned char maxint = uc.bNumInterfaces;
		for (unsigned char k = 0; k < maxint; ++k) {
			usb_interface &ui=uc.interface[k];
			unsigned char maxalt = ui.num_altsetting;
			for (unsigned char l = 0; l < maxalt; ++l) {
				usb_interface_descriptor &uid=ui.altsetting[l];
				printf("bNumEndpoints %i\n",uid.bNumEndpoints);
				unsigned char maxep = uid.bNumEndpoints;
				for (unsigned char n = 0; n < maxep; ++n) {
					check_ep(uid.endpoint[n]);

				}
			}
		}
	}

	if (ep_read == -1 || ep_write == -1) return 1;

	// Fill in hid_info

	char s[128];
	usb_get_string_simple(h_hid,h_dev->descriptor.iManufacturer,s,sizeof(s));
	hid_info.mfg=s;
	usb_get_string_simple(h_hid,h_dev->descriptor.iProduct,s,sizeof(s));
	hid_info.prod=s;

	hid_info.vid=h_dev->descriptor.idVendor;
	hid_info.pid=h_dev->descriptor.idProduct;
	hid_info.ver=h_dev->descriptor.bcdDevice; // ???

	hid_info.irl=irl;
	hid_info.orl=orl;
	hid_info.frl=0;	/// ???

	return 0;
}

int HID_WriteReport(const uint8_t *data)
{
	const int err=usb_interrupt_write(h_hid, ep_write,
		reinterpret_cast<char *>(const_cast<uint8_t*>(data+1)),
		orl, 500);

	return (err < 0) ? err : 0;
}

int HID_ReadReport(uint8_t *data, unsigned int timeout)
{
	const int err=usb_interrupt_read(h_hid, ep_read,
		reinterpret_cast<char *>(data+1), irl, timeout);

	return (err < 0) ? err : 0;
}

#endif
