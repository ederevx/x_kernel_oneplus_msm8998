// SPDX-License-Identifier: GPL-2.0
/*
 * Lightweight Timeout
 * Copyright (C) 2021 ederekun <sedrickvince@gmail.com>.
 */

#ifndef _LINUX_LWTIMEOUT_H
#define _LINUX_LWTIMEOUT_H

/**
 * lwtimeout_check - check if the timeout had occured
 *
 * Can be called by the user as _name##_check();
 * i.e. if 'cpufreq_timeout' is declared, then use cpufreq_timeout_check();
 */
#define lwtimeout_check(_name) READ_ONCE(_name##_state)

/**
 * lwtimeout_update - evaluate the timeout state value
 *
 * Can be called by the user as _name##_update();
 * i.e. if 'cpufreq_timeout' is declared, then use cpufreq_timeout_update();
 */
#define lwtimeout_update(_name, _expires)				\
	do {															\
		if (write_trylock(&_name##_lock)) {						\
			WRITE_ONCE(_name##_state, (jiffies - _name##_ts > _expires));	\
			write_unlock(&_name##_lock);							\
		}														\
	} while (0)

/**
 * lwtimeout_update_ts - update the timeout's timestamp and clear timeout
 *
 * Can be called by the user as _name##_update_ts();
 * i.e. if 'cpufreq_timeout' is declared, then use cpufreq_timeout_update_ts();
 */
#define lwtimeout_update_ts(_name)								\
	do {															\
		read_lock(&_name##_lock);							\
		if (raw_spin_trylock(&_name##_ts_lock)) {						\
			WRITE_ONCE(_name##_state, false);					\
			_name##_ts = jiffies;									\
			raw_spin_unlock(&_name##_ts_lock);					\
		}																\
		read_unlock(&_name##_lock);								\
	} while (0)

/**
 * DECLARE_LW_TIMEOUT - Defines and initializes all needed variables and 
 * functions static
 * @_name: the name of the lwt structure to be made
 * @_expires: the structure's timeout in jiffies
 * 
 * This initializes the following functions to be used based upon the defined 
 * lwt macros:
 * 		lwtimeout_check -> _name##_check();
 * 		lwtimeout_update -> _name##_update();
 * 		lwtimeout_update_ts -> _name##_update_ts();
 */
#define DECLARE_LW_TIMEOUT(_name, _expires);	  					\
	bool _name##_state = false;											\
	DEFINE_RWLOCK(_name##_lock);								\
	unsigned long _name##_ts = 0;											\
	DEFINE_RAW_SPINLOCK(_name##_ts_lock);								\
	static inline bool _name##_check(void) { return lwtimeout_check(_name); }		\
	static inline void _name##_update(void) { lwtimeout_update(_name, _expires); }		\
	static inline void _name##_update_ts(void) { lwtimeout_update_ts(_name); }

#endif /* _LINUX_LWTIMEOUT_H */