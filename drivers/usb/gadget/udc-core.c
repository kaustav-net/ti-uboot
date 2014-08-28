/**
 * udc-core.c - Core UDC Framework
 *
 * Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * Taken from Linux Kernel v3.16 (drivers/usb/gadget/udc-core.c) and ported
 * to uboot.
 *
 * commit b5fb8d0 : usb: udc-core: set gadget state as not attached after
 *		    unloading module
 *
 * SPDX-License-Identifier:     GPL-2.0
 */

#include <linux/compat.h>
#include <malloc.h>
#include <asm/cache.h>
#include <asm/dma-mapping.h>
#include <common.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

/**
 * struct usb_udc - describes one usb device controller
 * @driver - the gadget driver pointer. For use by the class code
 * @dev - the child device to the actual controller
 * @gadget - the gadget. For use by the class code
 * @list - for use by the udc class driver
 *
 * This represents the internal data structure which is used by the UDC-class
 * to hold information about udc driver and gadget together.
 */
struct usb_udc {
	struct usb_gadget_driver	*driver;
	struct usb_gadget		*gadget;
	struct list_head		list;
};

static LIST_HEAD(udc_list);

int usb_gadget_map_request(struct usb_gadget *gadget,
			   struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return 0;

	req->dma = dma_map_single(req->buf, req->length,
				  is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);

	return 0;
}

void usb_gadget_unmap_request(struct usb_gadget *gadget,
			      struct usb_request *req, int is_in)
{
	if (req->length == 0)
		return;

	dma_unmap_single((void *)req->dma, req->length,
			 is_in ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
}

void usb_gadget_set_state(struct usb_gadget *gadget,
			  enum usb_device_state state)
{
	gadget->state = state;
}

/**
 * usb_gadget_udc_start - tells usb device controller to start up
 * @gadget: The gadget we want to get started
 * @driver: The driver we want to bind to @gadget
 *
 * This call is issued by the UDC Class driver when it's about
 * to register a gadget driver to the device controller, before
 * calling gadget driver's bind() method.
 *
 * It allows the controller to be powered off until strictly
 * necessary to have it powered on.
 *
 * Returns zero on success, else negative errno.
 */
static inline int usb_gadget_udc_start(struct usb_gadget *gadget,
				       struct usb_gadget_driver *driver)
{
	return gadget->ops->udc_start(gadget, driver);
}

/**
 * usb_gadget_udc_stop - tells usb device controller we don't need it anymore
 * @gadget: The device we want to stop activity
 * @driver: The driver to unbind from @gadget
 *
 * This call is issued by the UDC Class driver after calling
 * gadget driver's unbind() method.
 *
 * The details are implementation specific, but it can go as
 * far as powering off UDC completely and disable its data
 * line pullups.
 */
static inline void usb_gadget_udc_stop(struct usb_gadget *gadget,
				       struct usb_gadget_driver *driver)
{
	gadget->ops->udc_stop(gadget, driver);
}

/**
 * usb_add_gadget_udc_release - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller driver's
 * device.
 * @gadget: the gadget to be added to the list.
 * @release: a gadget release function.
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc_release(struct usb_gadget *gadget)
{
	struct usb_udc		*udc;
	int			ret = -ENOMEM;

	udc = kzalloc(sizeof(*udc), GFP_KERNEL);
	if (!udc)
		return ret;

	udc->gadget = gadget;
	list_add_tail(&udc->list, &udc_list);
	usb_gadget_set_state(gadget, USB_STATE_NOTATTACHED);
	return 0;
}

/**
 * usb_add_gadget_udc - adds a new gadget to the udc class driver list
 * @parent: the parent device to this udc. Usually the controller
 * driver's device.
 * @gadget: the gadget to be added to the list
 *
 * Returns zero on success, negative errno otherwise.
 */
int usb_add_gadget_udc(struct usb_gadget *gadget)
{
	return usb_add_gadget_udc_release(gadget);
}

static void usb_gadget_remove_driver(struct usb_udc *udc)
{
	dev_dbg(&udc->dev, "unregistering UDC driver [%s]\n",
		udc->gadget->name);
	usb_gadget_disconnect(udc->gadget);
	udc->driver->disconnect(udc->gadget);
	udc->driver->unbind(udc->gadget);
	usb_gadget_udc_stop(udc->gadget, NULL);

	udc->driver = NULL;
}

/**
 * usb_del_gadget_udc - deletes @udc from udc_list
 * @gadget: the gadget to be removed.
 *
 * This, will call usb_gadget_unregister_driver() if
 * the @udc is still busy.
 */
void usb_del_gadget_udc(struct usb_gadget *gadget)
{
	struct usb_udc		*udc = NULL;

	list_for_each_entry(udc, &udc_list, list)
		if (udc->gadget == gadget)
			goto found;

	dev_err(gadget->dev.parent, "gadget not registered.\n");
	return;

found:
	dev_vdbg(gadget->dev.parent, "unregistering gadget\n");

	list_del(&udc->list);

	if (udc->driver)
		usb_gadget_remove_driver(udc);

	kfree(udc);
}

static int udc_bind_to_driver(struct usb_udc *udc,
			      struct usb_gadget_driver *driver)
{
	int ret;

	dev_dbg(&udc->dev, "registering UDC driver [%s]\n",
		driver->function);

	udc->driver = driver;

	ret = driver->bind(udc->gadget);
	if (ret)
		goto err1;

	ret = usb_gadget_udc_start(udc->gadget, driver);
	if (ret) {
		driver->unbind(udc->gadget);
		goto err1;
	}
	usb_gadget_connect(udc->gadget);

	return 0;
err1:
	if (ret != -EISNAM)
		dev_err(&udc->dev, "failed to start %s: %d\n",
			udc->driver->function, ret);
	udc->driver = NULL;
	return ret;
}

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret;

	if (!driver || !driver->bind || !driver->setup)
		return -EINVAL;

	list_for_each_entry(udc, &udc_list, list) {
		/* For now we take the first one */
		if (!udc->driver)
			goto found;
	}

	debug("couldn't find an available UDC\n");
	return -ENODEV;
found:
	ret = udc_bind_to_driver(udc, driver);
	return ret;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	struct usb_udc		*udc = NULL;
	int			ret = -ENODEV;

	if (!driver || !driver->unbind)
		return -EINVAL;

	list_for_each_entry(udc, &udc_list, list)
		if (udc->driver == driver) {
			usb_gadget_remove_driver(udc);
			usb_gadget_set_state(udc->gadget,
					     USB_STATE_NOTATTACHED);
			ret = 0;
			break;
		}

	return ret;
}
