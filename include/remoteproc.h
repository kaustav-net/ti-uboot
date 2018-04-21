/*
 * (C) Copyright 2015
 * Texas Instruments Incorporated - http://www.ti.com/
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _RPROC_H_
#define _RPROC_H_

/*
 * Note: The platform data support is not meant for use with newer
 * platforms. This is meant only for legacy devices. This mode of
 * initialization *will* be eventually removed once all necessary
 * platforms have moved to dm/fdt.
 */
#include <dm/platdata.h>	/* For platform data support - non dt world */

/**
 * enum rproc_mem_type - What type of memory model does the rproc use
 * @RPROC_INTERNAL_MEMORY_MAPPED: Remote processor uses own memory and is memory
 *	mapped to the host processor over an address range.
 *
 * Please note that this is an enumeration of memory model of different types
 * of remote processors. Few of the remote processors do have own internal
 * memories, while others use external memory for instruction and data.
 */
enum rproc_mem_type {
	RPROC_INTERNAL_MEMORY_MAPPED	= 0,
};

/**
 * struct dm_rproc_uclass_pdata - platform data for a CPU
 * @name: Platform-specific way of naming the Remote proc
 * @mem_type: one of 'enum rproc_mem_type'
 * @driver_plat_data: driver specific platform data that may be needed.
 *
 * This can be accessed with dev_get_uclass_platdata() for any UCLASS_REMOTEPROC
 * device.
 *
 */
struct dm_rproc_uclass_pdata {
	const char *name;
	enum rproc_mem_type mem_type;
	void *driver_plat_data;
};

/**
 * struct dm_rproc_ops - Operations that are provided by remote proc driver
 * @init:	Initialize the remoteproc device invoked after probe (optional)
 *		Return 0 on success, -ve error on fail
 * @load:	Load the remoteproc device using data provided(mandatory)
 *		This takes the following additional arguments.
 *			addr- Address of the binary image to be loaded
 *			size- Size of the binary image to be loaded
 *		Return 0 on success, -ve error on fail
 * @start:	Start the remoteproc device (mandatory)
 *		Return 0 on success, -ve error on fail
 * @stop:	Stop the remoteproc device (optional)
 *		Return 0 on success, -ve error on fail
 * @reset:	Reset the remote proc device (optional)
 *		Return 0 on success, -ve error on fail
 * @is_running:	Check if the remote processor is running(optional)
 *		Return 0 on success, 1 if not running, -ve on others errors
 * @ping:	Ping the remote device for basic communication check(optional)
 *		Return 0 on success, 1 if not responding, -ve on other errors
 */
struct dm_rproc_ops {
	int (*init)(struct udevice *dev);
	int (*load)(struct udevice *dev, ulong addr, ulong size);
	int (*start)(struct udevice *dev);
	int (*stop)(struct udevice *dev);
	int (*reset)(struct udevice *dev);
	int (*is_running)(struct udevice *dev);
	int (*ping)(struct udevice *dev);
};

/* Accessor */
#define rproc_get_ops(dev) ((struct dm_rproc_ops *)(dev)->driver->ops)

#ifdef CONFIG_REMOTEPROC
/**
 * rproc_init() - Initialize all bound remote proc devices
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_init(void);

/**
 * rproc_dev_init() - Initialize a remote proc device based on id
 * @id:		id of the remote processor
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_dev_init(int id);

/**
 * rproc_is_initialized() - check to see if remoteproc devices are initialized
 *
 * Return: 0 if all devices are initialized, else appropriate error value.
 */
bool rproc_is_initialized(void);

