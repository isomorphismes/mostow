# Two pants, three cuffs, three signed twists

`icky/mostow_pants.c` constructs an intrinsically hyperbolic closed genus-2
surface. Its input consists of three finite positive cuff lengths and three
finite real twists measured in hyperbolic arclength. A twist is retained as a
real number; it is not reduced modulo the cuff length. The marking, including
three selected transverse curves, is part of the encounter.

## The right hexagons and the pants

The source identity is Lemma 2.1 in Alessandrini, Liu, Papadopoulos, Su and Sun,
*On Fenchel–Nielsen coordinates on Teichmüller spaces of surfaces of infinite
type*, Annales Academiæ Scientiarum Fennicæ Mathematica **36** (2011), 621–659,
[DOI 10.5186/aasfm.2011.3637](https://doi.org/10.5186/aasfm.2011.3637),
[author-hosted published paper, pp.624–625](https://daniele-math.github.io/Publications/On%20Fenchel-Nielsen%20coordinates.pdf).
Only the finite right-hexagon identity is used here; no infinite-type theorem
is transferred to this closed surface.

For cuffs i,j and remaining cuff k, the seam distance obeys

cosh(seamᵢⱼ) = [cosh(ℓₖ/2) + cosh(ℓᵢ/2) cosh(ℓⱼ/2)] ÷
[sinh(ℓᵢ/2) sinh(ℓⱼ/2)].

Three positive half-cuff lengths determine a right-angled hexagon. Doubling
along its three seam sides gives a pair of pants with geodesic cuffs of the
requested full lengths. Two such pants are glued along matching cuffs. This
is a construction from hyperbolic pieces and boundary isometries, rather
than a claim that satisfying one matrix equation alone proves discreteness.

The implementation returns actual hexagon vertices in the hyperboloid
−t²+x²+y² = −1, t > 0. A geodesic cuff has a unit spacelike normal ν. For
two disjoint cuff axes with normals ν and μ, put s = ⟨ν,μ⟩. The common
perpendicular foot on the first axis is the future-pointing choice of
(μ − sν) ÷ √(s²−1). This computes all six corners. Distances, six right
angles and unit hyperboloid norms are checked independently of the seam
formula. `mostow_pants_hexagon_point` interpolates geodesic arclength in
the hyperboloid and projects to the Poincaré disk; the renderer consumes
these samples.

Four right hexagons form the closed surface. Each has area
(6−2)π − 6π/2 = π. The returned area is the sum of four angular defects
using the measured hexagon angles. Two pants each have Euler characteristic
−1; gluing their boundaries by circles adds zero, so χ = −2 and genus = 2.
The intrinsic model has curvature −1. A drawing of the four hexagons is a
cut presentation of this metric, not an isometric embedding into space.

## Explicit holonomy and the marking

Matrices act on the upper half-plane by real fractional transformations.
Write a = ℓ₀/2, b = ℓ₁/2 and d = seam₀₁. The first pants has

```
A = diag(exp(a), exp(−a))
B = [[cosh(b) − sinh(b) cosh(d),   sinh(b) sinh(d)],
     [−sinh(b) sinh(d),           cosh(b) + sinh(b) cosh(d)]]
C = (AB)⁻¹.
```

Their determinants are one; ABC = I and tr(AB) = −2 cosh(ℓ₂/2). The
boundary axes are disjoint. The seam formula, actual axis geometry, cuff
traces and hexagon identities jointly qualify the pants construction.

For each cuff Pᵢ, its positive-trace lift is
cosh(ℓᵢ/2) I + sinh(ℓᵢ/2) Xᵢ. The trace-zero matrix Xᵢ has Xᵢ² = I
and determinant −1; interpreted as an anti-Möbius map, it reflects in the
cuff axis. The translation by signed distance τ along that axis is
Eᵢ(τ) = cosh(τ/2) I + sinh(τ/2) Xᵢ.

Let G = E₀(τ₀) X₀. The second pants cuffs are
Qᵢ = G Pᵢ⁻¹ G⁻¹, in boundary order Q₀,Q₂,Q₁. The first gluing is a
tree edge. Set T₀ = I and Tᵢ = Eᵢ(τᵢ) Xᵢ G⁻¹ for i = 1,2. All Tᵢ
have determinant one, and the matching identity is
Tᵢ Qᵢ Tᵢ⁻¹ = Pᵢ⁻¹. Thus these are actual cuff isometries with the
specified twists.

The surface fundamental group is the graph of the two pants groups joined
along three boundary groups. Its theta graph has one tree edge and two
stable letters. The four generators A,B,T₁,T₂ satisfy the genus-2 relator

A⁻¹ T₂⁻¹ C⁻¹ T₂ T₁⁻¹ B⁻¹ T₁ = I, with C = (AB)⁻¹.

The code evaluates this eight-letter expression in A,B,T₁,T₂ and reports
the maximum entrywise residual from I. It also reports each cuff-matching
residual, determinant error and the first pants relation error.

The three selected transverse loops have holonomies T₁,
T₂ T₁⁻¹ and T₂⁻¹. Their geodesic lengths are 2 arcosh(|trace|/2).
Multiplying the reflection/translation expressions independently gives

|trace(transverseᵢⱼ)|/2 = cosh(seamᵢⱼ) cosh(τᵢ/2) cosh(τⱼ/2)
                           + sinh(τᵢ/2) sinh(τⱼ/2).

At zero twists each transverse length is twice the corresponding seam.
Changing each of the three twists changes the lengths of its two incident
transverse curves. A full twist around cuff 1 changes T₁ to B T₁. This is
a change of marking on the same unmarked gluing, not evidence that a full
twist gives a new unmarked isometry class. Different measured lengths of
a fixed marked cuff certify different marked hyperbolic structures.

## Numerical evidence and limits

The model uses binary64 arithmetic and reports a tolerance of 10⁻⁸ for
absolute matrix and hyperboloid residuals. This is a numerical acceptance
threshold, not an interval bound, a roundoff theorem or a proof over all
inputs. Overflow, loss of hyperbolicity or invalid projected points return
unavailable; finite but excessive residuals produce
`numerically_qualified = 0`. Parameters are never clamped to hide a failed
calculation. Very short cuffs, very long cuffs and large twists may become
ill conditioned in this particular matrix normalization.

The deterministic test includes an algebraic case with every half cuff
equal to arcosh(2). Here sinh(half cuff) = √3, every seam is arcosh(2),
and the first two cuff matrices have explicit entries in ℚ(√3). Their
cuff trace magnitudes are 4. A full twist changes the selected transverse
trace magnitude from 4 to 8. Further controls test all six parameters,
the independent scalar trace identity, disk arclength samples, invalid
inputs and 729 combinations in the displayed window: cuffs 0.75,1.5,3
and twists −2,0,2. That finite grid is not global certification.

The existing regular-octagon metric is a separate presentation with a
separate default. No equality between that metric and these pants defaults
is claimed without an actual coordinate conversion.
