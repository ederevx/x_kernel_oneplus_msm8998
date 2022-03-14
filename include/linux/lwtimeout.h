// SPDX-License-Identifier: GPL-2.0
/*
 * Lightweight Timeout
 * Copyright (C) 2021 ederekun <sedrickvince@gmail.com>.
 */

#ifndef _LINUX_LWTIMEOUT_H
#define _LINUX_LWTIMEOUT_H

struct lwtimeout {
	unsigned long state;
	atomic_t soft_lock;
	raw_spinlock_t ts_lock;
	u64 ts;
	u64 duration;
};

enum lwt_flags {
	TIMEOUT_EXPIRED = 0,
	TIMEOUT_LOCKED
};

/**
 * lwtimeout_update - disable timeout updates by acquiring a soft lock
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_lock(struct lwtimeout *lwt)
{
	if (atomic_inc_return(&lwt->soft_lock) == 1)
		set_bit(TIMEOUT_LOCKED, &lwt->state);
}

/**
 * lwtimeout_update - retrieve locks to re-enable timeout updates
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_unlock(struct lwtimeout *lwt)
{
	if (atomic_read(&lwt->soft_lock) > 0) {
		if (atomic_dec_return(&lwt->soft_lock) == 0)
			clear_bit(TIMEOUT_LOCKED, &lwt->state);
	}
}

/**
 * lwtimeout_check_timeout - check if the timeout had occured
 * @lwt: timeout to be evaluated
 */
static inline bool lwtimeout_check_timeout(struct lwtimeout *lwt) 
{
	if (!test_bit(TIMEOUT_EXPIRED, &lwt->state))
		return false;

	/* Do not timeout yet when timestamp is being accessed */
	if (raw_spin_is_locked(&lwt->ts_lock))
		return false;

	return true;
}

/**
 * lwtimeout_update - evaluate the timeout state value
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_timeout(struct lwtimeout *lwt)
{
	u64 curr;

	if (test_bit(TIMEOUT_EXPIRED, &lwt->state))
		return;

	if (test_bit(TIMEOUT_LOCKED, &lwt->state))
		return;

	curr = ktime_get_ns();

	if (raw_spin_trylock(&lwt->ts_lock)) {
		if ((curr - lwt->ts) > lwt->duration)
			set_bit(TIMEOUT_EXPIRED, &lwt->state);
		raw_spin_unlock(&lwt->ts_lock);
	}
}

/**
 * lwtimeout_update_ts - update the timeout's timestamp and clear timeout
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_ts(struct lwtimeout *lwt)
{
	u64 curr = ktime_get_ns();

	raw_spin_lock(&lwt->ts_lock);
	if (curr > lwt->ts)
		lwt->ts = curr;
	raw_spin_unlock(&lwt->ts_lock);

	clear_bit(TIMEOUT_EXPIRED, &lwt->state);
}

/**
 * DEFINE_LW_TIMEOUT - defines a lightweight timeout structure
 * @_name: the name of the lwt structure to be made
 * @_expires: the structure's timeout in nanoseconds
 */
#define DEFINE_LW_TIMEOUT(_name, _expires)		\
	struct lwtimeout _name = {							\
		.state = 0,									\
		.soft_lock = ATOMIC_INIT(0),			\
		.ts_lock = __RAW_SPIN_LOCK_UNLOCKED(_name.ts_lock),	\
		.ts = _expires,						\
		.duration = _expires							\
	}

#endif /* _LINUX_LWTIMEOUT_H */