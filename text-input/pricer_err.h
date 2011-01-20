#ifndef __PRICER_ERR_H_123129301923802395__
#define __PRICER_ERR_H_123129301923802395__

#include <ostream>
#include "pricer_types.h"

struct exception_base
{
    virtual void what(std::ostream & os) const = 0;
};

struct invalid_side : public exception_base
{
    timestamp_t m_timestamp;
    char        m_side;

    invalid_side(timestamp_t ts, char s) 
        : m_timestamp(ts)
        , m_side(s) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid side '"<<m_side<<"'\n";
    }
};
struct invalid_order_id : public exception_base
{
    timestamp_t m_timestamp;
    order_id_t  m_id;

    invalid_order_id(timestamp_t ts, order_id_t id) 
        : m_timestamp(ts)
        , m_id(id) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid order id '"<<m_id<<"'\n";
    }
};

struct invalid_size : public exception_base
{
    timestamp_t m_timestamp;
    int         m_size;

    invalid_size(timestamp_t ts, int s) 
        : m_timestamp(ts)
        , m_size(s) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid size "<<m_size<<"\n";
    }
};

struct invalid_price : public exception_base
{
    timestamp_t m_timestamp;
    price_t     m_price;

    invalid_price(timestamp_t ts, price_t price) 
        : m_timestamp(ts)
        , m_price(price) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid price "<<m_price<<"\n";
    }
};

struct cant_add_order: public exception_base
{
    timestamp_t m_timestamp;
    order_id_t m_id;
    const char* m_msg;

    cant_add_order(timestamp_t ts, order_id_t id, const char *msg) 
        : m_timestamp(ts)
        , m_id(id)
        , m_msg(msg) 
    {
        assert(NULL != m_msg);
    }
    virtual void what(std::ostream & os)  const
    {
        os << "err: at "<<m_timestamp << " ms can't add order'"
            <<m_id<<"' ("<<m_msg<<")\n";
    }
};

struct cant_reduce_order: public exception_base
{
    timestamp_t m_timestamp;
    order_id_t m_id;
    const char* m_msg;

    cant_reduce_order(timestamp_t ts, order_id_t id, const char* msg) 
        : m_timestamp(ts)
        , m_id(id)
        , m_msg(msg) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms can't reduce order'"
            <<m_id<<"' ("<<m_msg<<")\n";
    }
};

struct unsupported_msg_type: public exception_base
{
    timestamp_t m_timestamp;
    char        m_type;

    unsupported_msg_type(timestamp_t ts, char t) 
        : m_timestamp(ts)
        , m_type(t) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms unsupported message type '"
            <<m_type<<"'\n";
    }
};

struct invalid_timestamp: public exception_base
{
    timestamp_t m_timestamp;

    invalid_timestamp(timestamp_t ts) 
        : m_timestamp(ts) 
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid timestamp\n";
    }
};

struct invalid_num_shares: public exception_base
{
    timestamp_t m_timestamp;
    int         m_num_shares;

    invalid_num_shares(timestamp_t ts, int num_shares)
        : m_timestamp(ts) 
        , m_num_shares(num_shares)
    {}
    virtual void what(std::ostream & os) const
    {
        os << "err: at "<<m_timestamp << " ms invalid number of shares "
            << m_num_shares << '\n';
    }
};
#endif