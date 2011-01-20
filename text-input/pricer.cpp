// 2011/01/18
// a pricer program 
// see a full problem description at http://www2.rgmadvisors.com/public/problems/orderbook


// author Denis Gorodetskiy noufos@gmail.com, 
// resume: http://bit.ly/gjXEjM, 
// linkedin profile: http://kr.linkedin.com/in/gorodetskiy

#include <assert.h>
#include <memory.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <iterator>
#include <utility>
#include <algorithm>
#include <map>

#ifdef  __GNUC__
#include <tr1/unordered_map>
#else
#include <unordered_map>
#endif

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "pricer_types.h"
#include "pricer_err.h"

// #define PROFILING

static void usage()
{
	std::cerr << "usage: pricer target-size\n";
	std::cerr << "or: pricer target-size max-messages input-filename\n";
	std::cerr << "\t if max-messages=-1 reads messages until eof\n";
	std::cerr << "\t if input-filename is omitted, reads messages from stdin\n";
}

enum
{
	BUY = 0, SELL = 1
};

typedef char side_t;

static
char char_from_side(side_t s)
{
	assert(BUY == s || SELL == s);
	return (BUY == s) ? 'B' : 'S';
}

static side_t side_from_char(char ch)
{
	assert('B' == ch || 'S' == ch);
	return ('B' == ch) ? BUY : SELL;
}

struct order
{
	order()
	{
	}
	order(timestamp_t ts, side_t side, price_t price, int size) :
		m_timestamp(ts), m_side(side), m_price(price), m_size(size)
	{
		assert(ts > 0);
		assert(size > 0);
		assert(BUY == side || SELL == side);
		assert(price > 0);
	}
	void reduce(int size)
	{
		assert(m_size > 0);
		assert(size > 0);
		if (size >= m_size)
		{
			m_size = 0;
		}
		else
		{
			m_size -= size;
		}
	}

	bool active() const
	{
		assert(m_size >= 0);
		return m_size > 0;
	}

	price_t price_of(int shares) const
	{
		assert(shares <= m_size && shares > 0);
		return m_price * shares;
	}

	int size() const
	{
		return m_size;
	}
	side_t side() const
	{
		return m_side;
	}
	price_t price() const
	{
		return m_price;
	}
	timestamp_t timestamp() const
	{
		return m_timestamp;
	}

protected:
	timestamp_t m_timestamp;
	side_t m_side; // BUY or SELL
	price_t m_price;
	int m_size; // number of shares
};

typedef boost::shared_ptr<order> order_ptr;
typedef std::list<order_ptr> sorted_orders_t;

// Seq = sorted sequence (linked list)
// complexity = O(N) if seq already sorted; N/2 on average
static
void insert_sorted(sorted_orders_t & seq, const order_ptr& val)
{
	int op = 0;

	typedef sorted_orders_t::iterator iterator;
	iterator it = seq.begin();
	for (; it != seq.end(); ++it, ++op)
	{
		if (!(*it)->active())
		{
			continue;
		}

		if (val->price() < (*it)->price())
		{
			seq.insert(it, val);
			return;
		}
	}

	seq.push_back(val);
}

static
bool if_deleted(const order_ptr & a)
{
	return !a->active();
}

static side_t opposite_side(side_t s)
{
	assert(BUY == s || SELL == s);
	return (BUY == s) ? SELL : BUY;
}

template<typename T>
T myabs(T v)
{
	return v > 0 ? v : -v;
}

struct market_base
{
	market_base(side_t side) :
		m_shares(0), m_last_income(), m_num_ghosts(0), m_side(side),
				m_last_timestamp(0), m_max_orders(0), m_max_sorted(0)
	{
	}
	virtual ~market_base()
	{
	}

	void add_order(timestamp_t timestamp, order_id_t id, side_t side,
			price_t price, int size)
	{
		assert(timestamp >= 0);
		assert(id >= 0);
		assert(price > (price_t) 0);
		assert(size >= 0);
		assert(side == m_side);

		// find() takes O(1) for hash tables
		order_table_iter_t pos = m_orders.find(id);

		if (pos != m_orders.end()) // order id already exists in the book
		{
			throw cant_add_order(timestamp, id,
					"order with the id already exists in the book");
		}
		order_ptr neword(new order(timestamp, side, price, size));

		m_orders[id] = neword; // operator [] takes O(1) amortized time for hash_table

		// O(N); N/2 on average
		insert_sorted(m_sorted_orders, neword);

		m_shares += size;
		m_last_timestamp = timestamp;

		if (m_orders.size() > m_max_orders)
			m_max_orders = m_orders.size();
		if (m_sorted_orders.size() > m_max_sorted)
			m_max_sorted = m_sorted_orders.size();
	}

