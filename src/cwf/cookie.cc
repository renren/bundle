#include <sstream>
// #include <boost/date_time/gregorian/gregorian.hpp>
// #include <boost/date_time/posix_time/posix_time.hpp>
// #include <boost/date_time/local_time/local_time_io.hpp>
#include "cwf/cookie.h"

namespace cwf {

std::string Cookie::DateFormat(int hour/*=0*/, int day/*=0*/, int month/*=0*/) {
  // using namespace boost::gregorian;
  // using namespace boost::posix_time;
  
  // Tue, 03-Sep-2019 11:25:23 GMT
  // date today = day_clock::local_day();

#if 0
  ptime today = second_clock::local_time();
  ptime ret;
  if (hour)
   ret = today + hours(hour);

  if (day)
    ret = today + days(day);

  local_time_facet* output_facet = new local_time_facet();
  output_facet->format("%a %b %d, %H:%M %z");

  ss.imbue(locale(locale::classic(), output_facet));
  std::ostringstream ss;
  ss << ret;
  return ss.str();
#endif
  return "Tue, 03-Sep-2019 11:25:23 GMT";
}

}

