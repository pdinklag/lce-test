/*******************************************************************************
 * benchmark/bench_time.cpp
 *
 * Copyright (C) 2019 Alexander Herlez <alexander.herlez@tu-dortmund.de>
 * Copyright (C) 2019 Florian Kurpicz <florian.kurpicz@tu-dortmund.de>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <malloc_count.h>

#include <fstream>
#include <sys/time.h>
#include <vector>
#include <iomanip>

#include <filesystem>

#include <memory>

#include <tlx/cmdline_parser.hpp>
#include <tlx/math/aggregate.hpp>

#include "io.hpp"
#include "timer.hpp"
#include "build_lce_ranges.hpp"
#include "lce_naive.hpp"
#include "lce_naive_xor.hpp"
#include "lce_naive_ultra.hpp"
#include "lce_prezza.hpp"
#include "lce_prezza_mersenne.hpp"
#include "lce_semi_synchronizing_sets.hpp"
#include "lce_semi_synchronizing_sets_par.hpp"

#include "lce_sdsl_cst.hpp"

namespace fs = std::filesystem;

class lce_benchmark {

public:
  void run() {
 
    fs::path input_path(file_path);
    std::string const filename = input_path.filename();
    
    string lce_path = output_path + filename;

    if (prefix_length > 0) {
      lce_path += "_" + std::to_string(prefix_length);
    }

    const array<string, 21> lce_set{lce_path + "/lce_0", lce_path + "/lce_1",
                                    lce_path + "/lce_2", lce_path + "/lce_3",
                                    lce_path + "/lce_4", lce_path + "/lce_5",
                                    lce_path + "/lce_6", lce_path + "/lce_7",
                                    lce_path + "/lce_8", lce_path + "/lce_9",
                                    lce_path + "/lce_10", lce_path + "/lce_11",
                                    lce_path + "/lce_12", lce_path + "/lce_13",
                                    lce_path + "/lce_14", lce_path + "/lce_15",
                                    lce_path + "/lce_16",lce_path + "/lce_17",
                                    lce_path + "/lce_18", lce_path + "/lce_19",
                                    lce_path + "/lce_X"};
    
    build_lce_range(file_path, output_path + filename, prefix_length);
    
    
    /************************************
     ****PREPARE LCE DATA STRUCTURES*****
     ************************************/

    std::unique_ptr<LceDataStructure> lce_structure;
    std::vector<uint8_t> text;

    timer t;
    tlx::Aggregate<size_t> construction_times;
    tlx::Aggregate<size_t> construction_mem_peak;
    tlx::Aggregate<size_t> lce_mem;

    std::cout << "RESULT "
              << "algo=" << print_algo_name() << " "
              << "runs=" << runs << " ";

    for (size_t i = 0; i < runs; ++i) {
      text = load_text(file_path, prefix_length);

      auto* old_structure = lce_structure.release();
      if (old_structure != nullptr) {
        delete old_structure;
      }
      if (algorithm == "u") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure = std::make_unique<LceUltraNaive>(text);
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "n") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure = std::make_unique<LceNaive>(text);
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "nx") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure = std::make_unique<LceNaiveXor>(text);
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "m") {
        t.reset();
        lce_structure = std::make_unique<rklce::LcePrezzaMersenne>(text);
        construction_times.add(t.get_and_reset());
      } else if (algorithm == "p") {
        // Make sure the text can be divided into 64 bit blocks
        text.resize(text.size() + (8 - (text.size() % 8)));
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure =
          std::make_unique<LcePrezza<128>>(reinterpret_cast<uint64_t*>(text.data()),
                                      text.size());
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s2048") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<LceSemiSyncSets<2048, true>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<LceSemiSyncSets<2048, false>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s1024") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<LceSemiSyncSets<1024, true>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<LceSemiSyncSets<1024, false>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s512") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<LceSemiSyncSets<512, true>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<LceSemiSyncSets<512, false>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s256") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<LceSemiSyncSets<256, true>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<LceSemiSyncSets<256, false>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s2048_par") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<2048>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<2048>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s1024_par") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<1024>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<1024>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s512_par") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<512>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<512>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "s256_par") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        if (prefer_long_queries) {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<256>>(text, i == 0);
        } else {
          lce_structure = std::make_unique<lce_test::par::LceSemiSyncSetsPar<256>>(text, i == 0);
        }
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else if (algorithm == "sada") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure = std::make_unique<LceSDSLsada>(file_path);
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
        //sdsl::ram_fs::remove(tmp_file);
      } else if (algorithm == "sct3") {
        size_t const mem_before = malloc_count_current();
        t.reset();
        lce_structure = std::make_unique<LceSDSLsada>(file_path);
        construction_times.add(t.get_and_reset());
        lce_mem.add(malloc_count_current() - mem_before);
        construction_mem_peak.add(malloc_count_peak() - mem_before);
      } else {
        return;
      }
    }

    std::cout << "construction_min_time=" << construction_times.min() << " "
              << "construction_max_time=" << construction_times.max() << " "
              << "construction_avg_time=" << construction_times.avg() << " "

              << "input=" << file_path << " "
              << "size=" << text.size() << " "

              << "lce_mem=" << lce_mem.max() << " "
              << "construction_mem_peak=" << construction_mem_peak.max() << " "
              << "threads=" << omp_get_max_threads() << " ";
    std::cout << std::endl;

    std::vector<uint64_t> lce_indices(number_lce_queries * 2);
    bool correct = true;
    size_t wrong_queries = 0;

    for (size_t i = lce_from; i < lce_to; ++i) {
      tlx::Aggregate<size_t> queries_times;
      tlx::Aggregate<size_t> lce_values;
      std::cout << "RESULT "
                << "algo=" << print_algo_name() << "_queries "
                << "runs=" << runs << " "
                << "length_exp=" << i << " "
                << "input=" << file_path << " "
                << "size=" << text.size() << " ";
      vector<uint64_t> v;
      std::ifstream lc(lce_set[i], ios::in);
      util::inputErrorHandling(&lc);
          
      string line;
      string::size_type sz;
      while(getline(lc, line)) {
        v.push_back(stoi(line, &sz));
      }
      lc.close();

      if (v.size() > 0) {
        for(uint64_t i = 0; i < number_lce_queries * 2; ++i) {
          lce_indices[i] = v[i % v.size()];
        }
        for (size_t i = 0; i < runs; ++i) {
          t.reset();
          for (size_t j = 0; j < number_lce_queries * 2; j += 2) {
            size_t const lce = lce_structure->lce(lce_indices[j],
                                                  lce_indices[j + 1]);
            lce_values.add(lce);
          }
          queries_times.add(t.get_and_reset());
        }
        if (check) {
          correct = true;
          auto check_text = load_text(file_path, prefix_length);
          auto lce_naive = LceUltraNaive(check_text);
          for (size_t j = 0; j < number_lce_queries * 2; j += 2) {
            size_t const lce = lce_structure->lce(lce_indices[j],
                                                  lce_indices[j + 1]);
            size_t const lce_res_naive = lce_naive.lce(lce_indices[j],
                                                        lce_indices[j + 1]);
            if (lce != lce_res_naive) {
              correct = false;
              ++wrong_queries;
            }
          }
        }
      }
      std::cout << "lce_values_min=" << lce_values.min() << " "
                << "lce_values_max=" << lce_values.max() << " "
                << "lce_values_avg=" << lce_values.avg() << " "
                << "lce_values_count=" << lce_values.count() << " "
                << "queries_times_min=" << queries_times.min() << " "
                << "queries_times_max=" << queries_times.max() << " "
                << "queries_times_avg=" << queries_times.avg() << " "
                << "check="
                << (check ? (correct ? "passed" :
                              ("failed(" + std::to_string(wrong_queries)
                              + ")" )) : "none") << " "
                << std::endl;
    }
  }


