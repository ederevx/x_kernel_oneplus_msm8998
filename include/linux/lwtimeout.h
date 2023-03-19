// SPDX-License-Identifier: GPL-2.0
/*
 * Lightweight Timeout
 * Copyright (C) 2023 ederekun <sedrickvince@gmail.com>.
 */

#ifndef _LINUX_LWTIMEOUT_H
#define _LINUX_LWTIMEOUT_H

struct lwtimeout {
	atomic_t update_cnt;
	u64 ts;
	bool expired;
	const u64 duration;
};

/**
 * lwtimeout_check_timeout - check the expired boolean for timeout
 * @lwt: timeout to be evaluated
 */
static inline bool lwtimeout_check_timeout(struct lwtimeout *lwt) 
{
	return READ_ONCE(lwt->expired);
}

/**
 * lwtimeout_update_timeout - update expired boolean if timestamp had expired
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_timeout(struct lwtimeout *lwt)
{
	if (READ_ONCE(lwt->expired))
		return;

	/* Limit the timestamp access, avoid concurrency as much as possible */
	if (atomic_inc_return(&lwt->update_cnt) == 1) {
		if (ktime_get_ns() > READ_ONCE(lwt->ts) + lwt->duration)
			WRITE_ONCE(lwt->expired, true);
	}
	atomic_dec(&lwt->update_cnt);
}

/**
 * lwtimeout_update_ts - update the timeout's timestamp and clear timeout
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_ts(struct lwtimeout *lwt)
{
	bool expired = READ_ONCE(lwt->expired);
	u64 ts_new = ktime_get_ns();

	atomic_inc(&lwt->update_cnt);
	if (expired || ts_new > READ_ONCE(lwt->ts))
		WRITE_ONCE(lwt->ts, ts_new);
	if (expired)
		WRITE_ONCE(lwt->expired, false);
	atomic_dec(&lwt->update_cnt);
}

/**
 * DEFINE_LW_TIMEOUT - defines a lightweight timeout structure
 * @_name: the name of the lwt structure to be made
 * @_expires: the structure's timeout in nanoseconds
 */
#define DEFINE_LW_TIMEOUT(_name, _expires)		\
	struct lwtimeout _name = {							\
		.update_cnt = ATOMIC_INIT(0),			\
		.ts = _expires,						\
		.expired = false,									\
		.duration = _expires							\
	}

#endif /* _LINUX_LWTIMEOUT_H */