/**
 * rproc_load() - load binary to a remote processor
 * @id:		id of the remote processor
 * @addr:	address in memory where the binary image is located
 * @size:	size of the binary image
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_load(int id, ulong addr, ulong size);

/**
 * rproc_start() - Start a remote processor
 * @id:		id of the remote processor
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_start(int id);

/**
 * rproc_stop() - Stop a remote processor
 * @id:		id of the remote processor
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_stop(int id);

/**
 * rproc_reset() - reset a remote processor
 * @id:		id of the remote processor
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_reset(int id);

/**
 * rproc_ping() - ping a remote processor to check if it can communicate
 * @id:		id of the remote processor
 *
 * NOTE: this might need communication path available, which is not implemented
 * as part of remoteproc framework - hook on to appropriate bus architecture to
 * do the same
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_ping(int id);

/**
 * rproc_is_running() - check to see if remote processor is running
 * @id:		id of the remote processor
 *
 * NOTE: this may not involve actual communication capability of the remote
 * processor, but just ensures that it is out of reset and executing code.
 *
 * Return: 0 if all ok, else appropriate error value.
 */
int rproc_is_running(int id);
#else
static inline int rproc_init(void) { return -ENOSYS; }
static inline int rproc_dev_init(int id) { return -ENOSYS; }
static inline bool rproc_is_initialized(void) { return false; }
static inline int rproc_load(int id, ulong addr, ulong size) { return -ENOSYS; }
static inline int rproc_start(int id) { return -ENOSYS; }
static inline int rproc_stop(int id) { return -ENOSYS; }
static inline int rproc_reset(int id) { return -ENOSYS; }
static inline int rproc_ping(int id) { return -ENOSYS; }
static inline int rproc_is_running(int id) { return -ENOSYS; }
#endif

/**
 * struct proc_ctrl - A handle to Remote Processor control data
 * @dev:	Processor control device pointer
 * @id:		ID of the remote processor ID as
 *		understood by the Processor control
 * @data:	Private data of the remote processor (Optional)
 *
 * @id and @data is updated by the xlate api. This information is used by
 * other services provided by the processor control.
 */
struct proc_ctrl {
	struct udevice *dev;
	unsigned long id;
	unsigned long data;
};

/**
 * struct proc_ctrl_ops - Operations that are provided by the processor control
 *				driver.
 * @request:	Request for controlling the remote core.
 * @release:	Release the access to the remote core.
 * @init:	Initialize the remoteproc device.
 * @load:	Load the remoteproc device using data provided(mandatory)
 *		This takes the following additional arguments.
 *			addr- Address of the binary image to be loaded
 *			size- Size of the binary image to be loaded
 * @start:	Start the remoteproc device
 * @stop:	Stop the remoteproc device
 * @reset:	Reset the remote proc device
 * @is_running:	Check if the remote processor is running
 * @ping:	Ping the remote device for basic communication check
 * @of_xlate:	Convert the DT information to proc_ctrl structure.
 */
struct proc_ctrl_ops {
	int (*request)(struct proc_ctrl *ctrl);
	int (*release)(struct proc_ctrl *ctrl);
	int (*init)(struct proc_ctrl *ctrl);
	int (*load)(struct proc_ctrl *ctrl, ulong addr, ulong size);
	int (*start)(struct proc_ctrl *ctrl);
	int (*stop)(struct proc_ctrl *ctrl);
	int (*reset)(struct proc_ctrl *ctrl);
	int (*is_running)(struct proc_ctrl *ctrl);
	int (*ping)(struct proc_ctrl *ctrl);
	int (*of_xlate)(struct proc_ctrl *ctrl,
			struct ofnode_phandle_args *args);
};

int proc_ctrl_get_by_index(struct udevice *dev, int index,
			   struct proc_ctrl *proc);
/**
 * Processor control apis that act as wrapper between the remoteproc driver
 * and the processing entity that can control the remote core. All the services
 * that are provided by the remoteproc driver are support here, additionally
 * there are two other apis that are supported:
 * proc_ctrl_rproc_request() -  API to request access for controlling the
 *				remote processor.
 * proc_ctrl_rproc_release() - API to release access for controlling the
 *				remote core.
 * These apis should be called by the remoteproc driver in their respective
 * service calls.
 */
int proc_ctrl_rproc_request(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_release(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_init(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_load(struct proc_ctrl *ctrl, ulong addr, ulong size);
int proc_ctrl_rproc_start(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_stop(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_reset(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_is_running(struct proc_ctrl *ctrl);
int proc_ctrl_rproc_ping(struct proc_ctrl *ctrl);
#endif	/* _RPROC_H_ */
