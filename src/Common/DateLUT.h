#pragma once

#include "DateLUTImpl.h"

#include <base/defines.h>

#include <boost/noncopyable.hpp>
#include "Common/CurrentThread.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>


/// This class provides lazy initialization and lookup of singleton DateLUTImpl objects for a given timezone.
class DateLUT : private boost::noncopyable
{
public:
    /// Return singleton DateLUTImpl instance for session timezone.
    /// The session timezone is configured by a session setting.
    /// If not set (empty string), it is the server timezone.
    static ALWAYS_INLINE const DateLUTImpl & instance()
    {
        const auto & date_lut = getInstance();

        if (DB::CurrentThread::isInitialized())
        {
            std::string context_timezone;
            const DB::ContextPtr query_context = DB::CurrentThread::get().getQueryContext();

            if (query_context)
            {
                context_timezone = extractTimezoneFromContext(query_context);

                if (!context_timezone.empty())
                    return date_lut.getImplementation(context_timezone);
            }

            /// Timezone is passed in query_context, but on CH-Client we have no query context,
            /// and each time we modify client's global context
            const auto global_context = DB::CurrentThread::get().getGlobalContext();
            if (global_context)
            {
                context_timezone = extractTimezoneFromContext(global_context);

                if (!context_timezone.empty())
                    return date_lut.getImplementation(context_timezone);
            }

        }
        return *date_lut.default_impl.load(std::memory_order_acquire);
    }

    static ALWAYS_INLINE const DateLUTImpl & instance(const std::string & time_zone)
    {
        if (time_zone.empty())
            return instance();

        const auto & date_lut = getInstance();
        return date_lut.getImplementation(time_zone);
    }

    // Return singleton DateLUTImpl for the server time zone.
    static ALWAYS_INLINE const DateLUTImpl & serverTimezoneInstance()
    {
        const auto & date_lut = getInstance();
        return *date_lut.default_impl.load(std::memory_order_acquire);
    }

    static void setDefaultTimezone(const std::string & time_zone)
    {
        auto & date_lut = getInstance();
        const auto & impl = date_lut.getImplementation(time_zone);
        date_lut.default_impl.store(&impl, std::memory_order_release);
    }

protected:
    DateLUT();

private:
    static DateLUT & getInstance();

    static std::string extractTimezoneFromContext(DB::ContextPtr query_context);

    const DateLUTImpl & getImplementation(const std::string & time_zone) const;

    using DateLUTImplPtr = std::unique_ptr<DateLUTImpl>;

    /// Time zone name -> implementation.
    mutable std::unordered_map<std::string, DateLUTImplPtr> impls;
    mutable std::mutex mutex;

    std::atomic<const DateLUTImpl *> default_impl;
};

inline UInt64 timeInMilliseconds(std::chrono::time_point<std::chrono::system_clock> timepoint)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(timepoint.time_since_epoch()).count();
}

inline UInt64 timeInMicroseconds(std::chrono::time_point<std::chrono::system_clock> timepoint)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(timepoint.time_since_epoch()).count();
}

inline UInt64 timeInSeconds(std::chrono::time_point<std::chrono::system_clock> timepoint)
{
    return std::chrono::duration_cast<std::chrono::seconds>(timepoint.time_since_epoch()).count();
}

inline UInt64 timeInNanoseconds(std::chrono::time_point<std::chrono::system_clock> timepoint)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(timepoint.time_since_epoch()).count();
}