	bool not_available(int target_size, std::ostream &os)
	{
		// if enough shares to (buy) sell
		if (m_shares < target_size)
		{
			if (m_last_income > 0)
			{
#ifndef PROFILING
				os << m_last_timestamp << ' ' << char_from_side(opposite_side(
						m_side)) << " NA\n";
#endif
				m_last_income = -1; // not available
			}
			return true;
		}
		return false;
	}

	// Amortized complexity O(1)
	void reduce_order(timestamp_t timestamp, order_id_t id, int size)
	{
		assert(id >= 0);
		assert(size >= 0);

		// hash_table takes O(1) to find
		order_table_iter_t pos = m_orders.find(id);

		if (pos == m_orders.end())
		{
			throw cant_reduce_order(timestamp, id, "no order id in the book");
		}

		order_ptr ord = (*pos).second;

		int to_reduce = std::min(size, ord->size());

		ord->reduce(to_reduce);

		if (!ord->active())
		{
			++m_num_ghosts;
			m_orders.erase(pos); // O(1) for hash_table
		}

		// amortized complexity of clearing dead orders is O(1)
		// because this code is called rarely (about 1 time per order),
		if (m_num_ghosts > (int) m_sorted_orders.size() * 0.75)
		{
			m_sorted_orders.remove_if(if_deleted);
		}

		m_shares -= to_reduce;
		m_last_timestamp = timestamp;
	}
	void report_income(price_t income, std::ostream & os)
	{
		assert(income > 0);
		if (myabs(income - m_last_income) < 0.005)
			return;

#ifndef PROFILING
		//printf("%u %c %.2f\n", m_last_timestamp, opposite_side(m_side), income);
		os << m_last_timestamp << ' ' << char_from_side(opposite_side(m_side))
				<< ' ' << income << '\n';
#endif
		m_last_income = income;
	}

	bool has_order(order_id_t id) const
	{
		return m_orders.find(id) != m_orders.end();
	}
	virtual void evaluate_income(int target_size, std::ostream &os) = 0;

protected:
	// hash table of all orders
	typedef std::tr1::unordered_map<order_id_t, order_ptr> order_table_t;

	typedef order_table_t::iterator order_table_iter_t;
	typedef std::list<order_ptr> sorted_orders_t;

	order_table_t m_orders;
	sorted_orders_t m_sorted_orders; // vector of pointers, sorted by price


	int m_shares; // total number of shares at the market at the amount
	price_t m_last_income;

	int m_num_ghosts; // number of ghost orders in m_sorted_orders
	side_t m_side; // BUY or SELL
	timestamp_t m_last_timestamp;

	size_t m_max_orders;
	size_t m_max_sorted;
	size_t m_num_reduces;
};

// the specialization of this market is that we have to traverse orders from 
// the beginning of the list in order to buy cheap
struct buy_market: public market_base
{
	buy_market() :
		market_base(BUY)
	{
	}

	virtual void evaluate_income(int target_size, std::ostream &os)
	{
		if (not_available(target_size, os))
		{
			return;
		}

		int remaining = target_size;
		price_t income(0);

		// start from the beginning of orders to maximize profit
		// iterate over sorted orders; O(N)
		sorted_orders_t::reverse_iterator it = m_sorted_orders.rbegin();
		for (; it != m_sorted_orders.rend() && remaining > 0; ++it)
		{
			const order_ptr & ord = (*it);

			if (!ord->active())
			{
				continue;
			}

			int shares = std::min(remaining, ord->size());

			income += ord->price_of(shares);

			remaining -= shares;

		}
		report_income(income, os);
	}
};

// the specialization of this market is that we have to traverse orders from 
// the end of the list in order to sell high
struct sell_market: public market_base
{
	sell_market() :
		market_base(SELL)
	{
	}

	virtual void evaluate_income(int target_size, std::ostream &os)
	{
		if (not_available(target_size, os))
		{
			return;
		}

		int remaining = target_size;
		price_t income(0);

		// start from the end of the order list to maximize profit
		// iterate over sorted orders; O(N), N/2 on average
		sorted_orders_t::iterator it = m_sorted_orders.begin();
		for (; it != m_sorted_orders.end() && remaining > 0; ++it)
		{
			order_ptr & ord = (*it);

			if (!ord->active())
			{
				continue;
			}

			int shares = std::min(remaining, ord->size());

			income += ord->price_of(shares);

			remaining -= shares;
		}
		report_income(income, os);
	}
};

