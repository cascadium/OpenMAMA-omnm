//
// Created by fquinn on 6/8/20.
//

#ifndef OPENMAMA_OMNM_BENCHMARK_H
#define OPENMAMA_OMNM_BENCHMARK_H

#include <stdint.h>

class Benchmarker {
public:
    void runIterationTests(uint64_t repeats, bool readOnly = false, bool directAccess = false);
    void runSerializationTests(uint64_t repeats);
};


#endif //OPENMAMA_OMNM_BENCHMARK_H
