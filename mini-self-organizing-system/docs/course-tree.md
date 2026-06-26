# Course Tree — Self-Organizing Systems

## Prerequisites (What this module depends on)

### From mini-general-system-theory
- **mini-bertalanffy-open-system**: Open systems, entropy budget, Prigogine thermodynamics
  - Used in: `ThermodynamicRegime`, entropy production, free energy computation
- **mini-ashby-homeostasis**: Requisite variety, feedback control
  - Used in: `OrganizationLevel`, cybernetic regulation concepts
- **mini-boulding-hierarchy-levels**: System hierarchy, emergence
  - Used in: OrganizationLevel enum, emergence detection

### From mini-complexity-foundations
- Basic graph theory: adjacency matrices, Floyd-Warshall
  - Used in: network topology functions (`sos_network_init`, clustering, modularity)
- Numerical analysis: ODE integration, eigenvalue computation
  - Used in: RK4, Lyapunov exponent, Turing dispersion relation

## Dependents (What depends on this module)

### Within cybernetics-foundation
- **mini-second-order-cybernetics**: Observer-in-the-loop self-organization
- **mini-autopoiesis-theory**: Self-producing systems as a form of self-organization
- **mini-complex-adaptive-system**: Multi-agent self-organization with adaptation

### Cross-module
- **mini-game-of-life** (circuit-complexity): Cellular automata as computational substrate
- **mini-pattern-formation** (applied-math): Reaction-diffusion systems in biology
- **mini-evolutionary-dynamics**: NK fitness landscapes in evolutionary biology

## Learning Path
1. Start with `sos_core.h` — understand the fundamental types and measures
2. Study `sos_cellular.c` — concrete example of self-organization (Game of Life)
3. Explore `sos_turing.c` — pattern formation from homogeneous initial conditions
4. Dive into `sos_criticality.c` — self-organized criticality and 1/f noise
5. Master `sos_synergetics.c` — Haken's comprehensive theory of self-organization
6. Apply with `sos_autocatalytic.c` — origins of life and evolutionary self-organization

