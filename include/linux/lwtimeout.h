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
	rwlock_t lock;
	raw_spinlock_t expires_lock;
	u64 expires;
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
 * lwtimeout_check - check if the timeout had occured
 * @lwt: timeout to be evaluated
 */
static inline bool lwtimeout_check(struct lwtimeout *lwt) 
{
	return test_bit(TIMEOUT_EXPIRED, &lwt->state);
}

/**
 * lwtimeout_update - evaluate the timeout state value
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update(struct lwtimeout *lwt)
{
	if (lwtimeout_check(lwt))
		return;

	if (test_bit(TIMEOUT_LOCKED, &lwt->state))
		return;

	if (write_trylock(&lwt->lock)) {
		if (ktime_get_ns() > lwt->expires)
			set_bit(TIMEOUT_EXPIRED, &lwt->state);
		write_unlock(&lwt->lock);
	}
}

/**
 * lwtimeout_update_expires - update the timeout's expires and clear timeout
 * @lwt: timeout to be modified
 */
static inline void lwtimeout_update_expires(struct lwtimeout *lwt)
{
	read_lock(&lwt->lock);

	if (lwtimeout_check(lwt))
		clear_bit(TIMEOUT_EXPIRED, &lwt->state);

	if (raw_spin_trylock(&lwt->expires_lock)) {
		lwt->expires = ktime_get_ns() + lwt->duration;
		raw_spin_unlock(&lwt->expires_lock);
	}

	read_unlock(&lwt->lock);
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
		.lock = __RW_LOCK_UNLOCKED(_name.lock),			\
		.expires_lock = __RAW_SPIN_LOCK_UNLOCKED(_name.expires_lock),	\
		.expires = _expires,						\
		.duration = _expires							\
	}

#endif /* _LINUX_LWTIMEOUT_H */