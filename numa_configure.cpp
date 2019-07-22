#include <stdio.h>
#include <numa.h>

#define def(name,args...)                       \
  do {                                          \
    printf("#ifndef %s\n", name);               \
    printf("#define %s ", name);                \
    printf(args);                               \
    printf("\n#endif\n");                       \
  } while (false)

int log2(int n) {
  if (n <= 0) {
    fprintf(stderr, "ERROR: tried to take log2 of nonpositive integer.\n");
    exit(1);
  }
  int log = 0;
  while (n % 2 == 0) {
    log++;
    n /= 2;
  }
  if (n > 1) {
    log ++;
    while (n > 1) {
      log++;
      n /= 2;
    }
  }
  return log;
}

bool is_round_robin(int num_nodes, int num_cpus) {
  if (num_cpus % num_nodes != 0) {
    return false;
  }
  for (int cpu = 0; cpu < num_cpus; cpu++) {
    if (numa_node_of_cpu(cpu) != cpu % num_nodes) {
      return false;
    }
  }
  return true;
}

bool is_blocks(int num_nodes, int num_cpus) {
  bool *is_node_used = new bool[num_nodes];
  const int cpus_per_node = num_cpus / num_nodes;
  bool okay = true;
  for (int node_index = 0; node_index < num_nodes; node_index++) {
    const int first_cpu = node_index * cpus_per_node;
    const int node = numa_node_of_cpu(first_cpu);
    if (is_node_used[node]) {
      okay = false;
    }
    is_node_used[node] = true;
    for (int cpu = first_cpu + 1; cpu < first_cpu + cpus_per_node; cpu++) {
      if (numa_node_of_cpu(cpu) != node) {
        okay = false;
      }
    }
  }
  delete[] is_node_used;
  return okay;
}

int main(int argc, char** argv) {
  if (numa_available() == -1) {
    fprintf(stderr, "ERROR: NUMA not available.\n");
    exit(1);
  }

  const int num_nodes = numa_num_configured_nodes();
  const int num_nodes_shift = log2(num_nodes);
  const int num_cpus = numa_num_configured_cpus();
  const int num_cpus_shift = log2(num_cpus);
  const bool round_robin = is_round_robin(num_nodes, num_cpus);
  const bool blocks = is_blocks(num_nodes, num_cpus);

  if (!round_robin && !blocks) {
    fprintf(stderr, "ERROR: CPU numbering pattern is neither blocks nor round-robin.\n");
    exit(1);
  }

  if (argc >= 2) {
    const int cpus_per_node = num_cpus / num_nodes;
    const int spacing = 2;
    for (int i = 1; i < argc; i++) {
      const int cpu = atoi(argv[i]);
      if (0 <= cpu && cpu < num_cpus) {
        int node;
        int smt_pair;
        int smt_parity;
        if (round_robin) {
          node = cpu % num_nodes;
          smt_pair = (cpu / num_nodes) % (cpus_per_node / 2);
          smt_parity = (cpu / num_nodes) / (cpus_per_node / 2);
        } else {
          node = numa_node_of_cpu(cpu);
          smt_pair = (cpu % cpus_per_node) / 2;
          smt_parity = cpu % 2;
        }
        const int cpu_position = ((cpus_per_node + spacing) * node) + (2 * smt_pair) + smt_parity;
        printf("%d\n", cpu_position);
      } else {
        // Hack: if the argument isn't a valid CPU, print plot labels
        printf("set xtics (");
        for (int node = 0; node < num_nodes; node++) {
            printf("\"node %d\" %d, ", node, cpus_per_node/2 + (cpus_per_node + spacing) * node);
        }
        printf(")\n");
      }
    }
  } else {
    printf("#pragma once\n");
    def("NUM_NODES", "%d", num_nodes);
    def("NUM_NODES_SHIFT", "%d", num_nodes_shift);
    def("NUM_CPUS", "%d", num_cpus);
    def("NUM_CPUS_SHIFT", "%d", num_cpus_shift);
    def("ROUND_ROBIN", "%s", round_robin ? "true" : "false");
  }
  return 0;
}
