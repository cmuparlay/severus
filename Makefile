CC = g++
CFLAGS = -mcx16 -O2 -Wall -std=c++11 -march=native -pthread
CLIBS = -lnuma -lboost_program_options
DEFS_SEQUENCE = -DONELOC -DMAPPING -DNODELAY -DLOG -DXADD
DEFS_COMPETITION = -DONELOC -DMAPPING -DMAPPING2
DEFS_CUSTOM = -imacros custom_configuration.hpp
SOURCES = main.cpp run.hpp protocols.hpp get_time.hpp parallel.hpp init.hpp histo.hpp trace.hpp hash.hpp data_item.hpp helper.hpp mathematica.hpp sized_array.hpp
DEPS_MAIN = numa_configuration.hpp $(SOURCES)

.PHONY: default sequence competition custom clean

default: sequence competition

sequence: main_sequence
main_sequence: $(DEPS_MAIN)
	$(CC) $(DEFS_SEQUENCE) $(CFLAGS) main.cpp $(CLIBS) -o main_sequence

competition: main_competition
main_competition: $(DEPS_MAIN)
	$(CC) $(DEFS_COMPETITION) $(CFLAGS) main.cpp $(CLIBS) -o main_competition

custom: main_custom
main_custom: custom_configuration.hpp $(DEPS_MAIN)
	$(CC) $(DEFS_CUSTOM) $(CFLAGS) main.cpp $(CLIBS) -o main_custom

numa_configuration.hpp: numa_configure
	./numa_configure > numa_configuration.hpp

numa_configure: numa_configure.cpp
	$(CC) $(CFLAGS) numa_configure.cpp $(CLIBS) -o numa_configure

clean:
	rm main_competition main_sequence main_custom numa_configuration.hpp numa_configure
