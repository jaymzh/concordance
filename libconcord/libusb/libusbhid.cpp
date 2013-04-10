/*
 * vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright Kevin Timmerman 2007
 * (C) Copyright Phil Dibowitz 2007
 */

#include "../lc_internal.h"
#include "../libconcord.h"

#ifdef LIBUSB

#include "../hid.h"
#ifdef WIN32
#include "win/usb.h"
#else
#include <usb.h>
#endif
#include <errno.h>
#include <string.h>

/*
 * Harmonies either fall under logitech's VendorID (0x046d), and logitech's
 * productID range for Harmonies (0xc110 - 0xc14f)...
 *
 * OR, they fall under 0x400/0xc359 (older 7-series, all 6-series).
 */
#define LOGITECH_VID 0x046D
#define LOGITECH_MIN_PID 0xc110
#define LOGITECH_MAX_PID 0xc14f
#define NATIONAL_VID 0x0400
#define NATIONAL_PID 0xc359

static usb_dev_handle *h_hid = NULL;
static unsigned int irl;
static unsigned int orl;
static int ep_read = -1;
static int ep_write = -1;

int InitUSB()
{
	usb_init();
	return 0;
}

void ShutdownUSB()
{
	if (h_hid) {
		usb_release_interface(h_hid,0);
	}
}

void check_ep(usb_endpoint_descriptor &ued)
{
	debug("address %02X attrib %02X max_length %i",
		ued.bEndpointAddress, ued.bmAttributes,
		ued.wMaxPacketSize);

	if ((ued.bmAttributes & USB_ENDPOINT_TYPE_MASK) ==
	    USB_ENDPOINT_TYPE_INTERRUPT) {
		if (ued.bEndpointAddress & USB_ENDPOINT_DIR_MASK) {
			if (ep_read == -1) {
				ep_read = ued.bEndpointAddress;
				// hack! todo: get from HID report descriptor
				irl = ued.wMaxPacketSize;
			}
		} else {
			if (ep_write == -1) {
				ep_write = ued.bEndpointAddress;
				// hack! todo: get from HID report descriptor
				orl = ued.wMaxPacketSize;
			}
		}
	}
}

bool is_harmony(struct usb_device *h_dev)
{
	/* IF vendor == logitech AND product is in range of harmony
	 *   OR vendor == National Semiconductor and product is harmony
	 */
	if ((h_dev->descriptor.idVendor == LOGITECH_VID
	      && (h_dev->descriptor.idProduct >= LOGITECH_MIN_PID
	          && h_dev->descriptor.idProduct <= LOGITECH_MAX_PID))
	    || (h_dev->descriptor.idVendor == NATIONAL_VID
	          && h_dev->descriptor.idProduct == NATIONAL_PID)) {
		return true;
	}
	return false;
}

/*
 * Find a HID device with VendorID == 0x046D ||
 *    (VendorID == 0x0400 && ProductID == 0xC359)
 */
int FindRemote(THIDINFO &hid_info)
{

	usb_find_busses();
	usb_find_devices();

	struct usb_device *h_dev = NULL;
	bool found = false;
	for (usb_bus *bus = usb_busses; bus && !found; bus = bus->next) {
		for (h_dev = bus->devices; h_dev; h_dev = h_dev->next) {
			if (is_harmony(h_dev)) {
				found = true;
				break;
			}
		}
	}

	if (h_dev) {
		h_hid = usb_open(h_dev);
	}
	if (!h_hid) {
		debug("Failed to establish communication with remote: %s",
			usb_strerror());
		return LC_ERROR_CONNECT;
	}

#ifdef linux
	/*
	 * Before we attempt to claim the interface, lets go ahead and get
	 * the kernel off of it, in case it claimed it already.
	 *
	 * This is ONLY available when on Linux. We don't check for an error
	 * because it will error if no kernel driver is attached to it.
	 *
	 * Don't attempt to do this if this is a usbnet remote as it will
	 * unload the zaurus driver, which is not desired.
	 */
	if (!(h_dev && (h_dev->descriptor.idProduct == 0xC11F))) {
		usb_detach_kernel_driver_np(h_hid, 0);
	}
#endif

	int err;
	if ((err = usb_set_configuration(h_hid, 1))) {
		debug("Failed to set device configuration: %d (%s)", err,
			usb_strerror());
		return err;
	}

	if ((err=usb_claim_interface(h_hid, 0))) {
		debug("Failed to claim interface: %d (%s)", err,
			usb_strerror());
		return err;
	}

	unsigned char maxconf = h_dev->descriptor.bNumConfigurations;
	for (unsigned char j = 0; j < maxconf; ++j) {
		usb_config_descriptor &uc = h_dev->config[j];
		unsigned char maxint = uc.bNumInterfaces;
		for (unsigned char k = 0; k < maxint; ++k) {
			usb_interface &ui = uc.interface[k];
			unsigned char maxalt = ui.num_altsetting;
			for (unsigned char l = 0; l < maxalt; ++l) {
				usb_interface_descriptor &uid =
					ui.altsetting[l];

					debug("bNumEndpoints %i",
						uid.bNumEndpoints);
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
	hid_info.mfg = s;
	usb_get_string_simple(h_hid,h_dev->descriptor.iProduct,s,sizeof(s));
	hid_info.prod = s;

	hid_info.vid = h_dev->descriptor.idVendor;
	hid_info.pid = h_dev->descriptor.idProduct;
	hid_info.ver = h_dev->descriptor.bcdDevice;

	hid_info.irl = irl;
	hid_info.orl = orl;
	hid_info.frl = 0;/// ???

	return 0;
}

int HID_WriteReport(const uint8_t *data)
{
	/*
	 * In Windows you send an preceeding 0x00 byte with
	 * every command, and the codebase used to do that, and we'd
	 * skip the first byte here. Now, we do not assume this, we send
	 * wholesale here, and add the 0 in the windows code.
	 */
	const int err=usb_interrupt_write(h_hid, ep_write,
		reinterpret_cast<char *>(const_cast<uint8_t*>(data)),
		orl, 500);

	if (err < 0) {
		debug("Failed to write to device: %d (%s)", err,
			strerror(-err));
		return err;
	}

	return 0;
}

int HID_ReadReport(uint8_t *data, unsigned int timeout)
{
	/* Note default timeout is set to 500 in hid.h */
	const int err = usb_interrupt_read(h_hid, ep_read,
		reinterpret_cast<char *>(data), irl, timeout);

	if (err == -ETIMEDOUT) {
		debug("Timeout on interrupt read from device");
		return err;
	}

	if (err < 0) {
		debug("Failed to read from device: %d (%s)", err,
			usb_strerror());
		return err;
	}

	return 0;
}

#endif
