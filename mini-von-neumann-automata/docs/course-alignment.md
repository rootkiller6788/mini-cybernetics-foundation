# Course Alignment — mini-von-neumann-automata

## Nine-School Mapping

### MIT
- **6.045 / 18.400 Automata, Computability, Complexity**
  - Topic: Cellular automata as models of computation
  - Coverage: L1-L4 (definitions, Turing completeness, rule universality)
  - Key files: `vna_core.h`, `vna_rule110.c`

### Stanford
- **CS254 Computational Complexity**
  - Topic: Rule 110 universality (Cook's theorem) as complexity result
  - Coverage: L4-L5 (Cook's proof components, cyclic tag system)
  - Key files: `vna_rule110.c`, `vna_formal.lean`

### Berkeley
- **CS172 Computability and Complexity**
  - Topic: CA as universal computers
  - Coverage: L1-L3 (mathematical CA structures)
  - Key files: `vna_core.c`, `vna_cellular.c`

### CMU
- **15-455 Undergraduate Complexity Theory**
  - Topic: CA and computational complexity
  - Coverage: L5 (algorithms for CA analysis)
  - Key files: `vna_rule.c`, `vna_cellular.c`

### Princeton
- **COS 522 Computational Complexity**
  - Topic: CA and cryptography (cryptographic applications)
  - Coverage: L7 (Rule 110 PRNG, documented CA crypto)
  - Key files: `vna_rule110.c` (PRNG)

### Caltech
- **CS 151 Complexity Theory**
  - Topic: Phase transitions in CA, damage spreading
  - Coverage: L8 (Lyapunov exponent, stochastic CA)
  - Key files: `vna_cellular.c` (damage spreading)

### Cambridge
- **Part II Complexity Theory**
  - Topic: Universality proofs (Cook's theorem)
  - Coverage: L4 (universality, formal statements)
  - Key files: `vna_formal.lean`, `vna_rule110.c`

### Oxford
- **Computational Complexity**
  - Topic: Quantum CA (research frontier)
  - Coverage: L9 (documented, future work)
  - Key files: `docs/knowledge-graph.md`

### ETH
- **263-4650 Advanced Complexity Theory**
  - Topic: Formal rule analysis, logic and computation
  - Coverage: L5 (de Bruijn diagrams, subset diagrams)
  - Key files: `vna_rule.c` (diagram construction)

## Textbook Alignment

| Textbook | Chapters | Coverage |
|----------|----------|----------|
| von Neumann (1966): Theory of Self-Reproducing Automata | All | L1-L4 (core reference) |
| Wolfram (2002): A New Kind of Science | Ch 2-6 | L1-L6 (CA classification) |
| Arora & Barak (2009): Computational Complexity | Ch 6 | L1-L4 (complexity) |
| Cook (2004): Universality in Elementary CA | All | L4 (Rule 110) |
| Langton (1984): Self-Reproduction in CA | All | L3-L4 (loops) |
| Nagel & Schreckenberg (1992): Traffic CA | All | L7 (traffic model) |
| von Neumann (1956): Probabilistic Logics | All | L4, L7 (error threshold) |
