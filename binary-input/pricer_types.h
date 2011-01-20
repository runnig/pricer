#ifndef __PRICER_TYPES_H_1203912039843993059646023480__
#define __PRICER_TYPES_H_1203912039843993059646023480__

#include <stdint.h>

typedef uint64_t order_id_t;
typedef uint32_t timestamp_t;
typedef double price_t;

struct message_t
{
	int timestamp;
	char msg_type; // 'A' for add or 'R' for reduce
	uint64_t id; // order id
	int size; // number of shares
	char side; // 'B' or 'S'
	double price;
};

#endif