public:
  std::string file_path;
  std::string output_path = "/tmp/res_lce/";
  uint64_t prefix_length = 0;

  std::string algorithm = "u";
  bool prefer_long_queries = false;

  bool check = false;

  size_t number_lce_queries = 1000000;
  uint32_t runs = 5;

  uint32_t lce_from = 0;
  uint32_t lce_to = 21;

private:
  std::string print_algo_name() {
    std::string name("unknown");
    if (algorithm == "u") {
      name = "ultra_naive";
    } else if (algorithm == "n") {
      name = "naive";
    } else if (algorithm == "nx") {
      name = "naive_xor";
    } else if (algorithm == "m") {
      name = "prezza_mersenne";
    } else if (algorithm == "p") {
      name = "prezza";
    } else if (algorithm == "s2048") {
      name = "sss2048";
    } else if (algorithm == "s1024") {
      name = "sss1024";
    } else if (algorithm == "s512") {
      name = "sss512";
    } else if (algorithm == "s256") {
      name = "sss256";
    } else if (algorithm == "s2048_par") {
      name = "sss2048_par";
    } else if (algorithm == "s1024_par") {
      name = "sss1024_par";
    } else if (algorithm == "s512_par") {
      name = "sss512_par";
    } else if (algorithm == "s256_par") {
      name = "sss256_par";
    } else if (algorithm == "sada") {
      name = "sdsl_sada";
    } else if (algorithm == "sct3") {
      name = "sdsl_sct3";
    }

    if (name.rfind("sss", 0) == 0 && prefer_long_queries) {
      name.append("pl");
    }

    return name;
  }

}; // class lce_benchmark

