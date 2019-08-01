#include <stdlib.h>
#include <sysexits.h>
#include <climits>
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

#if defined(LOG) && !defined(XADD)
#error LOG only supported in XADD mode
#endif
#if defined(MAPPING2) && !defined(MAPPING)
#error MAPPING2 only supported in MAPPING mode
#endif
#if defined(XADD) && defined(MAPPING2)
#error MAPPING2 not supported in XADD mode
#endif

#define eprintf(args...) fprintf(stderr, args)

#ifdef DEBUG
#define debugf(args...) eprintf(args)
#else
#define debugf(args...) pass()
#endif

#define NUM_SHARED_DATA_SHIFT 18
#define NUM_SHARED_DATA (1lu << NUM_SHARED_DATA_SHIFT)
#define NUM_SHARED_DATA_MASK (NUM_SHARED_DATA - 1lu)

#define MAX_SHARED_DATA_CAP_SHIFT 8
#define MAX_SHARED_DATA_CAP (1lu << MAX_SHARED_DATA_CAP_SHIFT)

#define NUM_LOCAL_DATA_SHIFT 24
#define NUM_LOCAL_DATA (1lu << NUM_LOCAL_DATA_SHIFT)
#define NUM_LOCAL_DATA_MASK (NUM_LOCAL_DATA - 1lu)

#define NUM_TRACE_SHIFT (NUM_LOCAL_DATA_SHIFT + NUM_CPUS_SHIFT)
#define NUM_TRACE (1lu << NUM_TRACE_SHIFT)
#define NUM_TRACE_MASK (NUM_TRACE - 1lu)

// Machine-specific parameters:
// ROUND_ROBIN: Does the machine name its CPUs in round-robin fashion over the NUMA nodes?
// BLOCKS: Does the machine name its CPUs such that each node is a consecutive block?
// NUM_NODES: number of NUMA nodes
// NUM_NODES_SHIFT: ceil(log_2(NUM_NODES))
// NUM_CPUS: number of CPUs on this machine ( simultaneous multithreading)
// NUM_CPUS_SHIFT: ceil(log_2(NUM_CPUS))
// Below these parameters are specified for the 3 machines we tested on. To test on a new machine, define it and these parameter values here.

// AWARE: running on aware (Intel machine from paper)
// #define AWARE
// PBBS: running on PBBS (AMD machine from paper)
// #define PBBS
// ANDREW: running on unix.andrew machine (another Intel machine)
// #define ANDREW

#ifdef AWARE
#define ROUND_ROBIN true
#define BLOCKS false
#define NUM_NODES 4
#define NUM_NODES_SHIFT 2
#define NUM_CPUS 144
#define NUM_CPUS_SHIFT 8
#elif defined(PBBS)
#define ROUND_ROBIN false
#define BLOCKS true
#define NUM_NODES 8
#define NUM_NODES_SHIFT 3
#define NUM_CPUS 64
#define NUM_CPUS_SHIFT 6
#elif defined(ANDREW)
#define ROUND_ROBIN true
#define BLOCKS false
#define NUM_NODES 2
#define NUM_NODES_SHIFT 1
#define NUM_CPUS 40
#define NUM_CPUS_SHIFT 6
#else
#include "numa_configuration.hpp"
#endif

#define NUM_NODES_MASK ((1lu << NUM_NODES_SHIFT) - 1lu)

#ifdef MAPPING
#define NUM_MAPPINGS NUM_NODES
#else
#define NUM_MAPPINGS 1
#endif

#include "run.hpp"
#include "helper.hpp"
#include "init.hpp"
#include "sized_array.hpp"
#include "mathematica.hpp"

#ifdef LOG
#include "trace.hpp"
#ifndef XADD
#include "histo.hpp"
#endif
#endif

int usage_error(const char *msg) {
    eprintf("Error: %s\n", msg);
    return EX_USAGE;
}

uint64_t read_uint64(char* str) {
    return strtoul(str, NULL, 10);
}

uint64_t round_pow2(uint64_t n) {
    if (n == 0) {
        return 0;
    }
    // The left-hand expression is 63 for uint64_t,
    // but it adapts if we change to a different bit width.
    uint64_t log2_n = (sizeof n * CHAR_BIT - 1) - __builtin_clzl(n);
    return 1lu << log2_n;
}

