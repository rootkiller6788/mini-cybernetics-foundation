# Knowledge Graph — Self-Organizing Systems

## L1: Definitions
- OrganizationLevel (Ashby-Boulding hierarchy): 7 levels from static to self-aware
- ThermodynamicRegime: Equilibrium, near-equilibrium, far-equilibrium, critical, turbulent
- EmergenceType: None, weak (Bedau), strong, nomological (Campbell)
- CriticalityState: Subcritical, critical, supercritical, metastable
- OrderParameter (Haken): dξ/dt = αξ - βξ³ + F(t)
- CAClass (Wolfram): Class I-IV (fixed, periodic, chaotic, complex)
- CellState (Conway): Dead/Alive
- CABoundaryType / CANeighborhoodType
- TuringSystemType: Schnakenberg, Gierer-Meinhardt, Gray-Scott, Brusselator, FitzHugh-Nagumo
- MoleculeType: Food, Catalyst, Product, Intermediate
- Hypercycle (Eigen-Schuster): cyclic catalytic self-replication
- NKLandscape (Kauffman-Levin): N genes, K epistatic interactions
- ModeType: Stable, Marginal, Unstable
- LaserSystem (Haken): photon number n, inversion N, pump threshold

## L2: Core Concepts
- Self-organization: spontaneous increase in organization without external guidance
- Emergence: macro properties irreducible to micro components
- Self-organized criticality: system evolves to critical state without external tuning
- Autocatalysis: products catalyze their own production
- Pattern formation: Turing instability creates spatial structure from homogeneity
- Slaving principle: fast variables enslaved by slow order parameters near criticality
- Edge of chaos: maximal computational capability at phase transition (Langton 1990)
- Dissipative structures: ordered states maintained by energy dissipation (Prigogine)
- Positive feedback: autocatalytic amplification drives self-organization
- Bifurcation: qualitative change in dynamics as parameter crosses threshold

## L3: Mathematical Structures
- Reaction-diffusion PDEs: ∂u/∂t = f(u,v) + Du∇²u, ∂v/∂t = g(u,v) + Dv∇²v
- Cellular automaton state space: Σ^{Z^d} with local update rule
- NK fitness landscape: f: {0,1}^N → [0,1] with K-epistatic interactions
- Hypercycle ODEs: dx_i/dt = x_i(A_iQ_i - φ) + Σ_j k_ji x_j
- Landau free energy: F(ξ) = a/2·ξ² + b/4·ξ⁴
- Sandpile model: discrete height field with threshold toppling
- Catalytic network: directed bipartite graph (molecules, reactions)
- Master equation: dP(s)/dt = Σ_{s'} [W(s|s')P(s') - W(s'|s)P(s)]

## L4: Fundamental Laws/Theorems
- Turing Instability Condition (1952): Dv·fu + Du·gv > 2√(Du·Dv·det(J))
- Ashby's Principle (1947): self-organization = change to higher organization
- Haken's Slaving Principle (1977): adiabatic elimination of fast variables
- Eigen's Error Threshold (1971): Q_min = 1/σ_m
- Landau Phase Transition: symmetry breaking at a=0, ordered phase for a<0
- Bak-Tang-Wiesenfeld SOC (1987): P(s) ~ s^{-τ}, S(f) ~ f^{-α}
- Hordijk-Steel RAF Theorem (2004): maximal RAF set existence
- Prigogine's Minimum Entropy Production (1945): d_iS/dt ≥ 0, minimum at steady state

## L5: Algorithms/Methods
- Cellular automaton evolution: O(WH) per generation with neighbor counting
- Sandpile relaxation: iterative toppling to stable configuration
- RK4 ODE integration: classical 4th-order Runge-Kutta
- RAF set computation: iterative closure algorithm (Hordijk-Steel)
- Power spectrum DFT: O(N²) direct computation
- Floyd-Warshall APSP: O(N³) all-pairs shortest path
- NK hill-climbing: greedy 1-bit flip local search
- Bak-Sneppen evolution: find minimum, replace neighbors
- Turing pattern evolution: forward Euler / RK2 for reaction-diffusion
- Power-law MLE fitting: Clauset-Shalizi-Newman (2009) method
- Forest fire simulation: lightning strike + fire spread + regrowth
- Box-counting fractal dimension: log-log regression

## L6: Canonical Problems
- Conway's Game of Life (1970): B3/S23 rule, still lifes, oscillators, gliders
- Bak-Sneppen evolution model (1993): punctuated equilibrium, f_c ≈ 0.667
- BTW Sandpile (1987): power-law avalanche distribution
- Schnakenberg Turing pattern: spots, stripes, labyrinth formation
- Drossel-Schwabl Forest Fire: SOC in excitable media
- Eigen's Hypercycle: cooperative catalytic cycles
- Laser threshold (Haken 1970): phase transition in coherent light
- NK fitness landscape: complexity catastrophe at intermediate K

## L7: Applications
- Swarm robotics: self-organizing collective behavior
- Chemical morphogenesis: biological pattern formation (zebra stripes, leopard spots)
- Evolutionary dynamics: NK landscape models of adaptation
- Neural self-organization: Kohonen SOM, Hebbian learning
- Traffic self-organization: emergent flow patterns
- Earthquake modeling: SOC as explanation for Gutenberg-Richter law

## L8: Advanced Topics
- Self-organized criticality: 1/f noise, power laws, universality classes
- Synergetics order parameters: Haken's comprehensive theory
- Edge of chaos (Langton 1990): λ ≈ 0.5 as optimal computation region
- Non-equilibrium phase transitions: Hopf, pitchfork, saddle-node bifurcations
- Information-theoretic emergence: Shalizi's ε-machine, causal states
- Fractal geometry in self-organization: Mandelbrot, box-counting dimension

## L9: Research Frontiers
- Quantum self-organization: non-equilibrium quantum phase transitions
- Self-assembling molecular systems: DNA origami, programmable matter
- Machine learning of self-organization: discovering CA rules via optimization
- Thermodynamics of computation: Landauer's principle in self-organizing systems
- Maximum entropy production principle: climate, ecosystems, planetary dynamics
- Meta-self-organization: systems that learn to self-organize

