#pragma once
// Manually set or unset these options, then run "make custom".

// NOREAD: don't read at all
// #define NOREAD

// ONEREAD: read once per success, in the first iteration
// #define ONEREAD

// CCAS: use compare-compare-and-swap instead of normal CAS
// #define CCAS

// XADD: run atomic fetch&add instead of CAS.
#define XADD

// LOG: keep log of events.
// If defined, must have XADD defined, too
#define LOG

// DEBUG: print debug information
#define DEBUG

// MAPPING: workers on node x access data on node mapping[x]
// Limited functionality when XADD is defined
#define MAPPING

// MAPPING2: workers on node x may also access data on node mapping2[x]
// If defined, must have MAPPING defined, too
// Not supported when XADD is defined
// #define MAPPING2

// ONELOC: only use one location (or, with mapping, one location per node)
#define ONELOC

// NODELAY: no delays
#define NODELAY
