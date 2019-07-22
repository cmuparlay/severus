#pragma once

#include "get_time.hpp"

#define MAX_NUM_WORKERS 144

namespace par {
    static timer bt;
    static pthread_attr_t attr;
    static cpu_set_t cpu;
    static pthread_t threads[MAX_NUM_WORKERS];
    static pthread_t threads_len;

    template <typename X>
    struct with_worker {
        uint64_t worker;
        X value;
    };

    // wxs: pairs of w = worker (CPU) number, x = function input
    // wxs_len: length of wxs
    // f: function from worker-input pairs to void
    template <typename X>
    static void create_for
        (with_worker<X> *wxs,
         size_t wxs_len,
         void *(*f)(with_worker<X> *)) {
        threads_len = wxs_len;
        pthread_attr_init(&attr);
        bt.start();
        for (size_t i = 0; i < threads_len; i++) {
            CPU_ZERO(&cpu);
            CPU_SET(wxs[i].worker, &cpu);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu);
            void *(*f_void) (void *) = (void *(*) (void *)) f;
            pthread_create(&threads[i], &attr, f_void, (void *) &wxs[i]);
        }
    }

    // returns: time since last create_for
    static double join_for () {
        for (size_t i = 0; i < threads_len; i++) {
            pthread_join(threads[i], NULL);
        }
        return bt.stop();
    }
}
