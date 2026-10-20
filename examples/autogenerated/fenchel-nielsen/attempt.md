# Fenchel–Nielsen construction

Purpose: construct a closed marked genus-2 hyperbolic surface from two pants,
exposing three actual cuff lengths and three signed arclength twists.

Values and domains: lengths in (0,∞), finite real twists, determinant-one
real matrices up to sign, spacelike axis normals and future unit timelike
hexagon vertices. Each diagnostic has availability and numerical-qualification
flags; invalid or ill-conditioned data cannot become passing evidence.

Inputs: `MostowPantsParameters { lengths[3], twists[3] }`.
Outputs: cuff/seam/transverse lengths and traces, explicit cuff and crossing
holonomies, six real right-hexagon vertices, topology, curvature, area and
numerical residuals. `mostow_pants_hexagon_point` is a partial pure operation
from diagnostic/side/fraction to a Poincaré disk point.

Effects: initialization writes caller-owned state; evaluation and sampling
perform no I/O, allocation or platform operations. Test I/O is limited to
deterministic assertions and an error report.

Invariants: hyperbolic cuff traces, three matching cuff pairs, two pants
relations, a genus-2 surface relation, right angles, prescribed half-cuff and
seam lengths, χ = −2, curvature −1 and area 4π. Marking is preserved under
continuous parameter changes; full twists are not reduced modulo length.

Implementation language: the user explicitly selected the existing
compositional ICKY C implementation as authoritative. No speculative Idriç
program or generated-C replacement is introduced. Qualified Idriç execution
is not claimed. Host and Android builds must use the established ICK route.

Independent checks: the published right-hexagon identity is pinned by its
2011 journal version and lemma. A separate ℚ(√3) fixture, scalar transverse
trace identity, direct hyperboloid geometry and disk arclength formula test
the implementation. See `notes/fenchel-nielsen-construction.md` for the
construction and its proof boundaries.

Host execution: the parent-provided `mostow-mechanical-kernel` Flexible Pipes
pipeline with `KERNEL=pants` executed the maintained Kitchen task through pinned
Ike and ICK on 20 October 2026. Run token `20261020T040243.837387Z` passed the
algebraic, geometric, domain and 729 grid controls. The maximum reported
absolute matrix/geometric residual was 2.7637270250124857e−10, compared with
the declared 10⁻⁸ threshold. This is sampled binary64 evidence, not an interval
certificate. The compiler executable SHA256 was
`45805e3295f2709737fcc87985a8a2341aa8663ff8e4e398d2a9d78dc1283e10`.

The exact source SHA256 was
`d5460c6529a5a3f41762a0f5f29c791405ad590a0b18021588dfb61e41a4943c`;
the test SHA256 was
`658a5c801c4b6dececcf687be4ba9d5a0bdf7713861badf6f3d2314817bf219e`.
`test-mutants.grease` separately compiled four changed implementations and
required the intended unchanged-test assertion for each: ignoring twists,
allowing a zero cuff, changing arclength interpolation and dropping one pant's
area all failed at their intended witnesses. Its first invocation exposed
Grease's explicit raw-string requirement for backslashes; adding `r` prefixes
to the sed expressions corrected the parse error before executing mutations.

Independent AI-CI acceptance, integrated-renderer qualification, Android build
and physical-device results remain separate obligations of the parent task.
No result from those stages is inferred from these host tests.
