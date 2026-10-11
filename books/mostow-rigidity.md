# Mostow rigidity — reading list

Short and geometric sources worth keeping with this project.

## Likely 2010s candidates

### José Andrés Rodríguez Migueles — *Mostow's Rigidity Theorem* (2015)

Master's project, Université de Rennes 1, supervised by Juan Souto.

A substantial but readable exposition comparing several routes to the theorem, including the Gromov–Thurston proof. This is a plausible candidate for the paper/notes read in the 2010s.

- Author page: https://sites.google.com/a/ciencias.unam.mx/joseandres-rodriguezmigueles/research
- Search/title: **Mostow's Rigidity Theorem**, José Andrés Rodríguez Migueles, 2015.

### Hans J. Munkholm — *Simplices of maximal volume in hyperbolic space, Gromov's norm, and Gromov's proof of Mostow's rigidity theorem (following Thurston)*

Especially relevant to the visualization in this repository: it centers the proof on maximal-volume regular ideal simplices and explains how that feeds into Gromov's proof of Mostow rigidity.

- PDF: https://bpb-us-w2.wpmucdn.com/sites.uwm.edu/dist/0/158/files/2016/10/Munkholm.Mostow-Rigidity-1c8re8n.pdf

This is another strong candidate for the old short paper, particularly if the remembered explanation involved ideal simplices and volume.

## Classical geometric sources

### Stephen Agard — *A geometric proof of Mostow's rigidity theorem for groups of divergence type*

*Acta Mathematica* **151** (1983), 231–252.

A deliberately geometric treatment of the boundary-map argument.

- DOI: https://doi.org/10.1007/BF02393208
- PDF mirror: https://archive.ymsc.tsinghua.edu.cn/pacm_download/117/6342-11511_2006_Article_BF02393208.pdf

### William P. Thurston — *The Geometry and Topology of Three-Manifolds*

Two especially relevant sections:

- §5.9 — **A Proof of Mostow's Theorem**
- §6.3 — **Gromov's proof of Mostow's Theorem**

The §6.3 route is the one most directly connected to regular ideal simplices, hyperbolic volume, and simplicial/Gromov norm.

- Widely available as Thurston's Princeton lecture notes.
- One public copy: https://homepages.math.uic.edu/~kauffman/Thurston.pdf

### Peter Scott — *The Geometries of 3-Manifolds* (1983)

**Unbuilt exhibit:** [dimension-two flexibility versus dimension-three rigidity #3](https://github.com/isomorphismes/mostow/issues/3). Do not confuse with [existing hyperbolic sheet qualification #2](https://github.com/isomorphismes/mostow/issues/2).

**Peter Scott**, “The Geometries of 3-Manifolds,” *Bulletin of the London Mathematical Society* **15** (1983), no. 5, 401–487. [DOI](https://doi.org/10.1112/blms/15.5.401).

Scott's §§3–6 place hyperbolic 3-space alongside the other seven Thurston geometries and explain their relationship to Seifert-fibered and decomposed 3-manifolds. **Mostow rigidity is about the hyperbolic setting under its hypotheses; it does not say that all 3-manifolds are hyperbolic or that the other seven model geometries have the same rigidity.** Useful context for deciding exactly which geometry a proposed deformation or visual experiment belongs to.

## Modern exposition

### Richard Evan Schwartz — *Mostow Rigidity Made Easier* (2025/2026)

A self-contained, analytically light proof aimed at graduate students. Too recent to be the paper remembered from the 2010s, but useful for this project.

- Author PDF: https://www.math.brown.edu/reschwar/MathNotes/mostow.pdf
- arXiv: https://arxiv.org/abs/2512.09774

## Original theorem

### G. D. Mostow — *Quasi-conformal mappings in n-space and the rigidity of hyperbolic space forms* (1968)

The original rigidity theorem for finite-volume hyperbolic manifolds in dimension at least 3.

- *Publications Mathématiques de l'IHÉS* **34** (1968).

## Visual idea to keep in mind

For a closed hyperbolic (n)-manifold, (n \ge 3), the topology determines the hyperbolic geometry up to isometry.

The Gromov–Thurston route gives a particularly visual mechanism:

1. An equivalence of the manifolds lifts to a quasi-isometry of hyperbolic space.
2. It induces a homeomorphism of the sphere at infinity.
3. Regular ideal simplices maximize hyperbolic volume.
4. Simplicial volume/topological degree prevents a genuine deformation from lowering those simplex volumes on average.
5. The boundary map is therefore forced to preserve the regular ideal-simplex structure.
6. Preserving that structure forces the boundary map to be Möbius.
7. A Möbius boundary map extends to an isometry of hyperbolic space.

For (mathbb H^3), putting one ideal vertex at infinity turns the other three vertices of a regular ideal tetrahedron into an equilateral Euclidean triangle in the boundary plane. That is a useful concrete visual model for the rigidity mechanism.

## Identification note

The remembered source was read in the **2010s**, so Schwartz is ruled out. The best current candidates are:

1. Rodríguez Migueles (2015), if it was a graduate-level expository set of notes.
2. Munkholm, if the memorable part was the maximal-volume / regular-ideal-simplex argument.
3. Agard, if it was an older paper found online during the 2010s.
4. Thurston's notes, if the remembered "paper" was actually a short section of the Princeton notes.