int main(int argc, char** argv) {
    uint64_t len_atom = 0;
    uint64_t len_prep = 0;
    uint64_t len_wait = 0;
    uint64_t num_locs;
    double time;
    std::vector<uint64_t> nodes;
    std::vector<uint64_t> workers;
#ifdef MAPPING
    std::vector<uint64_t> mapping;
#ifdef MAPPING2
    std::vector<uint64_t> mapping2;
    double split;
#endif
#endif
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
#ifndef NODELAY
        ("atom", po::value<uint64_t>(&len_atom)->default_value(0), "length of atomic delay (between each read and following CAS attempt)")
        ("prep", po::value<uint64_t>(&len_prep)->default_value(0), "length of preparation delay (before each read)")
        ("wait", po::value<uint64_t>(&len_wait)->default_value(0), "length of waiting delay (between each successful CAS and following read)")
#endif
#ifdef MAPPING
        ("nlocs", po::value<uint64_t>(&num_locs)->default_value(1), "number of locations per node")
#else
        ("nlocs", po::value<uint64_t>(&num_locs)->default_value(1), "number of locations")
#endif
        ("time", po::value<double>(&time)->default_value(2), "time in seconds to run (rounded to nearest microsecond)")
        ("nodes", po::value< std::vector<uint64_t> >(&nodes)->multitoken()->default_value(std::vector<uint64_t>(), "all nodes"), "list of active nodes")
#if ROUND_ROBIN || BLOCKS
        ("no-smt", "do not use simultaneous multithreading")
        ("workers", po::value< std::vector<uint64_t> >(&workers)->multitoken()->default_value(std::vector<uint64_t>(), "all workers"), "list of workers (will be filtered by --nodes and --no-smt if applicable)")
#else
        ("workers", po::value< std::vector<uint64_t> >(&workers)->multitoken()->default_value(std::vector<uint64_t>(), "all workers"), "list of workers (will be filtered by --nodes if applicable)")
#endif
#ifdef MAPPING
#ifdef XADD
        ("mapping", po::value< std::vector<uint64_t> >(&mapping)->multitoken()->default_value(std::vector<uint64_t>(), "0"), "node accessed during benchmark")
#else
        ("mapping", po::value< std::vector<uint64_t> >(&mapping)->multitoken()->default_value(std::vector<uint64_t>(), "all 0s"), "mapping specifying which node accesses which node")
#ifdef MAPPING2
        ("mapping2", po::value< std::vector<uint64_t> >(&mapping2)->multitoken()->default_value(std::vector<uint64_t>(), "same as mapping"), "another mapping specifying which node accesses which node")
        ("split", po::value<double>(&split)->default_value(0.5), "fraction of workers using mapping2 instead of mapping")
#endif
#endif
#endif
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cerr << desc << "\n";
        return EX_USAGE;
    }

    bool use_smt = vm.count("no-smt") ? false : true;

#ifdef ONELOC
    if (num_locs != 1lu) {
        return usage_error("nlocs not supported when ONELOC is enabled");
    }
#else
    if (num_locs > MAX_SHARED_DATA_CAP) {
        return usage_error("nlocs must not exceed MAX_SHARED_DATA_CAP");
    }
#endif

    if (num_locs != round_pow2(num_locs)) {
        return usage_error("nlocs must be a power of 2");
    }

    // Should have mapping nonempty iff MAPPING is defined.
#ifdef MAPPING
#ifdef XADD
    if (mapping.size() == 0) {
        mapping.push_back(0);
    } else if (mapping.size() > 1) {
        return usage_error("mapping must be a single entry in XADD mode");
    }
    for (int i = 1; i < NUM_NODES; i++) {
        mapping.push_back(mapping[0]);
    }
#else
    if (mapping.size() != NUM_NODES) {
        if (mapping.size() == 0) {
            for (int i = 0; i < NUM_NODES; i++) {
                mapping.push_back(0);
            }
        } else if (mapping.size() == 1) {
            eprintf("mapping has single entry %lu, so assuming all entries should be %lu\n", mapping[0], mapping[0]);
            for (int i = 1; i < NUM_NODES; i++) {
                mapping.push_back(mapping[0]);
            }
        } else {
            return usage_error("mapping must have length equal to the number of nodes (or, as a shortcut, length 1)");
        }
    }
#ifdef MAPPING2
    if (mapping2.size() != NUM_NODES) {
        if (mapping2.size() == 0) {
            mapping2 = mapping;
        } else if (mapping2.size() == 1) {
            eprintf("mapping2 has single entry %lu, so assuming all entries should be %lu\n", mapping2[0], mapping2[0]);
            for (int i = 1; i < NUM_NODES; i++) {
                mapping2.push_back(mapping2[0]);
            }
        } else {
            return usage_error("mapping2 must have length equal to the number of nodes (or, as a shortcut, length 1)");
        }
    }
    if (split < 0 || split > 1) {
        return usage_error("split must be between 0 and 1");
    }
#endif // ifdef MAPPING2
#endif // ifdef XADD else
# endif // ifdef MAPPING

    test_params p = {};
    p.len_atom = len_atom;
    p.len_prep = len_prep;
    p.len_wait = len_wait;
    p.loc_mask = num_locs - 1;
#ifdef MAPPING
    p.mapping = mapping;
