
#ifndef JOURNAL_HEAD_H_INCLUDED
#define JOURNAL_HEAD_H_INCLUDED

typedef unsigned int		tid_t;		
typedef struct transaction_s	transaction_t;	


struct buffer_head;

struct journal_head {
	struct buffer_head *b_bh;

	int b_jcount;

	unsigned b_jlist;

	unsigned b_modified;

	tid_t b_cow_tid;

	char *b_frozen_data;

	char *b_committed_data;

	transaction_t *b_transaction;

	transaction_t *b_next_transaction;

	struct journal_head *b_tnext, *b_tprev;

	transaction_t *b_cp_transaction;

	struct journal_head *b_cpnext, *b_cpprev;

	
	struct jbd2_buffer_trigger_type *b_triggers;

	
	struct jbd2_buffer_trigger_type *b_frozen_triggers;
};

#endif		
