# Course Alignment — Self-Organizing Systems

## Nine-School Curriculum Mapping

### MIT
- **6.841 Advanced Complexity Theory**: Cellular automata as computational models, Wolfram classes
- **6.832 Underactuated Robotics**: Self-organizing swarm behavior, emergence
- **IDS.131/STS.081**: Self-organization in social systems
- Our coverage: `sos_cellular.c` (CA, Game of Life, Wolfram classification), `sos_core.c` (emergence detection)

### Stanford
- **CS358 Circuit Complexity**: Boolean circuits, threshold circuits → cellular automata as parallel computers
- **CS228 Probabilistic Graphical Models**: Markov blankets, information flow → transfer entropy
- **AA274 Multi-agent Systems**: Self-organizing swarms
- Our coverage: `sos_core.c` (transfer entropy), `sos_cellular.c` (CA classes)

### Berkeley
- **EE222 Nonlinear Systems**: Bifurcation theory, phase transitions, stability analysis
- **CS294 Deep Reinforcement Learning**: Self-organizing representations
- Our coverage: `sos_synergetics.c` (Landau theory, bifurcation), `sos_core.c` (eigenvalue analysis)

### CMU
- **24-654 Systems Thinking**: Hierarchy theory, emergence, self-organization principles
- **15-455 Undergraduate Complexity Theory**: Computability at the edge of chaos
- Our coverage: `sos_core.h` (OrganizationLevel hierarchy), `sos_cellular.c` (Langton lambda)

### Princeton
- **ELE530 Estimation and Detection**: Information theory, entropy, mutual information
- **MAE546 Optimal Control**: Dynamic programming on fitness landscapes
- Our coverage: `sos_core.c` (entropy, MI, transfer entropy), `sos_autocatalytic.c` (NK landscape)

### Caltech
- **CDS140 Nonlinear Dynamics**: Bifurcations, chaos, Lyapunov exponents
- **CS151 Complexity Theory**: Self-organization as computation
- Our coverage: `sos_core.c` (Lyapunov exponent), `sos_synergetics.c` (Landau theory)

### Cambridge
- **4F3 Nonlinear and Predictive Control**: Slaving principle, model reduction
- **Part III Complexity Theory**: Phase transitions in computation
- Our coverage: `sos_synergetics.c` (slaving principle), `sos_criticality.c` (SOC phase transitions)

### Oxford
- **C20 Adaptive Control**: Self-tuning, self-organizing controllers
- **B4 Predictive Control**: Model-based self-organization
- Our coverage: `sos_autocatalytic.c` (hypercycle adaptation), `sos_core.c` (history tracking)

### ETH
- **227-0216 System Identification**: Power spectrum, correlation analysis
- **227-0220 Model Reduction**: Slaving principle, order parameter reduction
- Our coverage: `sos_core.c` (power spectrum, autocorrelation), `sos_synergetics.c` (adiabatic elimination)

