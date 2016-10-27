#ifndef __CHAN_SELECT_H__
#define __CHAN_SELECT_H__

/**
 * See chan_read in chan.h
 */
extern int chan_select_read(struct chan*, struct chan_msg*, int);

/**
 * See chan_select in chan.h
 */
extern int chan_select_select(struct chan**, unsigned int, struct chan_msg*, int);

#endif /* __CHAN_SELECT_H__ */