struct book_t
{
	book_t() :
		m_last_side(0)
	{
		m_markets[BUY] = new buy_market();
		m_markets[SELL] = new sell_market();
	}
	~book_t()
	{
		delete m_markets[BUY];
		delete m_markets[SELL];
	}

	void add_order(timestamp_t timestamp, order_id_t id, side_t side,
			price_t price, int size)
	{
		assert(BUY == side || SELL == side);
		m_last_side = side;
		m_markets[(int) side]->add_order(timestamp, id, side, price, size);
	}

	void reduce_order(timestamp_t timestamp, order_id_t id, int size)
	{
		assert(id >= 0);
		assert(size >= 0);

		if (m_markets[BUY]->has_order(id))
		{
			m_markets[BUY]->reduce_order(timestamp, id, size);
			m_last_side = BUY;
		}
		else if (m_markets[SELL]->has_order(id))
		{
			m_markets[SELL]->reduce_order(timestamp, id, size);
			m_last_side = SELL;
		}
		else
		{
			throw cant_reduce_order(timestamp, id, "no order id in the book");
		}
	}

	void handle_message(const std::string & msg);

	void evaluate_income(int target_size, std::ostream & os)
	{
		m_markets[(int) m_last_side]->evaluate_income(target_size, os);
	}

protected:
	market_base* m_markets[2];
	side_t m_last_side;

};

static order_id_t order_id_from_str(const std::string & s)
{
	assert(s.size() <= sizeof(order_id_t));
	order_id_t id = 0;
	memcpy(&id, s.c_str(), s.size());
	return id;
}

void book_t::handle_message(const std::string & msg)
{
	int timestamp;
	char msg_type; // 'A' for add or 'R' for reduce
	std::string id_str;
	order_id_t id;
	int size; // number of shares

	std::stringstream is(msg); // input stream

	is >> timestamp >> msg_type >> id_str;

	if (timestamp < 0)
	{
		throw invalid_timestamp(timestamp);
	}

	id = order_id_from_str(id_str);
	if (id < 0)
	{
		throw invalid_order_id(timestamp, id);
	}

	if ('A' == msg_type)
	{
		char ch; // side character 'B' or 'S'
		price_t price;

		is >> ch >> price >> size;

		if ('B' != ch && 'S' != ch)
		{
			throw invalid_side(timestamp, ch);
		}
		if (price <= 0)
		{
			throw invalid_price(timestamp, price);
		}
		if (size <= 0)
		{
			throw invalid_size(timestamp, size);
		}

		side_t side = side_from_char(ch);
		add_order(timestamp, id, side, price, size);
	}
	else if ('R' == msg_type)
	{
		is >> size;
		if (size <= 0)
		{
			throw invalid_size(timestamp, size);
		}

		reduce_order(timestamp, id, size);
	}
	else
	{
		throw unsupported_msg_type(timestamp, msg_type);
	}

}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		usage();
		return -1;
	}

	int target_size;
	int max_messages = -1;

	char * input_filename = NULL;

	try
	{
		assert(argc > 1);
		target_size = boost::lexical_cast<int>(argv[1]);
		if (argc > 2)
		{
			max_messages = boost::lexical_cast<int>(argv[2]);
		}
		if (argc > 3)
		{
			input_filename = argv[3];
		}
	} catch (boost::bad_lexical_cast & /* e */)
	{
		usage();
		return -1;
	}

	std::string input_line;
	std::vector<std::string> tokens;

	int num_line = 0;

	book_t book;

	std::istream * is;
	std::ifstream f;

	if (NULL != input_filename)
	{
		f.open(input_filename);
		is = &f;
	}
	else
	{
		is = &std::cin;
	}

	std::ostream & os = std::cout;

	os.setf(std::ios::fixed, std::ios::floatfield);
	os.setf(std::ios::showpoint);
	os.precision(2);

	while (is->good())
	{
		++num_line;

		if (max_messages > 0 && num_line > max_messages)
		{
			break;
		}

		std::getline(*is, input_line);

		if (input_line.empty())
		{
			break;
		}

		try
		{
			book.handle_message(input_line);
			book.evaluate_income(target_size, std::cout);
		} catch (const exception_base & b)
		{
			std::cerr << "line: " << num_line << " [" << input_line << "]:";
			b.what(std::cerr);
		}
	}
	return 0;
}

