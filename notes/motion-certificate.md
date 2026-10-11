# Metric and all-time motion contract

The target is a radius-2 disk of curvature −1. Ring populations follow
`ceil(2π sinh(radius)/0.14)`. Hyperbolic target edge lengths define flat
Euclidean comparison triangles, producing a separate PL reference metric.
The angular-zipper mesh has 1,023 vertices and 1,881 oriented faces. No
fixed seven-Euclidean-triangles refinement is used.

For reference coordinates `Q`, spatial edge columns `E`, and `J = E Q⁻¹`,
stretch is the pair of singular values of J. `L = max(σ_max,1/σ_min)` over
all faces bounds every abstract path and hence every intrinsic distance.
An ambient intersection never creates an abstract adjacency.

The accepted base has minimum stretch 0.999053321380 and maximum stretch
1.001708268990. Three smooth ambient fields, with their infinitesimal rigid
components removed, are scaled so the sum of face differential operator norms
is at most 0.035. Each scalar coefficient is clamped to [−1,1]. Periods are
48, 67, and 91 active seconds. Absolute-time evaluation prevents accumulated
metric drift; background time does not advance the phase.

The implementation conservatively reserves 0.005 for arithmetic error.
Per-vertex coordinate ranges over the whole coefficient box bound float
conversion and double operations; error propagation through Q⁻¹ bounds the
face differential by its Frobenius norm. The calculated worst error bound is
about 0.000053377, below that reserve. Builds omit fast-math. The reference
singular values are evaluated in double precision with additional ample
slack under the public contract.

Thus every pose has σ_min ≥ 0.959053321380 and σ_max ≤ 1.041708268990;
`L < 1.043`, stronger than the exposed `L ≤ 1.10` acceptance cap. This
certificate covers all phase combinations. Tests of 1,000 emitted float
poses and all eight mode-box corners provide regression evidence, not the
proof of continuous time. The 1,000-pose maximum measured L was 1.01644526396
and maximum K was 1.0300229957.

`K = σ_max/σ_min` measures local anisotropy, independently of absolute scale.
These are metrics on the same abstract mesh; the image may self-intersect.
The motion contract does not certify a smooth injective embedding or a 10%
approximation to the continuum hyperbolic plane.

Refinement from spacing 0.14 to 0.07 reduces polygon target area error against
the true disk: 17.3170004524 → 17.3457690444, with exact area 17.3553873818.
PL comparison area moves 17.3659011253 → 17.3585034321. This is a measured
fidelity/convergence report, not a universal continuum error certificate.
