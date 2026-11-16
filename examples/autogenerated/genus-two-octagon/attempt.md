# Regular genus-2 octagon

The existing ICKY C boundary is authoritative for this task; no Idriç runtime
is claimed. The user explicitly required extending that owner, so this is an
ICKY implementation rather than a speculative Idriç port.

## Values and types

`MostowOctagonPoint` is a point in the open Poincaré disk with curvature −1.
The kernel transports it to the positive unit hyperboloid with Lorentz form
`t² − x² − y²`. A `MostowOctagonIsometry` acts on column vectors. It is a
representation of an orientation-preserving isometry, not an arbitrary matrix
that the renderer may interpolate.

`MostowOctagon` owns eight vertices, side midpoints, outward normals, explicit
oriented pairing isometries and the pairing involution. Gram, determinant and
future-sheet residuals distinguish Lorentz isometries from the required
orientation-preserving ones. The STAR M1 boundary
word is `a b a⁻¹ b⁻¹ c d c⁻¹ d⁻¹`; indices pair as
`0↔2, 1↔3, 4↔6, 5↔7`. This is deliberately not the opposite-edge Bolza marking.

`mostow_octagon_diagnostics` computes equivalence classes, orientation-reversing
boundary endpoint matches, Lorentz residuals, the complete corner cycle,
hyperbolic angle, edge length and area. `mostow_octagon_move` is a bounded
transaction on a marked point, an accumulated cover isometry, and a word.
Failure leaves state unchanged. `side_point` and `neighbor` expose geometry to
the existing GLES renderer without giving the renderer mathematical ownership.

## Construction and independent checks

The center/vertex/side-midpoint right triangle has angles π/8, π/8, π/2.
Hyperbolic right-triangle identities give `cosh(inradius)=1+√2` and
`cosh(circumradius)=(1+√2)²`. Disk vertices consequently have radius `2⁻¹⸍⁴`.
This produces π/4 corner angles, rather than assigning angle readouts to a
Euclidean octagon. Its actual Lorentz tangent angles are checked numerically.

For a side, the Lorentz frame contains its midpoint, positive boundary
tangent and outward unit normal. The pairing sends the midpoint to its
partner and both tangent and outward normal to their negatives. It therefore
reverses boundary traversal while preserving surface orientation, and maps
source exterior to partner interior. The executable test compares all sampled
edge points to reversed arclength samples on the paired edge, checks intrinsic
distances through the independent Poincaré distance formula, crosses each
boundary and reverses the crossing.

Union-find on the explicit endpoint identifications gives V=1, E=4, F=1 and
χ=−2. The corner cycle returns the entire tangent frame, not merely its base
point. Eight π/4 angles make 2π at the sole quotient vertex. Poincaré's polygon
theorem supplies the global discreteness/fundamental-domain conclusion under
these side/cycle conditions; a finite tile drawing alone does not establish it.

Gauss--Bonnet gives `area = 6π − sum(interior angles) = 4π`. An independent
polar integration of the Poincaré area density `4r/(1−r²)²` checks this result.
The numerical integrator uses the side's circle equation for its radial bound,
and compares 1024 and 2048 Simpson intervals. Its Richardson difference is an
error estimate, not a rigorous enclosure. All residual thresholds are binary64
numerical acceptance; no interval or exact-algebraic certificate is claimed.

The displayed neighboring collection consists of exactly nine distinct
Lorentz actions: identity and the eight side-neighbors. It neither enumerates
the infinite cover nor calls a word tree a surface-group Cayley graph. The
navigation word performs only adjacent inverse cancellation. General equality
in the surface group is represented by the action, not inferred from that
partial reduction. A persisted lift is checked against reconstruction from
its word. Word capacity is 32, a move admits at most 32 crossings, disk input
stays at least 1e−10 from the ideal circle, and accumulated matrix coefficients
are bounded by 1e6; exceeding those implementation domains fails explicitly.

## Primary references and derivation boundaries

- STAR M1 §6.1–6.2 in this repository fixes the boundary marking and interaction.
- Anna Felikson, *Geometry III/IV*, §9.4, author-hosted lecture notes:
  https://www.maths.dur.ac.uk/users/anna.felikson/Geometry/Geometry24/notes.pdf
  explains polygon side-pairing construction and complete corner cycles.
- Steve Trettel, *Regular Polygons in the Poincaré Disk*, August 11, 2026:
  https://stevejtrettel.site/notes/2026/regular-polygons-disk-model/
  supplies the disk metric/geodesic-circle construction independently of this
  implementation's Lorentz frame calculation.
- Iordan Iordanov and Monique Teillaud, CGAL 5.0 user manual:
  https://doc.cgal.org/5.0/Periodic_4_hyperbolic_triangulation_2/index.html
  is an independently implemented opposite-pairing example. It is a comparison,
  not the oracle for this different marking. No CGAL runtime/source is imported.

## Verification state

On October 20, 2026 UTC the `mostow-mechanical-kernel` Flexible Pipes stage,
with `KERNEL=octagon`, executed the parent's maintained Kitchen/pinned-Ike/ICK
route. Direct original `←` and `÷` source compiled and the unchanged tests
passed. This is host semantic evidence; GLES, Android packaging, independent
AI-CI and physical-device claims remain separate.

Observed diagnostics: V=1, E=4, F=1, χ=−2; angle-defect area
12.5663706144, independently integrated area 12.5663706149, Simpson difference
estimate 4.98e−10; maximum pairing and Lorentz residuals 5.86e−14; corner-cycle
residual 3.64e−12. All eight crossing/reverse-crossing witnesses passed.

Exact executed identities (SHA256):

- ICK host driver: `45805e3295f2709737fcc87985a8a2341aa8663ff8e4e398d2a9d78dc1283e10`.
- Mathematical source: `2fe790d41ff616f96f0fa118eae0b1ffa4b0f4fff428f97342ee99c4e8db265b`.
- Test source: `f215a4fa280fb8831e8307e43756323bd93c6fa5d51641d5a9475c449e758387`.
- Test executable: `0ccb44d928ce5653e9f632cb42df4f49e2a1b875fbedfd3f1a1cbac3770f0dd9`.

The parent must bind the final source commit and rerun this route if any
material source or tool pin changes.

Targeted negative controls corrupt the pairing involution, replace one pairing
by identity, alter a vertex, forge the persisted lift, exceed word capacity,
inject an invalid letter, and move onto the ideal circle. A source mutation
which discards the diagnostics' mathematical constraints must be rejected by
these unchanged known-bad witnesses.

`test-mutants.grease` was executed through the verified Grease runtime and
the same ICK host compiler. Three source mutants were rejected by the unchanged
tests: removing the actual endpoint-pairing acceptance condition let all
identity maps masquerade as side pairings; removing orientation acceptance
accepted side-compatible reflections; and removing persisted-lift consistency
accepted a forged cover action. All three failed their intended named
assertions. The first Grease invocation exposed its strict string parser:
an ordinary quoted sed replacement containing `\&` was rejected as an
ambiguous backslash before compilation; the explicit raw string fixed that
shell-language error. No Bash fallback was used.
