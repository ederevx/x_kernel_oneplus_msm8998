// SPDX-License-Identifier: GPL-2.0
/*
 * Lightweight Timeout
 * Copyright (C) 2021 ederekun <sedrickvince@gmail.com>.
 */

#ifndef _LINUX_LWTIMEOUT_H
#define _LINUX_LWTIMEOUT_H

struct lwtimeout {
	atomic_t soft_lock;
	u64 ts;
	bool expired;
	const u64 duration;
};

/**
 * lwtimeout_update - disable timeout updates by acquiring a soft lock
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_lock(struct lwtimeout *lwt)
{
	atomic_inc(&lwt->soft_lock);
}

/**
 * lwtimeout_update - retrieve locks to re-enable timeout updates
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_unlock(struct lwtimeout *lwt)
{
	if (likely(atomic_read(&lwt->soft_lock) > 0))
		atomic_dec(&lwt->soft_lock);
}

/**
 * lwtimeout_check_timeout - check if the timeout had occured
 * @lwt: timeout to be evaluated
 */
static inline bool lwtimeout_check_timeout(struct lwtimeout *lwt) 
{
	bool expired = READ_ONCE(lwt->expired);

	if (!expired) {
		/* Enforce soft lock while also making the update single-threaded */
		if (atomic_inc_return(&lwt->soft_lock) == 1) {
			if (ktime_get_ns() > READ_ONCE(lwt->ts) + lwt->duration) {
				WRITE_ONCE(lwt->expired, true);
				expired = true;
			}
		}
		atomic_dec(&lwt->soft_lock);
	}

	return expired;
}

/**
 * lwtimeout_update_ts - update the timeout's timestamp and clear timeout
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_ts(struct lwtimeout *lwt)
{
	bool expired = READ_ONCE(lwt->expired);
	u64 ts_new = ktime_get_ns();

	atomic_inc(&lwt->soft_lock);
	if (expired || ts_new > READ_ONCE(lwt->ts))
		WRITE_ONCE(lwt->ts, ts_new);
	if (expired)
		WRITE_ONCE(lwt->expired, false);
	atomic_dec(&lwt->soft_lock);
}

/**
 * DEFINE_LW_TIMEOUT - defines a lightweight timeout structure
 * @_name: the name of the lwt structure to be made
 * @_expires: the structure's timeout in nanoseconds
 */
#define DEFINE_LW_TIMEOUT(_name, _expires)		\
	struct lwtimeout _name = {							\
		.soft_lock = ATOMIC_INIT(0),			\
		.ts = _expires,						\
		.expired = false,									\
		.duration = _expires							\
	}

#endif /* _LINUX_LWTIMEOUT_H */