//
// Manta - Structural Variant and Indel Caller
// Copyright (c) 2013-2025 Illumina, Inc.
//
// This program is licensed under the terms of the Polyform strict license
//
// ***As far as the law allows, the software comes as is, without
// any warranty or condition, and the licensor will not be liable
// to you for any damages arising out of these terms or the use
// or nature of the software, under any kind of legal claim.***
//
// You should have received a copy of the PolyForm Strict License 1.0.0
// along with this program.  If not, see <https://polyformproject.org/licenses/strict/1.0.0>.
//

//

/// \file
/// \author Chris Saunders
///

#pragma once

#include "boost/serialization/nvp.hpp"
#include "boost/utility.hpp"

#include <chrono>
#include <iosfwd>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

// ponytail: wall/user/system time via std::chrono + getrusage, replacing boost::timer
// (boost::timer was removed from boost 1.75+). User/system time is the process-wide
// rusage delta since process start, same semantics as boost::timer::cpu_timer.

namespace BOOST_TIMER_HELPER {
inline double getTimerSeconds(const std::chrono::nanoseconds& ns)
{
  return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(ns).count()) / 1000000.;
}

inline struct rusage currentRusage()
{
  struct rusage r;
#if defined(__unix__) || defined(__APPLE__)
  getrusage(RUSAGE_SELF, &r);
#else
  (void)r;
#endif
  return r;
}

inline double rusageSeconds(const struct rusage& r)
{
  return static_cast<double>(r.ru_utime.tv_sec) + static_cast<double>(r.ru_utime.tv_usec) / 1000000.
       + static_cast<double>(r.ru_stime.tv_sec) + static_cast<double>(r.ru_stime.tv_usec) / 1000000.;
}

inline double rusageUserSeconds(const struct rusage& r)
{
  return static_cast<double>(r.ru_utime.tv_sec) + static_cast<double>(r.ru_utime.tv_usec) / 1000000.;
}

inline double rusageSystemSeconds(const struct rusage& r)
{
  return static_cast<double>(r.ru_stime.tv_sec) + static_cast<double>(r.ru_stime.tv_usec) / 1000000.;
}
}  // namespace BOOST_TIMER_HELPER

/// this is a replacement for boost::timer cpu_times
/// with serialization/merge, etc...
struct CpuTimes {
  CpuTimes() {}

  CpuTimes(const CpuTimes& t)
    : wall(t.wall), user(t.user), system(t.system)
  {
  }

  void merge(const CpuTimes& rhs)
  {
    wall += rhs.wall;
    user += rhs.user;
    system += rhs.system;
  }

  void clear()
  {
    wall   = 0;
    user   = 0;
    system = 0;
  }

  void difference(const CpuTimes& rhs)
  {
    wall -= rhs.wall;
    user -= rhs.user;
    system -= rhs.system;
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned /*version*/)
  {
    ar& BOOST_SERIALIZATION_NVP(wall) & BOOST_SERIALIZATION_NVP(user) & BOOST_SERIALIZATION_NVP(system);
  }

  void reportSec(std::ostream& os) const
  {
    static const char   tlabel('s');
    static const double factor(1.);
    report(factor, &tlabel, os);
  }

  void reportHr(std::ostream& os) const
  {
    static const char   tlabel('h');
    static const double factor(1. / 3600.);
    report(factor, &tlabel, os);
  }

  void report(const double factor, const char* tlabel, std::ostream& os) const;

  double wall   = 0.;
  double user   = 0.;
  double system = 0.;
};

BOOST_CLASS_IMPLEMENTATION(CpuTimes, boost::serialization::object_serializable)

/// simple time track utility
struct TimeTracker {
  TimeTracker()
  {
    _baseRusage = BOOST_TIMER_HELPER::currentRusage();
    stop();
  }

  void clear() { _isReset = true; }

  /// starts clock without reset to accumulate total time
  void resume()
  {
    //assert((! _isStart) && "clock is running");
    if (_isReset) {
      _accumulated = std::chrono::nanoseconds(0);
      _isReset = false;
    }
    _start = std::chrono::steady_clock::now();
    _isStart = true;
  }

  /// stop clock
  void stop()
  {
    if (_isStart) {
      _accumulated += std::chrono::steady_clock::now() - _start;
      _isStart = false;
    }
  }

  CpuTimes getTimes() const
  {
    static const CpuTimes zero;
    if (_isReset) return zero;
    CpuTimes t;
    std::chrono::nanoseconds elapsed = _accumulated;
    if (_isStart) {
      elapsed += std::chrono::steady_clock::now() - _start;
    }
    t.wall = BOOST_TIMER_HELPER::getTimerSeconds(elapsed);
    const struct rusage now = BOOST_TIMER_HELPER::currentRusage();
    t.user   = BOOST_TIMER_HELPER::rusageUserSeconds(now) - BOOST_TIMER_HELPER::rusageUserSeconds(_baseRusage);
    t.system = BOOST_TIMER_HELPER::rusageSystemSeconds(now) - BOOST_TIMER_HELPER::rusageSystemSeconds(_baseRusage);
    return t;
  }

  /// DEPRECATED get user cpu time in seconds
  ///
  /// timer must be stopped
  double getUserSeconds() const { return getTimes().user; }

  /// DEPRECATED get wall time in seconds
  ///
  /// timer must be stopped
  double getWallSeconds() const { return getTimes().wall; }

private:
  bool _isReset = true;
  bool _isStart = false;
  std::chrono::steady_clock::time_point _start;
  std::chrono::nanoseconds _accumulated = std::chrono::nanoseconds(0);
  struct rusage _baseRusage;
};

/// utility for timetracker for scope based start-stop scenarios:
struct TimeScoper : private boost::noncopyable {
  explicit TimeScoper(TimeTracker& t) : _t(t) { _t.resume(); }

  ~TimeScoper() { _t.stop(); }

private:
  TimeTracker& _t;
};