#ifdef MAPPING2
    p.mapping2 = mapping2;
    p.split = split;
#endif
#endif
    p.time = (uint32_t) (1000000 * time);
    init(p, nodes, use_smt, workers);

    run(p);

#if defined(LOG) && defined(XADD)
    const uint64_t trace_print_len = NUM_TRACE/2;
    sized_array<shared_item>* traces = new sized_array<shared_item>[NUM_MAPPINGS * num_locs];
    for (uint64_t loc_index = 0; loc_index < NUM_MAPPINGS * num_locs; loc_index++) {
        traces[loc_index].array = new shared_item[NUM_TRACE];
        traces[loc_index].len = 0;
    }
    xadd_trace(traces, p.workers, p.shared_data, p.local_data, num_locs);
#endif


    // Printing output.
    mtca::assoc_open();

    mtca::assoc_item("machine",
#ifdef AWARE
                     "Aware"
#elif defined(PBBS)
                     "PBBS"
#elif defined(ANDREW)
                     "Unix"
#else
                     "Other"
#endif
                     );
    mtca::assoc_item("CASProtocol",
#ifdef CCAS
                     "Read-CCAS"
#elif defined(XADD)
                     "XADD"
#else
                     "Read-CAS"
#endif
                     );
    mtca::assoc_item("ReadProtocol",
#ifdef NOREAD
                     "No-Read"
#elif defined(ONEREAD)
                     "One-Read"
#else
                     "All-Read"
#endif
                     );
    mtca::assoc_item("atom", p.len_atom);
    mtca::assoc_item("prep", p.len_prep);
    mtca::assoc_item("wait", p.len_wait);
    mtca::assoc_item("nlocs", num_locs);
    mtca::assoc_item("time", (uint64_t) p.time);

    mtca::assoc_item_open("optimizations");
    mtca::list_open();
#ifdef ONELOC
    mtca::list_item("ONELOC");
#endif
#ifdef NODELAY
    mtca::list_item("NODELAY");
#endif
    mtca::list_close();
    mtca::assoc_item_close();

    mtca::assoc_item_open("workers");
    mtca::list_open();
    for (uint64_t worker : p.workers) {
        mtca::list_item_open();
        mtca::assoc_open();
        mtca::assoc_item("worker", worker);
#ifdef MAPPING
        mtca::assoc_item("mapping",
#ifdef MAPPING2
                         use_mapping2(p.split, worker) ?
                         p.mapping2[cpu_node(worker)] :
#endif
                         p.mapping[cpu_node(worker)]);
#endif
        mtca::assoc_close();
        mtca::list_item_close();
    }
    mtca::list_close();
    mtca::assoc_item_close();

#ifdef LOG

#ifndef XADD
    // histogram histos[NUM_CPUS] = {0};
    // tally(histos, p.local_data);
    // mtca::assoc_item_open("histo");
    // print_histos(histos, p.workers);
    // mtca::assoc_item_close();

    histogram3d histos3d[NUM_NODES] = {0};
    tally3d(histos3d, p.local_data);

    mtca::assoc_item_open("histo3d");
    print_histos3d(histos3d);
    mtca::assoc_item_close();
#endif

    mtca::assoc_item_open("traces");
#ifdef XADD
    print_xadd_trace(traces, num_locs, trace_print_len);
#else
    // make_trace(traces, p.workers, infos, p.shared_data, p.local_data, num_locs, trace_print_len);
    // print_trace(traces, p.workers, infos, p.local_data, num_locs, trace_print_len);
#endif
    mtca::assoc_item_close();

#endif // #ifdef LOG

    mtca::assoc_item("time", p.time);

    uint64_t num_attempt_total = 0;
    uint64_t num_success_total = 0;
    // uint64_t num_streak_total = 0;
    mtca::assoc_item_open("summary");
    mtca::list_open();
    for (auto& worker : p.workers) {
        num_attempt_total += p.num_read[worker];
        num_success_total += p.num_success[worker];
        // num_streak_total += p.num_streak[worker];
        mtca::list_item_open();
        mtca::assoc_open();
        mtca::assoc_item("worker", worker);
        mtca::assoc_item("numAttempt", p.num_read[worker]);
        mtca::assoc_item("numSuccess", p.num_success[worker]);
        mtca::assoc_item("numFailure", p.num_read[worker] - p.num_success[worker]);
        // mtca::assoc_item("numStreak", p.num_streak[worker]);
        mtca::assoc_close();
        mtca::list_item_close();
    }
    mtca::list_close();
    mtca::assoc_item_close();
    eprintf("attempts %lu, successes %lu, ratio %f\n",
            num_attempt_total,
            num_success_total,
            ((double) num_success_total) / ((double) (num_attempt_total))
            );

    mtca::assoc_close();

    mtca::check_nesting();

    return EX_OK;
}
