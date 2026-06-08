#include "oee/oee.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void print_usage() {
    std::cout <<
        "oee - Overall Equipment Effectiveness calculator\n\n"
        "Usage:\n"
        "  oee --planned <min> --total <count> --cycle <sec>\n"
        "      [--downtime <min>] [--rejects <count>] [--downtime-csv <file>]\n\n"
        "Options:\n"
        "  --planned       Planned production time in minutes (required)\n"
        "  --total         Total units produced (required)\n"
        "  --cycle         Ideal cycle time in seconds per unit (required)\n"
        "  --downtime      Unplanned stop time in minutes (default 0)\n"
        "  --rejects       Rejected / scrap units (default 0)\n"
        "  --downtime-csv  CSV of 'reason,minutes' rows for a Pareto of stops\n"
        "  -h, --help      Show this help\n\n"
        "Example:\n"
        "  oee --planned 480 --downtime 47 --total 19271 --rejects 423 --cycle 1.337\n";
}

double pct(double v) { return v * 100.0; }

std::vector<oee::DowntimeEvent> read_downtime_csv(const std::string& path, bool& ok) {
    std::vector<oee::DowntimeEvent> out;
    std::ifstream f(path);
    ok = static_cast<bool>(f);
    if (!ok) return out;

    std::string line;
    while (std::getline(f, line)) {
        const std::string::size_type comma = line.rfind(',');
        if (comma == std::string::npos) continue;
        std::string reason = line.substr(0, comma);
        const std::string mins = line.substr(comma + 1);
        try {
            const double m = std::stod(mins);  // a header row throws here -> skipped
            const std::string::size_type a = reason.find_first_not_of(" \t\"");
            const std::string::size_type b = reason.find_last_not_of(" \t\"\r");
            if (a != std::string::npos) reason = reason.substr(a, b - a + 1);
            out.push_back({reason, m});
        } catch (...) {
            // ignore malformed / header rows
        }
    }
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    oee::ProductionInput in;
    std::string downtime_csv;
    bool have_planned = false, have_total = false, have_cycle = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-h" || a == "--help") {
            print_usage();
            return 0;
        } else if (a == "--planned" && i + 1 < argc) {
            in.planned_time_min = std::atof(argv[++i]);
            have_planned = true;
        } else if (a == "--downtime" && i + 1 < argc) {
            in.downtime_min = std::atof(argv[++i]);
        } else if (a == "--cycle" && i + 1 < argc) {
            in.ideal_cycle_sec = std::atof(argv[++i]);
            have_cycle = true;
        } else if (a == "--total" && i + 1 < argc) {
            in.total_count = std::atol(argv[++i]);
            have_total = true;
        } else if (a == "--rejects" && i + 1 < argc) {
            in.reject_count = std::atol(argv[++i]);
        } else if (a == "--downtime-csv" && i + 1 < argc) {
            downtime_csv = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete option: " << a << "\n\n";
            print_usage();
            return 2;
        }
    }

    if (!have_planned || !have_total || !have_cycle) {
        std::cerr << "Error: --planned, --total and --cycle are required.\n\n";
        print_usage();
        return 2;
    }

    const oee::OeeResult r = oee::compute(in);
    if (!r.valid) {
        std::cerr << "Error: inputs did not yield a valid OEE "
                     "(planned, total and cycle must be > 0).\n";
        return 1;
    }

    std::cout << std::fixed << std::setprecision(1)
              << "OEE report\n"
              << "----------\n"
              << "Availability : " << std::setw(6) << pct(r.availability) << " %\n"
              << "Performance  : " << std::setw(6) << pct(r.performance)  << " %\n"
              << "Quality      : " << std::setw(6) << pct(r.quality)      << " %\n"
              << "OEE          : " << std::setw(6) << pct(r.oee) << " %  ("
              << oee::grade(r.oee) << ")\n"
              << "Run time     : " << std::setw(6) << r.run_time_min << " min\n"
              << "Good units   : " << r.good_count << " of " << in.total_count << "\n";

    if (!downtime_csv.empty()) {
        bool ok = false;
        const std::vector<oee::DowntimeEvent> events = read_downtime_csv(downtime_csv, ok);
        if (!ok) {
            std::cerr << "Warning: could not open downtime CSV: " << downtime_csv << "\n";
        } else if (!events.empty()) {
            const oee::DowntimeSummary s = oee::analyze_downtime(events, r.run_time_min);
            std::cout << "\nDowntime (Pareto)\n"
                      << "-----------------\n"
                      << "Stops        : " << s.events << " events, " << s.total_min << " min\n"
                      << "MTTR         : " << std::setw(6) << s.mttr_min << " min\n"
                      << "MTBF         : " << std::setw(6) << s.mtbf_min << " min\n";
            for (const std::pair<std::string, double>& kv : s.by_reason) {
                const double share = s.total_min > 0.0 ? pct(kv.second / s.total_min) : 0.0;
                std::cout << "  " << std::left << std::setw(18) << kv.first << std::right
                          << std::setw(6) << kv.second << " min (" << share << " %)\n";
            }
        }
    }
    return 0;
}
