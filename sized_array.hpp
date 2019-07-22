#pragma once
template <typename T>
struct sized_array {
    T* array;
    uint64_t len;
};
