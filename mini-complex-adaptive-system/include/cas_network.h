#ifndef CAS_NETWORK_H
#define CAS_NETWORK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * Interaction Networks for CAS (L1-L4)
 *
 * In CAS, agents interact through networks. The network structure
 * fundamentally shapes system-level behavior: emergence, propagation
 * of information, cascading failures, and collective adaptation.
 *
 * L1: Node, edge, adjacency matrix, network types
 * L2: Centrality, clustering, path length, degree distribution
 * L3: Graph Laplacian, spectral properties, adjacency algebra
 * L4: Small-world theorem (Watts-Strogatz), scale-free (Barabasi-Albert)
 *
 * Reference:
 *   Newman, M.E.J. (2010). Networks: An Introduction. Oxford.
 *   Watts, D.J. & Strogatz, S.H. (1998). Nature 393:440-442.
 *   Barabasi, A.-L. & Albert, R. (1999). Science 286:509-512.
 * ========================================================================= */

#define CAS_NET_MAX_NODES    50
#define CAS_NET_MAX_EDGES    512
#define CAS_NET_MAX_DEGREE   32

/* L1: Network type */
typedef enum {
    CAS_NET_ERDOS_RENYI   = 0,
    CAS_NET_WATTS_STROGATZ = 1,
    CAS_NET_BARABASI_ALBERT = 2,
    CAS_NET_COMPLETE      = 3,
    CAS_NET_CYCLE         = 4,
    CAS_NET_GRID_2D       = 5,
    CAS_NET_STAR          = 6,
    CAS_NET_CUSTOM        = 7
} cas_network_type_t;

/* L1: Edge */
typedef struct {
    uint32_t from;
    uint32_t to;
    double   weight;
    bool     directed;
    uint32_t created_at;
} cas_edge_t;

/* L1: Node attributes */
typedef struct {
    uint32_t id;
    double   degree;
    double   clustering_coeff;
    double   betweenness;
    double   closeness;
    double   eigenvector_centrality;
    double   pagerank;
    uint32_t community_id;
    bool     is_hub;
} cas_node_attr_t;

/* L3: Graph structure */
typedef struct {
    uint32_t num_nodes;
    uint32_t num_edges;
    cas_network_type_t type;
    cas_edge_t edges[CAS_NET_MAX_EDGES];
    double   adjacency[CAS_NET_MAX_NODES][CAS_NET_MAX_NODES];
    uint32_t degree[CAS_NET_MAX_NODES];
    uint32_t neighbors[CAS_NET_MAX_NODES][CAS_NET_MAX_DEGREE];
    double   global_clustering;
    double   avg_path_length;
    double   diameter;
    double   assortativity;
    bool     is_connected;
    uint32_t num_components;
    cas_node_attr_t node_attr[CAS_NET_MAX_NODES];
    double   laplacian_spectrum[CAS_NET_MAX_NODES];
    double   modularity;
} cas_network_t;

/* L5: Network API */
void cas_net_init(cas_network_t *net, uint32_t num_nodes,
                  cas_network_type_t type, uint64_t seed);
void cas_net_add_edge(cas_network_t *net, uint32_t from, uint32_t to,
                      double weight, bool directed);
void cas_net_remove_edge(cas_network_t *net, uint32_t from, uint32_t to);
void cas_net_build_adjacency(cas_network_t *net);
void cas_net_erdos_renyi(cas_network_t *net, double p, uint64_t *seed);
void cas_net_watts_strogatz(cas_network_t *net, uint32_t k, double beta,
                             uint64_t *seed);
void cas_net_barabasi_albert(cas_network_t *net, uint32_t m0, uint32_t m,
                              uint64_t *seed);

double cas_net_clustering_coeff(const cas_network_t *net, uint32_t node);
void cas_net_global_clustering(cas_network_t *net);
double cas_net_avg_path_length(const cas_network_t *net);
void cas_net_diameter(cas_network_t *net);
void cas_net_degree_distribution(const cas_network_t *net,
                                 uint32_t *dist, uint32_t max_degree);
double cas_net_assortativity(cas_network_t *net);
bool cas_net_is_connected(const cas_network_t *net);
uint32_t cas_net_connected_components(cas_network_t *net,
                                      uint32_t components[CAS_NET_MAX_NODES]);
void cas_net_betweenness_centrality(cas_network_t *net);
void cas_net_eigenvector_centrality(cas_network_t *net, uint32_t max_iter);
void cas_net_pagerank(cas_network_t *net, double damping, uint32_t max_iter);
void cas_net_louvain_communities(cas_network_t *net);
double cas_net_modularity(const cas_network_t *net);

void cas_net_random_walk(const cas_network_t *net, uint32_t start,
                         uint32_t steps, uint32_t *path, uint64_t *seed);
void cas_net_cascade(const cas_network_t *net, uint32_t seed_node,
                     double threshold, bool *affected);
void cas_net_laplacian_spectrum(cas_network_t *net);
double cas_net_algebraic_connectivity(const cas_network_t *net);
double cas_net_spectral_radius(const cas_network_t *net);

#endif /* CAS_NETWORK_H */
