# The fixed figure-eight triangulation and its geometric quantities

The semantic owner is `icky/mostow_m004.c`. Its exact combinatorial checks
and numerical shape calculations are separate. Neither a plotted tetrahedron
nor a small gluing residual substitutes for the combinatorial calculation.

## Source identity and independent calculations

The data source is the official SnapPy
[example_m004.tri](https://github.com/3-manifolds/SnapPy/blob/c3da53c2b3105489b4d71531c4f781c92b237753/src/snappy/tests/files/example_m004.tri),
Git blob `02207ee5393033bfb85c2fe9063a3b7ad3c1b7dd`, SHA256
`487f7e0557d10897c875c96f084a525c37494b9b1ccd730cc4a668654d1bd9b4`.
Only its mathematical face-pairing and peripheral-current data are used;
no SnapPy implementation is incorporated into the application. This work
does not change the separate draft that collects external implementation sources.

An isolated host installation of SnapPy 3.3.2 was actually executed on
2026-10-20 UTC. Its CPython 3.12 manylinux wheel SHA256 is
`a0e724f8141314c936b1b4162aed029a95de0e46898a659427ae2bb1b68815d0`.
`Manifold('m004')._get_tetrahedra_gluing_data()` reproduced the permutations
below; `gluing_equations()` reproduced all four existing equation rows;
`edge_valences()` returned `{6: 2}`; `cusp_info()` returned one torus cusp;
`homology()` returned `Z`; the decorated triangulation signature was
`cPcbbbiht_BaCB`. `ManifoldHP('m004').volume()` returned
2.029883212819307250042405108549040571883378615060599584034978214.
These are foreign host oracle calls, not an application Python dependency.
Their deterministic numerical results are retained in
`fixtures/m004-reference.tsv` and exercised by the C witness.

## Face pairings, edges, and the cusp

The tetrahedra are both positively oriented. Face f lies opposite vertex f.
Each four-digit permutation gives the images of vertices 0, 1, 2, 3 in the
other tetrahedron. Tetrahedron 0 has permutations 0132, 1230, 2310, 2103
on faces 0 through 3; tetrahedron 1 has 0132, 3201, 3012, 2103.

The verifier first checks that every permutation is a bijection, every
paired face has the inverse permutation, and every permutation is odd.
The last condition makes the gluing reverse the induced face orientations
when the tetrahedra carry the same orientation. It then computes, rather
than assumes, equivalence classes of vertices, edges, and directed edges.
A directed edge identified with its reversal is rejected.

Exactly two edge classes remain, each with six tetrahedral edge incidences:

| Edge class | Tetrahedron 0 | Tetrahedron 1 |
|---|---|---|
| First | 01, 13, 23 | 03, 12, 23 |
| Second | 02, 03, 12 | 01, 02, 13 |

The opposite edge pairs 01/23, 02/13, 03/12 carry z, z′, z″,
respectively. Counting these incidences reproduces the two exponent rows
(2,1,0,1,0,2) and (0,1,2,1,2,0). Thus the matrix is checked against
the face-pairing topology, rather than used to infer that topology.

Truncating each ideal tetrahedral vertex gives eight cusp triangles.
Their side pairings are induced by the same permutations. Equivalence
classes of the 24 corners give four cusp vertices; the 24 triangle sides
give twelve paired edges. All eight triangles belong to one connected
component, with Euler characteristic 4 − 12 + 8 = 0. The oriented
side gluings and circular corner neighborhoods make this a closed oriented
surface. Hence this one cusp link is a torus. The verifier also walks each
corner cycle and checks its length against its complete equivalence class.
This establishes the required manifold neighborhoods in this fixed ideal
triangulation; it is not a general recognition algorithm for arbitrary input.

## The peripheral curves carry the completeness equations

The pinned source records signed normal currents for the meridian and
longitude in each cusp triangle. A positive current enters a triangle
through its indicated side; a negative current leaves. The verifier checks
zero current through the nonexistent side opposite the truncated vertex,
zero total current at every triangle, and opposite currents across each
paired side. These checks establish actual closed integral cycles.

Closed loops alone need not form a peripheral basis. In the dual cusp
cellulation, a spanning tree in its eight-vertex, twelve-edge graph leaves
five integral cycle coordinates. The four cusp-vertex boundaries have
rank three. Adjoining the two peripheral cycles yields a 5 by 6 integer
matrix. The greatest common divisor of all its maximal minors is one.
Consequently the boundaries and the two curves generate the entire cycle
lattice. Since the already checked cusp is a torus, their homology classes
form a primitive integral basis. Doubling the meridian, or replacing the
longitude by the meridian, fails this test. All calculations here use small
integers with an explicit input bound; they do not use floating tolerances.

The completeness exponent rows are also derived from these currents.
For an oriented normal arc at vertex v, entering face f and leaving face g,
let w be the fourth vertex. Its turning multiplier is the parameter at
edge (v,w), with exponent +1 when (v,f,g,w) is an odd permutation and −1
when it is even. This convention follows the oriented triangular link and
the ordered z, z′, z″ edge parameters. Summing all arcs gives exactly
(1,0,0,0,−1,0) and (0,0,0,0,−2,2). Adding the meridian to the longitude
preserves a primitive basis but deliberately fails this named-row check.
The independent SnapPy call checks the same convention and row order.

## Branches, errors, and volume

The shape domain is Im(z) > 0, with finite coordinates and finite nonzero
edge parameters. All three parameters then have angles in (0,π). The
evaluator uses these positive-dihedral logarithmic branches and adds their
angles without reduction modulo 2π. Edge targets are 2π; the peripheral
targets are zero. The diagnostic exposes signed logarithmic size, signed
angle error, their norm, the stated 1e−10 tolerance, and a floating-roundoff
estimate. Invalid, nonfinite, orientation-reversed, or numerically collapsed
parameters yield unavailable diagnostics. Branch continuation through
degenerate shapes or arbitrary Dehn filling is outside this fixed encounter.

For a positive ideal tetrahedron the angles are α = arg(z),
β = arg(1/(1−z)), γ = π − α − β, and the volume is
Λ(α) + Λ(β) + Λ(γ), where Λ(θ) = −∫₀ᶿ log(2 sin(t)) dt.
The source is Milnor's discussion in
[Thurston, Chapter 7](https://library.slmath.org/nonmsri/gt3m/PDF/7.pdf).
Reducing by Λ(π−θ) = −Λ(θ) puts the argument in [0,π/2].
The product expansion of sin gives
Λ(θ) = θ(1−log(2θ)) + Σₙ≥₁ ζ(2n)θ(θ/π)²ⁿ ÷ (n(2n+1)).
The implementation uses 24 terms. Its geometric truncation bound uses
ζ(2n) ≤ ζ(2); replacing ζ(2n) by 1 after n=16 has a separate bound
from ζ(2n)−1 ≤ 3 × 4⁻ⁿ. Both analytic remainder contributions are
returned as `series_tail_bound`.

The bound on the mathematical series does not bound binary64 rounding,
angle cancellation, or system `log`/`atan2` errors. The separate roundoff
field is an estimate, not an interval enclosure. `rigorous_certificate`
is always false. No complete hyperbolic structure is certified rigorously
by these floating-point calculations. SnapPy high precision is an
independent numerical oracle, also not an interval proof in this run.

Independent Bloch–Wigner evaluations with mpmath 1.3.0 at 70 digits used
Im(Li₂(z)) + arg(1−z) log|z|, rather than this Lobachevsky series.
They give 0.97029202838851009873698930154523175829 at z=0.7+1.2i.
The shape i has volume Catalan's constant, approximately
0.91596559417721901505. These and the regular m004 volume test the formula
at distinct shapes. The regular tetrahedron has volume approximately
1.01494160640965362502, and m004 consists of two such tetrahedra.

## Why the regular tetrahedron is the maximizer

This is an analytic statement on α,β,γ > 0 with α+β+γ=π, not a
consequence of the finite samples. On this angle triangle, the negative
of the restricted Hessian has diagonal entries cot(α)+cot(γ) and
cot(β)+cot(γ), and off-diagonal entry cot(γ). Its first diagonal entry
is sin(β)/(sin(α)sin(γ)) > 0 and its determinant is one. The volume
is therefore strictly concave. Its unique stationary point is
α=β=γ=π/3, because the two first derivatives require all three sines
to agree. It is the unique global maximizer. A deterministic angle-grid
test checks that the implementation respects this proved inequality; the
grid is not its proof.

## Boundary examples

Homogeneous complex coordinates keep infinity explicit. The helper maps
the four normalized vertices by the real-linear map
w ↦ (1+k)Re(w) + i(1−k)Im(w), |k|<1, followed optionally by the genuine
Möbius map w ↦ (w−0.4)/(0.4w+1). Its complex matrix satisfies M* M = 1.16 I,
so it induces a sphere rotation. In real sphere coordinates this is
x′ = (21x+20z)/29, y′ = y, z′ = (−20x+21z)/29. An independent orthogonal
matrix witness checks the actual output vertices against these identities.
The helper recomputes the ordered cross-ratio from
complex 2 by 2 determinants and maps those same transformed points onto
the boundary sphere. The renderer therefore receives actual transformed
vertices and a separately recomputed shape.

At k=0 the Möbius map changes the boundary positions while preserving
the cross-ratio and ideal volume. For k≠0, normalization changes the
shape to Re(z) + i(1−k)Im(z)/(1+k), an explicit nonconformal boundary
example. No claim is made that this new tetrahedron supplies a complete
deformed figure-eight manifold. The existing cusp constraints decide that
separate question.

The first nontrivial Möbius example used (2w+1)/(w+1). Real GLES review
found that its sphere compression placed the regular tetrahedron almost
edge-on at the default camera. The rotation above preserves spherical
separations and fixes that readability issue while retaining a genuine
Möbius transformation. This is visual feedback about the displayed
comparison, not evidence against cross-ratio invariance.

The unchanged witness rejects six deliberately altered implementations:
missing reciprocal-face check, nonprimitive peripheral basis accepted,
peripheral rows disconnected from normal currents, volume divided by two,
anisotropic shear replaced by a conformal transformation, and cusp equations
ignored. These are implementation mutations, not only malformed inputs.
Host execution and rendering remain separate from physical-phone acceptance.
