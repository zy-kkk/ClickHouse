#include <IO/ReadBufferFromPocoSocketChunked.h>
#include <Common/logger_useful.h>
#include <IO/NetUtils.h>


namespace DB::ErrorCodes
{
    extern const int LOGICAL_ERROR;
}

namespace DB
{

ReadBufferFromPocoSocketChunked::ReadBufferFromPocoSocketChunked(Poco::Net::Socket & socket_, size_t buf_size)
    : ReadBufferFromPocoSocketChunked(socket_, ProfileEvents::end(), buf_size)
{}

ReadBufferFromPocoSocketChunked::ReadBufferFromPocoSocketChunked(Poco::Net::Socket & socket_, const ProfileEvents::Event & read_event_, size_t buf_size)
    : ReadBufferFromPocoSocketBase(socket_, read_event_, buf_size), our_address(socket_.address()), log(getLogger("Protocol"))

{
    chassert(buf_size <= std::numeric_limits<decltype(chunk_left)>::max());
}

void ReadBufferFromPocoSocketChunked::enableChunked()
{
    if (chunked)
        return;
    chunked = 1;
    data_end = buffer().end();
    working_buffer.resize(offset());
    chunk_left = 0;
    next_chunk = 0;
}

bool ReadBufferFromPocoSocketChunked::hasPendingData() const
{
    if (chunked)
        return available() || static_cast<size_t>(data_end - working_buffer.end()) > sizeof(next_chunk);

    return ReadBufferFromPocoSocketBase::hasPendingData();
}

bool ReadBufferFromPocoSocketChunked::poll(size_t timeout_microseconds) const
{
    if (chunked)
        if (available() || static_cast<size_t>(data_end - working_buffer.end()) > sizeof(next_chunk))
            return true;

    return ReadBufferFromPocoSocketBase::poll(timeout_microseconds);
}


bool ReadBufferFromPocoSocketChunked::load_next_chunk(Position c_pos, bool cont)
{
    auto buffered = std::min(static_cast<size_t>(data_end - c_pos), sizeof(next_chunk));

    if (buffered)
        std::memcpy(&next_chunk, c_pos, buffered);
    if (buffered < sizeof(next_chunk))
        if (socketReceiveBytesImpl(reinterpret_cast<char *>(&next_chunk) + buffered, sizeof(next_chunk) - buffered) < static_cast<ssize_t>(sizeof(next_chunk) - buffered))
            return false;
    next_chunk = fromLittleEndian(next_chunk);

    if (next_chunk)
    {
        if (cont)
            LOG_TEST(log, "Packet receive continued. Size {}", next_chunk);
    }
    else
        LOG_TEST(log, "Packet receive ended.");

    return true;
}

bool ReadBufferFromPocoSocketChunked::process_chunk_left(Position c_pos)
{
    if (data_end - c_pos < chunk_left)
    {
        working_buffer.resize(data_end - buffer().begin());
        nextimpl_working_buffer_offset = c_pos - buffer().begin();
        chunk_left -= (data_end - c_pos);
        return true;
    }

    nextimpl_working_buffer_offset = c_pos - buffer().begin();
    working_buffer.resize(nextimpl_working_buffer_offset + chunk_left);

    c_pos += chunk_left;

    if (!load_next_chunk(c_pos, true))
        return false;

    chunk_left = 0;
    return true;
}


bool ReadBufferFromPocoSocketChunked::nextImpl()
{
    if (!chunked)
        return ReadBufferFromPocoSocketBase::nextImpl();

    auto c_pos = pos;

    if (chunk_left == 0)
    {
        if (next_chunk == 0)
        {
            if (chunked == 1)
                chunked = 2; // first chunked block - no end marker
            else
                c_pos = pos + sizeof(next_chunk); // bypass chunk end marker

            if (c_pos > data_end)
                c_pos = data_end;

            if (!load_next_chunk(c_pos))
                return false;

            chunk_left = next_chunk;
            next_chunk = 0;

            if (chunk_left == 0)
                throw Exception(ErrorCodes::LOGICAL_ERROR, "Native protocol: empty chunk received");

            c_pos += sizeof(next_chunk);

            if (c_pos >= data_end)
            {
                if (!ReadBufferFromPocoSocketBase::nextImpl())
                    return false;
                data_end = buffer().end();
                c_pos = buffer().begin();
            }

            LOG_TEST(log, "Packet receive started. Message {}, size {}", static_cast<unsigned int>(*c_pos), chunk_left);
        }
        else
        {
            c_pos += sizeof(next_chunk);
            if (c_pos >= data_end)
            {
                if (!ReadBufferFromPocoSocketBase::nextImpl())
                    return false;
                data_end = buffer().end();
                c_pos = buffer().begin();
            }

            chunk_left = next_chunk;
            next_chunk = 0;
        }
    }
    else
    {
        chassert(c_pos == data_end);

        if (!ReadBufferFromPocoSocketBase::nextImpl())
            return false;
        data_end = buffer().end();
        c_pos = buffer().begin();
    }

    return process_chunk_left(c_pos);
}

}
