#pragma once

#define outf printf

namespace mtca {
    // Convention: all items of associations and lists have a trailing comma.
    // We therefore always use Sequence[] between the last item and closing delimiter.

    enum nest_type {dummy, list, assoc, item};

    // Can support nesting up to depth 32.
    static uint64_t nesting = 0;

    static void nest_open(nest_type n) {
        if ((nesting & (((uint64_t) 0b11) << 62)) != 0) {
            eprintf("Delimiter nesting depth limit (32) exceeded\n");
        }
        nesting <<= 2;
        nesting += n;
    }

    static void nest_close(nest_type n) {
        const nest_type n_open = (nest_type)(nesting & 0b11);
        if (n_open != n) {
            eprintf("Closed delimiter type %i with type %i\n", n_open, n);
        }
        nesting >>= 2;
    }

    void check_nesting() {
        const nest_type n_open = (nest_type)(nesting & 0b11);
        if (nesting != 0) {
            eprintf("Delimiter of type %i still open\n", n_open);
        }
    }

    void list_open() {
        nest_open(list);
        outf("{\n");
    }

    void list_close() {
        nest_close(list);
        outf("Sequence[]}\n");
    }

    void list_item_open() {
        nest_open(item);
        outf("(");
    }

    void list_item_close() {
        nest_close(item);
        outf("),\n");
    }

    void list_item(uint64_t data) {
        outf("%lu,\n", data);
    }

    void list_item(const char* data) {
        outf("\"%s\",\n", data);
    }

    void list_item(const std::vector<uint64_t>& datas) {
        list_item_open();
        list_open();
        for (const auto& data : datas) {
            list_item(data);
        }
        list_close();
        list_item_close();
    }

    void assoc_open() {
        nest_open(assoc);
        outf("<|");
    }

    void assoc_close() {
        nest_close(assoc);
        outf("Sequence[]|>");
    }

    // Internal
    static void __assoc_item_header(const char* name) {
        outf("\"%s\" -> ", name);
    }

    void assoc_item_open(const char* name) {
        __assoc_item_header(name);
        list_item_open();
    }

    void assoc_item_close() {
        list_item_close();
    }

    void assoc_item(const char* name, uint64_t data) {
        __assoc_item_header(name);
        outf("%lu, ", data);
    }

    void assoc_item(const char* name, const char* data) {
        __assoc_item_header(name);
        outf("\"%s\", ", data);
    }

    void assoc_item(const char* name, const std::vector<uint64_t>& datas) {
        assoc_item_open(name);
        list_open();
        for (const auto& data : datas) {
            list_item(data);
        }
        list_close();
        assoc_item_close();
    }
}
