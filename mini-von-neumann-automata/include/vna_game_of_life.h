#ifndef VNA_GAME_OF_LIFE_H
#define VNA_GAME_OF_LIFE_H

#include "vna_core.h"

/* Game of Life — canonical patterns and operations */

VnaPattern* vna_gol_pattern_block(void);
VnaPattern* vna_gol_pattern_beehive(void);
VnaPattern* vna_gol_pattern_loaf(void);
VnaPattern* vna_gol_pattern_boat(void);
VnaPattern* vna_gol_pattern_blinker(void);
VnaPattern* vna_gol_pattern_toad(void);
VnaPattern* vna_gol_pattern_glider(void);
VnaPattern* vna_gol_pattern_lwss(void);
VnaPattern* vna_gol_pattern_gosper_gun(void);

VnaTransitionRule* vna_gol_create_rule(void);
VnaNeighborhood* vna_gol_create_neighborhood(void);
VnaLattice* vna_gol_create_lattice(int width, int height);

int vna_gol_insert_block(VnaLattice* lattice, int x, int y);
int vna_gol_insert_glider(VnaLattice* lattice, int x, int y);
int vna_gol_insert_blinker(VnaLattice* lattice, int x, int y);
int vna_gol_insert_gosper_gun(VnaLattice* lattice, int x, int y);
int vna_gol_insert_beehive(VnaLattice* lattice, int x, int y);
int vna_gol_insert_toad(VnaLattice* lattice, int x, int y);
int vna_gol_insert_lwss(VnaLattice* lattice, int x, int y);

int64_t vna_gol_population(VnaLattice* lattice);
int vna_gol_count_gliders(VnaLattice* lattice, VnaNeighborhood* nb,
                           VnaTransitionRule* rule);
int vna_gol_local_classify(VnaLattice* lattice, int x, int y);

int vna_gol_lexicon_size(void);
const char* vna_gol_lexicon_name(int idx);
VnaPattern* vna_gol_lexicon_pattern(int idx);
const char* vna_gol_lexicon_category(int idx);

typedef struct GolEcosystemMetrics GolEcosystemMetrics;
GolEcosystemMetrics* vna_gol_ecosystem_analyze(VnaLattice* lattice,
                                                VnaNeighborhood* nb,
                                                VnaTransitionRule* rule);
void vna_gol_ecosystem_free(GolEcosystemMetrics* m);

void vna_gol_run_experiment(VnaLattice* lattice, VnaNeighborhood* nb,
                             VnaTransitionRule* rule, int steps,
                             int64_t* population_history,
                             double* entropy_history);

#endif /* VNA_GAME_OF_LIFE_H */