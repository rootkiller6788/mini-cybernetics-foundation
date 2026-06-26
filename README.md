# Mini Cybernetics Foundation

A collection of **from-scratch, zero-dependency C implementations** of the foundational theories of cybernetics — from Wiener's original vision of control and communication through Ashby's law of requisite variety, von Neumann's self-reproducing automata, Maturana & Varela's autopoiesis, and the second-order cybernetics of von Foerster. Each module maps to courses at MIT, Stanford, Caltech, Cambridge, and SFI, translating timeless cybernetic principles into runnable C code.

## Sub-Modules

| Sub-Module | Topics | Key Courses |
|-----------|--------|-------------|
| [mini-ashby-homeostasis](mini-ashby-homeostasis/) | Essential variables, ultrastability, Conant-Ashby theorem, homeostat device, variety theory | MIT 6.241, Stanford CS229 |
| [mini-autopoiesis-theory](mini-autopoiesis-theory/) | Autocatalytic sets (RAF), reaction networks, stoichiometric dynamics, organizational closure, structural coupling | MIT 6.S198 |
| [mini-complex-adaptive-system](mini-complex-adaptive-system/) | CAS agents, NK fitness landscapes, emergence detection, evolutionary computation, interaction networks | MIT 6.883, Stanford MS&E 338, SFI Complexity Explorer |
| [mini-control-philosophy](mini-control-philosophy/) | Teleology and purpose, observer theory, regulator theory, hierarchical control, variety and entropy | MIT 6.241J, MIT 6.832 |
| [mini-second-order-cybernetics](mini-second-order-cybernetics/) | Observing systems, constructivist epistemology, recursive computation, eigenforms, conversation theory | MIT 6.241J, Stanford AA274, ETH 227-0216 |
| [mini-self-organizing-system](mini-self-organizing-system/) | Autocatalytic hypercycles, cellular automata, self-organized criticality, synergetics, Turing patterns | MIT 6.841, Stanford CS358 |
| [mini-von-neumann-automata](mini-von-neumann-automata/) | Self-replicating automata, cellular automata, fault-tolerant computing, Game of Life, rule classification | MIT 6.045, Caltech CS 151, Stanford CS254, Cambridge Part II |
| [mini-wiener-cybernetics](mini-wiener-cybernetics/) | Feedback control, Wiener filter, information theory, Wiener prediction, Wiener process, fire control | MIT 6.241J / 6.432, Stanford EE363, Caltech CDS140, Cambridge Control Theory |

## Design Philosophy

- **Zero external dependencies** — pure C (C99/C11), only `libc` and `libm`
- **Self-contained modules** — each directory has its own `Makefile`, `include/`, `src/`, `examples/`, `demos/`, `tests/`, `docs/`
- **Theory-to-code mapping** — every module includes `docs/` with nine-school curriculum alignment and knowledge-layer coverage
- **Foundational cybernetics** — covers the full intellectual lineage from Wiener (1948) through Ashby (1952–1956), von Neumann (1966), Maturana & Varela (1973–1980), von Foerster (1981), to modern complexity science

## Building

Each module is standalone. Navigate to a module directory and run:

```bash
cd mini-ashby-homeostasis
make all    # build everything
make test   # run tests
```

Requires **GCC** and **GNU Make**.

## Project Structure

```
mini-cybernetics-foundation/
├── mini-ashby-homeostasis/           # W. Ross Ashby — homeostasis, ultrastability, requisite variety
├── mini-autopoiesis-theory/          # Maturana & Varela — autopoiesis, organizational closure, structural coupling
├── mini-complex-adaptive-system/     # Holland, Kauffman — CAS agents, NK landscapes, emergence
├── mini-control-philosophy/          # Wiener, Rosenblueth, Ashby — teleology, observer, regulator, hierarchy
├── mini-second-order-cybernetics/    # von Foerster, Maturana — observing systems, constructivism, recursion
├── mini-self-organizing-system/      # Prigogine, Haken, Bak — synergetics, SOC, Turing patterns
├── mini-von-neumann-automata/        # von Neumann, Conway, Wolfram — self-replication, CA, fault-tolerance
└── mini-wiener-cybernetics/          # Norbert Wiener — feedback, filtering, information, prediction
```

## License

MIT