int32_t main(int argc, char *argv[]) {
  lce_benchmark lce_bench;

  tlx::CmdlineParser cp;
  cp.set_description("This programs measures construction time and LCE query "
                     "time for several LCE data structures.");
  cp.set_author("Alexander Herlez <alexander.herlez@tu-dortmund.de>\n"
                "        Florian Kurpicz  <florian.kurpicz@tu-dortmund.de>\n"
                "        Patrick Dinklage <patrick.dinklage@tu-dortmund.de>");

  cp.add_param_string("file", lce_bench.file_path, "The text which is queried");
  cp.add_string('o', "output_path", lce_bench.output_path, "Path where LCE "
                "queries for [-m]ode [s]orted are stored "
                "(default: /tmp/res_lce).");
  cp.add_bytes('p', "pre", lce_bench.prefix_length, "Size of the prefix in "
               "bytes that will be read (optional).");
  cp.add_string('a', "algorithm", lce_bench.algorithm, "LCP data structure "
                "that is computed: [u]ltra naive (default), [n]aive, "
                "prezza [m]ersenne, [p]rezza, or [s]tring synchronizing sets "
                " with Tau = 1024. [s512] and [s256] for Tau = 512 and 256, "
                "resp.");
  cp.add_flag('l', "long", lce_bench.prefer_long_queries, "Prefer long queries,"
              " i.e., queries with long LCE get faster, all other get slower. "
              "Only for [s]tring synchronizing sets.");
  cp.add_flag('c', "check", lce_bench.check, "Check correctness of LCE queries "
              "by comparing with results of naive computation.");
  cp.add_size_t('q', "queries", lce_bench.number_lce_queries, "Number of LCE "
              "queries that are executed (default=1,000,000).");
  cp.add_uint('r', "runs", lce_bench.runs, "Number of runs that are used to "
              "report an average running time (default=5).");
  cp.add_uint("from", lce_bench.lce_from, "Use only lce "
              "queries which return at least 2^{from} (optional).");
  cp.add_uint("to", lce_bench.lce_to, "Use only lce queries "
              "which return less than 2^{from} with from < 22 (optional)");

  if (!cp.process(argc, argv)) {
    std::exit(EXIT_FAILURE);
  }

  lce_bench.run();
  return 0;
}

/******************************************************************************/
