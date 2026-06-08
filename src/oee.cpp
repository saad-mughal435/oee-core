#include "oee/oee.hpp"

#include <algorithm>
#include <map>

namespace oee {
namespace {

double clamp01(double v) {
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

}  // namespace

OeeResult compute(const ProductionInput& in) {
    OeeResult r;
    if (in.planned_time_min <= 0.0 || in.total_count <= 0 ||
        in.ideal_cycle_sec <= 0.0) {
        return r;  // valid stays false
    }

    const double downtime = (in.downtime_min > 0.0) ? in.downtime_min : 0.0;
    const double run_time_min = std::max(0.0, in.planned_time_min - downtime);
    r.run_time_min = run_time_min;

    const long rejects = (in.reject_count > 0) ? in.reject_count : 0;
    r.good_count = in.total_count - rejects;
    if (r.good_count < 0) r.good_count = 0;

    r.availability = clamp01(run_time_min / in.planned_time_min);

    if (run_time_min > 0.0) {
        const double ideal_min =
            (in.ideal_cycle_sec * static_cast<double>(in.total_count)) / 60.0;
        r.performance = clamp01(ideal_min / run_time_min);
    }

    r.quality = clamp01(static_cast<double>(r.good_count) /
                        static_cast<double>(in.total_count));

    r.oee   = r.availability * r.performance * r.quality;
    r.valid = true;
    return r;
}

std::string grade(double oee_value) {
    if (oee_value >= 0.85) return "World class";
    if (oee_value >= 0.60) return "Typical";
    if (oee_value >= 0.40) return "Low";
    return "Unacceptable";
}

DowntimeSummary analyze_downtime(const std::vector<DowntimeEvent>& events,
                                 double run_time_min) {
    DowntimeSummary s;
    std::map<std::string, double> totals;
    for (const auto& e : events) {
        if (e.minutes <= 0.0) continue;
        s.total_min += e.minutes;
        ++s.events;
        totals[e.reason] += e.minutes;
    }

    if (s.events > 0) {
        s.mttr_min = s.total_min / s.events;
        s.mtbf_min = (run_time_min > 0.0) ? run_time_min / s.events : 0.0;
    }

    s.by_reason.assign(totals.begin(), totals.end());
    std::sort(s.by_reason.begin(), s.by_reason.end(),
              [](const std::pair<std::string, double>& a,
                 const std::pair<std::string, double>& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });
    return s;
}

}  // namespace oee
