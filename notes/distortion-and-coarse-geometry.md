# Tangent distortion and the integer inclusion

`icky/mostow_distortion.c` owns these calculations. It does not depend on the
renderer or a sampled finite metric-space picture.

## What an ellipse measures

A positive-definite source metric G and target metric H specify the lengths
in two oriented tangent planes. The differential J is expressed in their
given coordinate bases. Let G = RᵀR and H = SᵀS be Cholesky factorizations with
positive diagonal. Then metric-orthonormal coordinates see M = SJR⁻¹.
Consequently the two semiaxis lengths of M applied to the unit circle are
σ₁ ≥ σ₂ > 0. The implementation computes K = σ₁/σ₂, unsigned area scaling
σ₁σ₂ and signed area scaling det(M). It does not infer a global
quasiconformal homeomorphism from one tangent differential.

Equal axes with positive orientation are conformal; equal axes with negative
orientation are anticonformal. Numerical equality uses relative tolerance
10⁻¹⁰ and the labels explicitly say “to tolerance.” Scaling changes area and
axis lengths but leaves K unchanged. A shear changes K while preserving area.
The sample function applies this same M, so the display cannot independently
invent ellipse eccentricity.

The algebra above supplies an independent derivation. The planar complex
derivative formulation agrees with Howard Masur, *Geometry of Teichmüller
space with the Teichmüller metric*, §2 (2010),
<https://math.uchicago.edu/~masur/chapter3.pdf>. It writes local dilatation as
(|f_z| + |f_zbar|)/(|f_z| − |f_zbar|), the ratio of the major and minor axes
for positive orientation. The source metric matters: coordinate anisotropy
alone does not imply geometric anisotropy.

### Numerical domain

All inputs must be finite. Each metric must be positive definite, have maximum
coefficient magnitude in [10⁻⁵⁰,10⁵⁰], and spectral condition number at most
10¹⁰. The nonzero differential has maximum coefficient magnitude in the same
interval; K must not exceed 10⁸. Combined conditioning must also yield a
roundoff estimate below 10⁻⁴. Unsupported cases return an explicit status.
The estimate 128ε √κ_metric K is an engineering warning, **not a rigorous
error bound**. It omits a certified libm and directed-rounding proof.
This is binary64 numerical evidence; no interval certificate is claimed.

The metric determinant uses a compensated two-product difference. Recovering
the smaller singular value through |det(M)|/σ₁ avoids subtraction of nearly
equal squared eigenvalues. Independent controls include rational rotations,
diagonal stretch, the golden-ratio spectrum of unit shear, determinant and
Frobenius-norm identities, and an off-diagonal metric basis change.

## A global quasi-isometry with a short proof

Give ℤ its ordinary distance |m−n| and ℝ its ordinary distance |x−y|.
The inclusion sends each integer to the same real number. For **every** pair
of integers m,n, the image distance is exactly |m−n|. Thus both distortion
inequalities hold with L = 1 and A = 0.

For every real x, choose the unique integer n with n ≤ x < n+1. One of n
and n+1 is within 1/2 of x: use n when x−n ≤ 1/2, and n+1 otherwise.
Therefore the image is 1/2-dense, giving B = 1/2. Half-integers have distance
exactly 1/2 from every nearest integer, so a smaller B fails. This proves the
statement about the infinite spaces without inferring it from a viewport.
The definition is the one already fixed in STAR M1 §3 and §7.4.

The executable nearest-integer witness only accepts finite binary64 x with
|x| ≤ 2⁵⁰. It chooses the lower integer at exact ties. The integer-distance
routine uses endpoints in [−2⁵⁰,2⁵⁰], so their difference fits int64 and is
exactly representable in binary64. These safe computation domains do not
limit the preceding global proof. A finite rendered segment is an illustration
of that proof, not its evidence source. No rooted-tree claim is introduced.

## Targeted mutations

The deterministic tests reject changing K to one (shear), dropping H
(doubled-metric control), treating negative orientation as conformal
(reflection), ignoring J in sampling (shear image coordinates), replacing
nearest integer by floor (three-quarter witnesses), lowering B below 1/2
(half-integers), or allowing singular differentials (rank-one control).
Their execution results must be recorded separately from this source note.
