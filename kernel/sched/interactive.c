// SPDX-License-Identifier: GPL-2.0
/*
 * Kernel scheduler interactive extension
 * Copyright (C) 2022 ederekun <sedrickvince@gmail.com>.
 */

#include <linux/input.h>

#include "sched.h"

/* Allow boosting to occur within this time frame from last input update */
#define SCHED_INTERACTIVE_INPUT_NS (5000 * NSEC_PER_MSEC)

/* Keep track of interactivity */
DEFINE_LW_TIMEOUT(sched_interactive_lwt, SCHED_INTERACTIVE_INPUT_NS);

static void sched_interactive_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	sched_interactive(update_ts);
}

static int sched_interactive_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "sched_interactive";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void sched_interactive_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id sched_interactive_ids[] = {
	/* Multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y) }
	},
	/* Touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	},
	/* Keypad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ }
};

static struct input_handler sched_interactive_handler = {
	.event		   = sched_interactive_event,
	.connect	   = sched_interactive_connect,
	.disconnect	   = sched_interactive_disconnect,
	.name		   = "sched_interactive_h",
	.id_table	   = sched_interactive_ids,
};

static int sched_interactive_init(void) 
{
	int ret = 0;

	ret = input_register_handler(&sched_interactive_handler);
	if (ret)
		pr_err("Failed to register sched interactive handler\n");

	return ret;
}
postcore_initcall(sched_interactive_init);