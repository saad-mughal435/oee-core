#pragma once

#include <string>
#include <utility>
#include <vector>

namespace oee {

/// Inputs describing one production run (a shift, a job order, etc.).
struct ProductionInput {
    double planned_time_min = 0.0;  ///< Planned production time, minutes.
    double downtime_min     = 0.0;  ///< Unplanned stop time, minutes.
    long   total_count      = 0;    ///< Total units produced (good + bad).
    long   reject_count     = 0;    ///< Rejected / scrap units.
    double ideal_cycle_sec  = 0.0;  ///< Ideal cycle time, seconds per unit.
};

/// The three OEE factors and their product, each in the range [0, 1].
struct OeeResult {
    double availability = 0.0;
    double performance  = 0.0;
    double quality      = 0.0;
    double oee          = 0.0;
    double run_time_min = 0.0;
    long   good_count   = 0;
    bool   valid        = false;  ///< True when inputs yielded a meaningful OEE.
};

/// Compute OEE = Availability x Performance x Quality.
///
///   Availability = run time / planned time   (run time = planned - downtime)
///   Performance  = (ideal cycle x total) / run time
///   Quality      = good / total
///
/// Guards against divide-by-zero and clamps each factor to [0, 1]; performance
/// is capped at 1.0 to absorb minor over-speed in logged data.
OeeResult compute(const ProductionInput& in);

/// SEMI-style world-class banding for an OEE value in [0, 1].
std::string grade(double oee_value);

/// A single downtime occurrence with its reason code.
struct DowntimeEvent {
    std::string reason;
    double      minutes = 0.0;
};

/// Aggregated downtime analytics over a set of events.
struct DowntimeSummary {
    double total_min = 0.0;
    int    events    = 0;
    double mtbf_min  = 0.0;  ///< Mean operating time between failures.
    double mttr_min  = 0.0;  ///< Mean time to repair (downtime / events).
    /// Minutes per reason, sorted descending — a Pareto of stops.
    std::vector<std::pair<std::string, double>> by_reason;
};

/// Summarise downtime events; run_time_min is the operating time used for MTBF.
DowntimeSummary analyze_downtime(const std::vector<DowntimeEvent>& events,
                                 double run_time_min);

}  // namespace oee
