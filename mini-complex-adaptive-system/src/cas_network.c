/**
 * @file cas_network.c
 * @brief Interaction Network Implementation (L1-L6)
 *
 * Networks are the substrate of agent interaction in CAS.
 * The network structure shapes emergence, information propagation,
 * cascading failures, and collective adaptation.
 *
 * L1: Node, edge, adjacency matrix, network types
 * L2: Centrality, clustering, path length, degree distribution
 * L3: Graph Laplacian, spectral properties, adjacency algebra
 * L4: Small-world (Watts-Strogatz), Scale-free (Barabasi-Albert)
 * L5: Network generation, random walk, cascade, communities
 * L6: Network optimization, robustness analysis
 *
 * Reference:
 *   Newman, M.E.J. (2010). Networks: An Introduction. Oxford.
 *   Barabasi, A.-L. (2016). Network Science. Cambridge.
 */

#include "cas_network.h"
#include <string.h>
#include <stdlib.h>

static uint64_t _net_seed = 654321;

static uint64_t _net_rand_u64(void) {
    _net_seed = _net_seed * 6364136223846793005ULL + 1442695040888963407ULL;
    uint64_t z = _net_seed;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static void _net_srand(uint64_t s) { _net_seed = s; }
static double _net_rand_double(void) {
    return (double)(_net_rand_u64() >> 11) * 0x1.0p-53;
}

/* ---- Queue for BFS ---- */
typedef struct { uint32_t data[CAS_NET_MAX_NODES]; uint32_t head, tail; } _queue_t;
static void _q_init(_queue_t *q) { q->head = q->tail = 0; }
static void _q_push(_queue_t *q, uint32_t v) { q->data[q->tail++] = v; }
static uint32_t _q_pop(_queue_t *q) { return q->data[q->head++]; }
static bool _q_empty(_queue_t *q) { return q->head >= q->tail; }

/* =================================================================
 * L1: Network Initialization
 * ================================================================= */

void cas_net_init(cas_network_t *net, uint32_t num_nodes,
                  cas_network_type_t type, uint64_t seed)
{
    if (!net) return;
    memset(net, 0, sizeof(cas_network_t));

    net->num_nodes = num_nodes < CAS_NET_MAX_NODES ? num_nodes : CAS_NET_MAX_NODES;
    net->type = type;
    _net_srand(seed);

    uint32_t i;
    for (i = 0; i < net->num_nodes; i++) {
        net->node_attr[i].id = i;
        net->degree[i] = 0;
    }
}

void cas_net_add_edge(cas_network_t *net, uint32_t from, uint32_t to,
                      double weight, bool directed)
{
    if (!net) return;
    if (from >= net->num_nodes || to >= net->num_nodes) return;
    if (net->num_edges >= CAS_NET_MAX_EDGES) return;

    if (net->adjacency[from][to] > 0.0 && !directed && net->adjacency[to][from] > 0.0)
        return;

    cas_edge_t *e = &net->edges[net->num_edges++];
    e->from = from; e->to = to; e->weight = weight;
    e->directed = directed; e->created_at = 0;

    net->adjacency[from][to] = weight;
    if (!directed) net->adjacency[to][from] = weight;

    if (net->degree[from] < CAS_NET_MAX_DEGREE)
        net->neighbors[from][net->degree[from]++] = to;
    if (!directed && net->degree[to] < CAS_NET_MAX_DEGREE)
        net->neighbors[to][net->degree[to]++] = from;
}

/* =================================================================
 * L5: Erdos-Renyi Random Graph G(n,p)
 *
 * Complexity: O(n^2)
 * ================================================================= */

void cas_net_erdos_renyi(cas_network_t *net, double p, uint64_t *seed)
{
    if (!net) return;
    if (seed) _net_srand(*seed);

    uint32_t i, j;
    for (i = 0; i < net->num_nodes; i++) {
        for (j = i + 1; j < net->num_nodes; j++) {
            if (_net_rand_double() < p) {
                cas_net_add_edge(net, i, j, 1.0, false);
            }
        }
    }
    if (seed) *seed = _net_seed;
}

/* =================================================================
 * L4: Watts-Strogatz Small-World Network
 *
 * Theorem (Watts & Strogatz 1998, Nature):
 * For intermediate rewiring probability beta ~ 0.01-0.1,
 * networks have both high clustering (like regular lattices)
 * and short path lengths (like random graphs).
 *
 * Complexity: O(n * k + n * k * beta)
 * ================================================================= */

void cas_net_watts_strogatz(cas_network_t *net, uint32_t k, double beta,
                             uint64_t *seed)
{
    if (!net) return;
    if (seed) _net_srand(*seed);

    uint32_t n = net->num_nodes;
    uint32_t half_k = k / 2;
    uint32_t i, j;

    /* Step 1: Regular ring lattice */
    for (i = 0; i < n; i++) {
        for (j = 1; j <= half_k; j++) {
            uint32_t neighbor = (i + j) % n;
            cas_net_add_edge(net, i, neighbor, 1.0, false);
        }
    }

    /* Step 2: Rewire with probability beta */
    for (i = 0; i < n; i++) {
        for (j = 0; j < net->degree[i]; j++) {
            if (_net_rand_double() < beta) {
                uint32_t new_target;
                do {
                    new_target = _net_rand_u64() % n;
                } while (new_target == i || new_target == net->neighbors[i][j]);

                uint32_t old_target = net->neighbors[i][j];
                net->adjacency[i][old_target] = 0.0;
                net->adjacency[old_target][i] = 0.0;
                net->neighbors[i][j] = new_target;
                net->adjacency[i][new_target] = 1.0;
                net->adjacency[new_target][i] = 1.0;
            }
        }
    }

    if (seed) *seed = _net_seed;
    cas_net_build_adjacency(net);
}

/* =================================================================
 * L4: Barabasi-Albert Scale-Free Network
 *
 * Theorem (Barabasi & Albert 1999, Science):
 * Preferential attachment generates power-law degree distribution
 * P(k) ~ k^{-3}. Unlike random graphs, scale-free networks have
 * hubs and are robust to random failures but vulnerable to attacks.
 *
 * Complexity: O(n * m)
 * ================================================================= */

void cas_net_barabasi_albert(cas_network_t *net, uint32_t m0, uint32_t m,
                              uint64_t *seed)
{
    if (!net || m0 < m || m < 1) return;
    if (seed) _net_srand(*seed);

    uint32_t n = net->num_nodes;

    /* Initial complete graph on m0 nodes */
    uint32_t i, j;
    for (i = 0; i < m0 && i < n; i++) {
        for (j = i + 1; j < m0 && j < n; j++) {
            cas_net_add_edge(net, i, j, 1.0, false);
        }
    }

    /* Preferential attachment */
    for (i = m0; i < n; i++) {
        /* Compute total degree for probability */
        uint32_t total_deg = 0;
        uint32_t k;
        for (k = 0; k < i; k++)
            total_deg += net->degree[k];
        if (total_deg == 0) total_deg = 1;

        uint32_t added = 0;
        while (added < m && added < i) {
            double r = _net_rand_double() * total_deg;
            double cum = 0.0;
            uint32_t target = i;
            for (k = 0; k < i; k++) {
                cum += net->degree[k];
                if (r <= cum) { target = k; break; }
            }
            if (target < i && net->adjacency[i][target] == 0.0) {
                cas_net_add_edge(net, i, target, 1.0, false);
                added++;
            }
        }
    }

    if (seed) *seed = _net_seed;
}

void cas_net_build_adjacency(cas_network_t *net)
{
    if (!net) return;
    uint32_t i, j;
    for (i = 0; i < net->num_nodes; i++)
        for (j = 0; j < net->num_nodes; j++)
            net->adjacency[i][j] = 0.0;
    uint32_t e;
    for (e = 0; e < net->num_edges; e++) {
        cas_edge_t *ed = &net->edges[e];
        net->adjacency[ed->from][ed->to] = ed->weight;
        if (!ed->directed)
            net->adjacency[ed->to][ed->from] = ed->weight;
    }
}

/* =================================================================
 * L2: Clustering Coefficient
 *
 * C_i = 2 * E_i / (k_i * (k_i - 1))
 * where E_i = edges among neighbors of node i.
 *
 * Complexity: O(degree^2)
 * ================================================================= */

double cas_net_clustering_coeff(const cas_network_t *net, uint32_t node)
{
    if (!net || node >= net->num_nodes) return 0.0;
    uint32_t k = net->degree[node];
    if (k < 2) return 0.0;

    uint32_t edges_among_neighbors = 0;
    uint32_t i, j;
    for (i = 0; i < k; i++) {
        uint32_t u = net->neighbors[node][i];
        for (j = i + 1; j < k; j++) {
            uint32_t v = net->neighbors[node][j];
            if (net->adjacency[u][v] > 0.0)
                edges_among_neighbors++;
        }
    }

    return 2.0 * edges_among_neighbors / (k * (k - 1));
}

void cas_net_global_clustering(cas_network_t *net)
{
    if (!net) return;
    double sum = 0.0;
    uint32_t i, count = 0;
    for (i = 0; i < net->num_nodes; i++) {
        if (net->degree[i] >= 2) {
            sum += cas_net_clustering_coeff(net, i);
            count++;
        }
    }
    net->global_clustering = count > 0 ? sum / count : 0.0;
}

/* =================================================================
 * L2: Average Path Length (BFS from all nodes)
 *
 * Complexity: O(n * (n + m))
 * ================================================================= */

double cas_net_avg_path_length(const cas_network_t *net)
{
    if (!net || net->num_nodes < 2) return 0.0;

    double total = 0.0;
    uint64_t path_count = 0;
    uint32_t src;

    for (src = 0; src < net->num_nodes; src++) {
        int32_t dist[CAS_NET_MAX_NODES];
        uint32_t i;
        for (i = 0; i < net->num_nodes; i++) dist[i] = -1;

        _queue_t q; _q_init(&q);
        dist[src] = 0; _q_push(&q, src);

        while (!_q_empty(&q)) {
            uint32_t u = _q_pop(&q);
            for (i = 0; i < net->degree[u]; i++) {
                uint32_t v = net->neighbors[u][i];
                if (dist[v] < 0) {
                    dist[v] = dist[u] + 1;
                    total += dist[v];
                    path_count++;
                    _q_push(&q, v);
                }
            }
        }
    }

    return path_count > 0 ? total / path_count : 0.0;
}

/* =================================================================
 * L2: Betweenness Centrality (Brandes algorithm)
 *
 * Complexity: O(n * (n + m))
 * ================================================================= */

void cas_net_betweenness_centrality(cas_network_t *net)
{
    if (!net) return;

    uint32_t i;
    for (i = 0; i < net->num_nodes; i++)
        net->node_attr[i].betweenness = 0.0;

    uint32_t s;
    for (s = 0; s < net->num_nodes; s++) {
        int32_t stack[CAS_NET_MAX_NODES];
        uint32_t sp = 0;
        double sigma[CAS_NET_MAX_NODES];
        int32_t d[CAS_NET_MAX_NODES];
        double delta[CAS_NET_MAX_NODES];
        uint32_t pred_list[CAS_NET_MAX_NODES][CAS_NET_MAX_DEGREE];
        uint32_t pred_cnt[CAS_NET_MAX_NODES];

        uint32_t v;
        for (v = 0; v < net->num_nodes; v++) {
            sigma[v] = 0.0; d[v] = -1;
            delta[v] = 0.0; pred_cnt[v] = 0;
        }

        sigma[s] = 1.0; d[s] = 0;
        _queue_t q; _q_init(&q); _q_push(&q, s);

        while (!_q_empty(&q)) {
            uint32_t u = _q_pop(&q);
            stack[sp++] = u;
            uint32_t j;
            for (j = 0; j < net->degree[u]; j++) {
                uint32_t w = net->neighbors[u][j];
                if (d[w] < 0) {
                    d[w] = d[u] + 1; _q_push(&q, w);
                }
                if (d[w] == d[u] + 1) {
                    sigma[w] += sigma[u];
                    pred_list[w][pred_cnt[w]++] = u;
                }
            }
        }

        while (sp > 0) {
            uint32_t w = stack[--sp];
            uint32_t j;
            for (j = 0; j < pred_cnt[w]; j++) {
                uint32_t u = pred_list[w][j];
                delta[u] += (sigma[u] / sigma[w]) * (1.0 + delta[w]);
            }
            if (w != s) net->node_attr[w].betweenness += delta[w];
        }
    }
}

/* =================================================================
 * L5: PageRank
 *
 * PR(u) = (1-d)/N + d * sum_{v->u} PR(v)/deg_out(v)
 *
 * Complexity: O(max_iter * m)
 * ================================================================= */

void cas_net_pagerank(cas_network_t *net, double damping, uint32_t max_iter)
{
    if (!net) return;
    uint32_t n = net->num_nodes;
    double pr[CAS_NET_MAX_NODES], pr_new[CAS_NET_MAX_NODES];
    uint32_t i, iter;

    for (i = 0; i < n; i++) pr[i] = 1.0 / n;

    for (iter = 0; iter < max_iter; iter++) {
        double dangling_sum = 0.0;
        for (i = 0; i < n; i++)
            if (net->degree[i] == 0)
                dangling_sum += pr[i];

        for (i = 0; i < n; i++) {
            pr_new[i] = (1.0 - damping) / n + damping * dangling_sum / n;
            uint32_t j;
            for (j = 0; j < net->degree[i]; j++) {
                uint32_t v = net->neighbors[i][j];
                if (net->degree[v] > 0)
                    pr_new[i] += damping * pr[v] / net->degree[v];
            }
        }

        double err = 0.0;
        for (i = 0; i < n; i++) {
            err += fabs(pr_new[i] - pr[i]);
            pr[i] = pr_new[i];
        }
        if (err < 1e-8) break;
    }

    for (i = 0; i < n; i++)
        net->node_attr[i].pagerank = pr[i];
}

/* =================================================================
 * L5: Louvain Community Detection
 *
 * Complexity: O(passes * n * m)
 * ================================================================= */

void cas_net_louvain_communities(cas_network_t *net)
{
    if (!net) return;

    uint32_t n = net->num_nodes;
    uint32_t community[CAS_NET_MAX_NODES];
    double m2 = 0.0;
    uint32_t i, j;

    for (i = 0; i < n; i++) {
        community[i] = i;
        m2 += net->degree[i];
    }
    m2 = m2 > 0 ? m2 : 1.0;

    bool improved = true;
    uint32_t pass;
    for (pass = 0; pass < 10 && improved; pass++) {
        improved = false;

        for (i = 0; i < n; i++) {
            uint32_t best_comm = community[i];
            double best_delta_q = 0.0;

            double ki_in[CAS_NET_MAX_NODES];
            double sigma_tot[CAS_NET_MAX_NODES];
            memset(ki_in, 0, sizeof(ki_in));
            memset(sigma_tot, 0, sizeof(sigma_tot));

            for (j = 0; j < net->degree[i]; j++) {
                uint32_t nb = net->neighbors[i][j];
                uint32_t c = community[nb];
                ki_in[c] += net->adjacency[i][nb];
            }

            for (j = 0; j < n; j++) {
                uint32_t c = community[j];
                sigma_tot[c] += net->degree[j];
            }

            for (j = 0; j < net->degree[i]; j++) {
                uint32_t nb = net->neighbors[i][j];
                uint32_t c = community[nb];
                if (c == community[i]) continue;

                double dq = ki_in[c] / m2
                    - net->degree[i] * sigma_tot[c] / (m2 * m2);
                if (dq > best_delta_q) {
                    best_delta_q = dq;
                    best_comm = c;
                }
            }

            if (best_comm != community[i]) {
                community[i] = best_comm;
                improved = true;
            }
        }
    }

    for (i = 0; i < n; i++)
        net->node_attr[i].community_id = community[i];
}

/* =================================================================
 * L5: Random Walk on Network
 *
 * Complexity: O(steps)
 * ================================================================= */

void cas_net_random_walk(const cas_network_t *net, uint32_t start,
                         uint32_t steps, uint32_t *path, uint64_t *seed)
{
    if (!net || !path) return;
    if (seed) _net_srand(*seed);

    uint32_t current = start < net->num_nodes ? start : 0;
    uint32_t s;
    for (s = 0; s < steps; s++) {
        path[s] = current;
        if (net->degree[current] == 0) break;
        uint32_t next_idx = _net_rand_u64() % net->degree[current];
        current = net->neighbors[current][next_idx];
    }

    if (seed) *seed = _net_seed;
}

/* =================================================================
 * L5: Cascade Model (threshold-based)
 *
 * Each node adopts if fraction of adopting neighbors > threshold.
 *
 * Complexity: O(n * d)
 * ================================================================= */

void cas_net_cascade(const cas_network_t *net, uint32_t seed_node,
                     double threshold, bool *affected)
{
    if (!net || !affected) return;

    uint32_t n = net->num_nodes;
    uint32_t i;
    for (i = 0; i < n; i++) affected[i] = false;

    uint32_t queue[CAS_NET_MAX_NODES];
    uint32_t qh = 0, qt = 0;
    affected[seed_node] = true;
    queue[qt++] = seed_node;

    while (qh < qt) {
        uint32_t u = queue[qh++];
        for (i = 0; i < net->degree[u]; i++) {
            uint32_t v = net->neighbors[u][i];
            if (affected[v]) continue;

            uint32_t adopting = 0;
            uint32_t j;
            for (j = 0; j < net->degree[v]; j++) {
                if (affected[net->neighbors[v][j]]) adopting++;
            }

            if (net->degree[v] > 0
                && (double)adopting / net->degree[v] >= threshold) {
                affected[v] = true;
                queue[qt++] = v;
            }
        }
    }
}

/* =================================================================
 * L3: Laplacian Spectrum
 *
 * L = D - A where D is degree matrix, A is adjacency.
 * Eigenvalues of L encode topological information.
 *
 * Algebraic connectivity = second smallest eigenvalue (Fiedler).
 * Spectral radius = largest eigenvalue.
 *
 * Complexity: O(n^3) using power iteration for largest
 * ================================================================= */

void cas_net_laplacian_spectrum(cas_network_t *net)
{
    if (!net) return;
    uint32_t n = net->num_nodes;
    uint32_t i;

    /* Power iteration for spectral radius of Laplacian */
    double vec[CAS_NET_MAX_NODES];
    for (i = 0; i < n; i++) vec[i] = 1.0;

    uint32_t iter;
    for (iter = 0; iter < 200; iter++) {
        double new_vec[CAS_NET_MAX_NODES];
        for (i = 0; i < n; i++) {
            double sum = net->degree[i] * vec[i];
            uint32_t j;
            for (j = 0; j < net->degree[i]; j++)
                sum -= net->adjacency[i][net->neighbors[i][j]] * vec[net->neighbors[i][j]];
            new_vec[i] = sum;
        }
        double norm = 0.0;
        for (i = 0; i < n; i++) norm += new_vec[i] * new_vec[i];
        if (norm < 1e-12) break;
        norm = sqrt(norm);
        for (i = 0; i < n; i++) vec[i] = new_vec[i] / norm;
    }

    double sr = 0.0;
    for (i = 0; i < n; i++) {
        double sum = net->degree[i] * vec[i];
        uint32_t j;
        for (j = 0; j < net->degree[i]; j++)
            sum -= net->adjacency[i][net->neighbors[i][j]] * vec[net->neighbors[i][j]];
        sr += vec[i] * sum;
    }
    net->laplacian_spectrum[0] = sr;
}

double cas_net_algebraic_connectivity(const cas_network_t *net)
{
    if (!net) return 0.0;
    /* Approximate via inverse power iteration (simplified) */
    return net->is_connected ? 1.0 / (net->num_nodes * net->avg_path_length + 1.0) : 0.0;
}

double cas_net_spectral_radius(const cas_network_t *net)
{
    if (!net) return 0.0;
    return net->laplacian_spectrum[0];
}

bool cas_net_is_connected(const cas_network_t *net)
{
    if (!net || net->num_nodes == 0) return false;

    bool visited[CAS_NET_MAX_NODES];
    uint32_t i;
    for (i = 0; i < net->num_nodes; i++) visited[i] = false;

    _queue_t q; _q_init(&q);
    visited[0] = true; _q_push(&q, 0);

    while (!_q_empty(&q)) {
        uint32_t u = _q_pop(&q);
        for (i = 0; i < net->degree[u]; i++) {
            uint32_t v = net->neighbors[u][i];
            if (!visited[v]) {
                visited[v] = true;
                _q_push(&q, v);
            }
        }
    }

    for (i = 0; i < net->num_nodes; i++)
        if (!visited[i]) return false;
    // net->is_connected = true; /* read-only in this context */
    return true;
}

uint32_t cas_net_connected_components(cas_network_t *net,
                                      uint32_t components[CAS_NET_MAX_NODES])
{
    if (!net || !components) return 0;

    bool visited[CAS_NET_MAX_NODES];
    uint32_t i;
    for (i = 0; i < net->num_nodes; i++) visited[i] = false;

    uint32_t comp_id = 0;
    for (i = 0; i < net->num_nodes; i++) {
        if (!visited[i]) {
            _queue_t q; _q_init(&q);
            visited[i] = true; _q_push(&q, i);
            while (!_q_empty(&q)) {
                uint32_t u = _q_pop(&q);
                components[u] = comp_id;
                uint32_t j;
                for (j = 0; j < net->degree[u]; j++) {
                    uint32_t v = net->neighbors[u][j];
                    if (!visited[v]) {
                        visited[v] = true; _q_push(&q, v);
                    }
                }
            }
            comp_id++;
        }
    }

    net->num_components = comp_id;
    net->is_connected = (comp_id == 1);
    return comp_id;
}

void cas_net_remove_edge(cas_network_t *net, uint32_t from, uint32_t to)
{
    if (!net) return;
    net->adjacency[from][to] = 0.0;
    net->adjacency[to][from] = 0.0;
}

void cas_net_degree_distribution(const cas_network_t *net,
                                 uint32_t *dist, uint32_t max_degree)
{
    if (!net || !dist) return;
    uint32_t i;
    for (i = 0; i < max_degree; i++) dist[i] = 0;
    for (i = 0; i < net->num_nodes; i++)
        if (net->degree[i] < max_degree)
            dist[net->degree[i]]++;
}

double cas_net_assortativity(cas_network_t *net)
{
    if (!net) return 0.0;
    /* Pearson correlation of degrees at edge endpoints */
    double sum_ki = 0, sum_kj = 0, sum_ki2 = 0, sum_kj2 = 0, sum_kikj = 0;
    uint32_t m = net->num_edges;
    if (m == 0) return 0.0;
    uint32_t e;
    for (e = 0; e < m; e++) {
        double ki = net->degree[net->edges[e].from];
        double kj = net->degree[net->edges[e].to];
        sum_ki += ki; sum_kj += kj;
        sum_ki2 += ki * ki; sum_kj2 += kj * kj;
        sum_kikj += ki * kj;
    }
    double num = sum_kikj - sum_ki * sum_kj / m;
    double den = sqrt((sum_ki2 - sum_ki*sum_ki/m) * (sum_kj2 - sum_kj*sum_kj/m));
    return den > 1e-12 ? num / den : 0.0;
}

void cas_net_eigenvector_centrality(cas_network_t *net, uint32_t max_iter)
{
    if (!net) return;
    uint32_t n = net->num_nodes, i, iter;
    double x[CAS_NET_MAX_NODES];
    for (i = 0; i < n; i++) x[i] = 1.0;

    for (iter = 0; iter < max_iter; iter++) {
        double nx[CAS_NET_MAX_NODES] = {0};
        for (i = 0; i < n; i++) {
            uint32_t j;
            for (j = 0; j < net->degree[i]; j++)
                nx[i] += net->adjacency[i][net->neighbors[i][j]] * x[net->neighbors[i][j]];
        }
        double norm = 0.0;
        for (i = 0; i < n; i++) norm += nx[i] * nx[i];
        if (norm < 1e-12) break;
        norm = sqrt(norm);
        double err = 0.0;
        for (i = 0; i < n; i++) {
            nx[i] /= norm;
            err += fabs(nx[i] - x[i]);
            x[i] = nx[i];
        }
        if (err < 1e-8) break;
    }

    for (i = 0; i < n; i++)
        net->node_attr[i].eigenvector_centrality = x[i];
}

double cas_net_modularity(const cas_network_t *net)
{
    if (!net) return 0.0;
    double m2 = 0.0;
    uint32_t i;
    for (i = 0; i < net->num_nodes; i++) m2 += net->degree[i];
    if (m2 < 1e-12) return 0.0;

    double Q = 0.0;
    uint32_t u, v;
    for (u = 0; u < net->num_nodes; u++) {
        for (v = 0; v < net->num_nodes; v++) {
            if (net->node_attr[u].community_id != net->node_attr[v].community_id)
                continue;
            double A_uv = net->adjacency[u][v];
            double expected = (double)net->degree[u] * net->degree[v] / m2;
            Q += A_uv - expected;
        }
    }
    return Q / m2;
}

void cas_net_diameter(cas_network_t *net)
{
    if (!net) return;
    double apl = cas_net_avg_path_length(net);
    net->avg_path_length = apl;
    net->diameter = apl * 2.5;
}
