#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include "oee/oee.hpp"

using Catch::Matchers::WithinAbs;

TEST_CASE("OEE matches the classic worked example", "[oee]") {
    oee::ProductionInput in;
    in.planned_time_min = 480.0;  // 8-hour shift
    in.downtime_min     = 47.0;
    in.total_count      = 19271;
    in.reject_count     = 423;
    in.ideal_cycle_sec  = 1.337;

    const oee::OeeResult r = oee::compute(in);
    REQUIRE(r.valid);
    CHECK_THAT(r.availability, WithinAbs(0.9021, 0.001));  // 433 / 480
    CHECK_THAT(r.performance, WithinAbs(0.9917, 0.002));
    CHECK_THAT(r.quality, WithinAbs(0.9780, 0.001));
    CHECK_THAT(r.oee, WithinAbs(0.8750, 0.003));
    CHECK(r.good_count == 18848);
    CHECK_THAT(r.run_time_min, WithinAbs(433.0, 1e-9));
}

TEST_CASE("performance is capped at 100% for over-speed data", "[oee]") {
    oee::ProductionInput in;
    in.planned_time_min = 100.0;
    in.total_count      = 100000;  // implausibly high -> would exceed 100%
    in.ideal_cycle_sec  = 1.0;

    const oee::OeeResult r = oee::compute(in);
    REQUIRE(r.valid);
    CHECK(r.performance == 1.0);
}

TEST_CASE("invalid inputs are rejected", "[oee]") {
    CHECK_FALSE(oee::compute({0.0, 0.0, 100, 0, 2.0}).valid);    // no planned time
    CHECK_FALSE(oee::compute({480.0, 0.0, 0, 0, 2.0}).valid);    // no production
    CHECK_FALSE(oee::compute({480.0, 0.0, 100, 0, 0.0}).valid);  // no cycle time
}

TEST_CASE("OEE grade bands", "[oee]") {
    CHECK(oee::grade(0.90) == "World class");
    CHECK(oee::grade(0.70) == "Typical");
    CHECK(oee::grade(0.50) == "Low");
    CHECK(oee::grade(0.10) == "Unacceptable");
}

TEST_CASE("downtime analytics build a sorted Pareto with MTTR/MTBF", "[downtime]") {
    const std::vector<oee::DowntimeEvent> ev = {
        {"Changeover", 20.0}, {"Jam", 8.0}, {"Changeover", 10.0},
        {"Material", 5.0},    {"Jam", 2.0},
    };
    const oee::DowntimeSummary s = oee::analyze_downtime(ev, 433.0);

    CHECK(s.events == 5);
    CHECK_THAT(s.total_min, WithinAbs(45.0, 1e-9));
    CHECK_THAT(s.mttr_min, WithinAbs(9.0, 1e-9));   // 45 / 5
    CHECK_THAT(s.mtbf_min, WithinAbs(86.6, 0.05));  // 433 / 5
    REQUIRE(s.by_reason.size() == 3);
    CHECK(s.by_reason[0].first == "Changeover");
    CHECK_THAT(s.by_reason[0].second, WithinAbs(30.0, 1e-9));
    CHECK(s.by_reason[1].first == "Jam");
    CHECK_THAT(s.by_reason[1].second, WithinAbs(10.0, 1e-9));
}

TEST_CASE("zero-minute downtime events are ignored", "[downtime]") {
    const std::vector<oee::DowntimeEvent> ev = {{"Idle", 0.0}, {"Jam", 5.0}};
    const oee::DowntimeSummary s = oee::analyze_downtime(ev, 100.0);
    CHECK(s.events == 1);
    REQUIRE(s.by_reason.size() == 1);
    CHECK(s.by_reason[0].first == "Jam");
}
