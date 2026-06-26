# Knowledge Graph — Mini-Autopoiesis-Theory

## Nine-Level Knowledge Coverage

### L1: Definitions (Complete ✅)

| Entry | C Implementation | Status |
|-------|-----------------|--------|
| Autopoietic System | `ap_system_t` in `autopoiesis.h` | Complete |
| Component | `ap_component_t` with roles, concentration, flags | Complete |
| Production Relation | `ap_production_t` with producer/product/catalyst | Complete |
| Boundary | `ap_boundary_t` with topology, permeability | Complete |
| Reaction | `rn_reaction_t` in `reaction_network.h` | Complete |
| Molecular Species | `rn_species_t` with concentration, flags | Complete |
| RAF Set | `acs_reaction_t` / `acs_system_t` | Complete |
| Structural Coupling | `sc_coupling_t` / `sc_perturbation_t` | Complete |
| Chemoton | `chemoton_t` with 3 subsystems | Complete |
| Hypercycle | `hypercycle_t` with n-species | Complete |

### L2: Core Concepts (Complete ✅)

| Concept | Implementation | Status |
|---------|---------------|--------|
| Operational Closure | `ap_compute_organizational_closure()` | Complete |
| Self-Production | `ap_self_production_fraction()` | Complete |
| Structural Determinism | `sc_structural_determinism_index()` | Complete |
| Autocatalysis | `rn_is_autocatalytic_species()` | Complete |
| Organizational Invariance | `sc_organization_preserved()` | Complete |
| Consensual Domain | `sc_cross_coupling_measure()` | Complete |
| Mass-Action Kinetics | `rn_compute_reaction_rates()` | Complete |
| State Classification | `ap_classify_state()` | Complete |

### L3: Mathematical Structures (Complete ✅)

| Structure | Implementation | Status |
|-----------|---------------|--------|
| Production Adjacency | `ap_build_production_adjacency()` | Complete |
| Stoichiometric Matrix | `stoich_system_t` with dense + sparse | Complete |
| Flux Vector | `stoich_flux_t` | Complete |
| Transitive Closure | `ap_production_transitive_closure()` | Complete |
| Catalytic Graph | DFS cycle detection in `rn_find_autocatalytic_cycles()` | Complete |
| Jacobian Matrix | `stoich_compute_jacobian()` | Complete |
| CSR Sparse Matrix | `stoich_matrix_t` with CSR format | Complete |

### L4: Fundamental Laws (Complete ✅)

| Theorem/Law | Implementation | Status |
|-------------|---------------|--------|
| Maturana-Varela Autopoiesis Criteria | `ap_check_autopoiesis()`: 3 criteria | Complete |
| Organizational Closure Theorem | `oc_organizational_closure()`: fixed-point | Complete |
| Boundary Self-Production Theorem | `ap_verify_self_produced_boundary()` | Complete |
| Production Closure Theorem | `oc_production_closure()` | Complete |
| Catalytic Closure Theorem | `oc_catalytic_closure()` | Complete |
| Minimal Organizational Set | `oc_find_minimal_organizational_set()` | Complete |
| Hordijk-Steel RAF Theorem | `acs_find_maximal_raf()` | Complete |
| Eigen Hypercycle Stability | `hypercycle_find_fixed_point()`: n≤4 stable | Complete |
| Structural Coupling Dynamics | `sc_evolve_coupling()` | Complete |
| Gánti Chemoton Criteria | `chemoton_check_autopoiesis()`: 3 criteria | Complete |

### L5: Algorithms/Methods (Complete ✅)

| Algorithm | Implementation | Complexity |
|-----------|---------------|------------|
| Production Closure (fixed-point) | `oc_production_closure()` | O(R·C²) |
| RAF Detection (Hordijk-Steel) | `acs_find_maximal_raf()` | O(R²·M) |
| Minimal RAF (greedy) | `acs_find_minimal_raf()` | O(R²) |
| Irreducible RAF Enumeration | `acs_enumerate_irreducible_rafs()` | Randomized |
| RK4 Integration | `stoich_rk4_step()`, `chemoton_integrate_rk4()`, `hypercycle_integrate_rk4()` | O(n) per step |
| Adaptive RK45 (Dormand-Prince) | `stoich_integrate_adaptive()` | Adaptive |
| Backward Euler (stiff) | `stoich_backward_euler_step()` | O(n²) Newton |
| Steady-State Detection | `stoich_find_steady_state()`, `chemoton_find_steady_state()` | O(steps) |
| QR Eigenvalue Algorithm | `stoich_eigenvalues()` | O(n³) |
| Bifurcation Analysis | `chemoton_bifurcation_scan()` | O(points × steps) |
| Minimal Generator Search | `ap_find_minimal_generators()` | O(n²) |
| Dependency Depth (BFS) | `oc_dependency_depths()` | O(R·C) |

### L6: Canonical Problems (Complete ✅)

| Problem | Example | Status |
|---------|---------|--------|
| Minimal Autopoietic Cell | `examples/minimal_cell.c` | Complete |
| Chemoton Assembly | `examples/chemoton_demo.c` | Complete |
| Hypercycle Dynamics | `examples/hypercycle_demo.c` | Complete |
| Error Catastrophe | `hypercycle_error_threshold()` | Complete |
| Steady-State Analysis | `stoich_find_steady_state()` | Complete |
| RAF Emergence (Kauffman) | `acs_simulate_random_emergence()` | Complete |

### L7: Applications (Partial+ ✅ — 3 applications)

| Application | Implementation | Reference |
|-------------|---------------|-----------|
| Social Autopoiesis (Luhmann) | `examples/social_autopoiesis.c` | Luhmann (1984) |
| Niche Construction | `sc_niche_construction()` | Odling-Smee et al. (2003) |
| Organizational Coupling | `sc_cross_coupling_measure()` | Maturana & Varela (1987) |

### L8: Advanced Topics (Partial ✅ — 4 topics)

| Topic | Implementation | Reference |
|-------|---------------|-----------|
| Quasispecies Distribution | `hypercycle_compute_quasispecies()` | Eigen et al. (1988) |
| Spatial Hypercycle (RD) | `hypercycle_spatial_step()` | Boerlijst & Hogeweg (1991) |
| Random RAF Emergence | `acs_simulate_random_emergence()` | Kauffman (1995) |
| Adaptive Step-Size Control | `stoich_integrate_adaptive()` | Dormand & Prince (1980) |

### L9: Research Frontiers (Partial — documented)

| Topic | Documentation | Status |
|-------|--------------|--------|
| Multi-scale Autopoiesis | `docs/coverage-report.md` | Documented |
| Autopoiesis in AI/ALife | `docs/gap-report.md` | Documented |
| Chemical Organization Theory | `docs/course-tree.md` | Documented |
