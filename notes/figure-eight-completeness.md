# A constrained and unconstrained figure-eight encounter

This implements a third encounter, separate from the living sheet and
hyperbolic string art. The object is SnapPy's fixed two-tetrahedron `m004`
model of the figure-eight complement. The authoritative convention and
integer matrix come from the
[official SnapPy gluing-equation documentation](https://snappy.computop.org/manifold.html#snappy.Manifold.gluing_equations_pgl).
The [topology, peripheral curves, volume and boundary source note](m004-topology-volume.md)
records the new exact face-pairing checks, independent host oracles and
explicit numerical limitations.

For each ideal shape z in the upper half-plane, use edge parameters z,
z′ = 1/(1−z), and z″ = (z−1)/z. The matrix rows, ordered by tetrahedron,
are two edge equations followed by meridian and longitude equations:

| Equation | z₀ | z₀′ | z₀″ | z₁ | z₁′ | z₁″ | Logarithmic target |
|---|---:|---:|---:|---:|---:|---:|---|
| Edge 0 | 2 | 1 | 0 | 1 | 0 | 2 | 2πi |
| Edge 1 | 0 | 1 | 2 | 1 | 2 | 0 | 2πi |
| Meridian | 1 | 0 | 0 | 0 | −1 | 0 | 0 |
| Longitude | 0 | 0 | 0 | 0 | −2 | 2 | 0 |

The numerical evaluator retains logarithmic size and **unwrapped edge angle**;
checking only exponentiated products would lose the required 2π winding.
Unavailable or nonpositive-orientation data cannot produce a zero residual.
Acceptance at 1e−10 is a numerical check, not interval certification.

## Why this complete shape is fixed

The meridian gives z₀ = 1/(1−z₁). Substitution into Edge 0 reduces its product
to −(1−z₁)/z₁² = 1. Thus z₁²−z₁+1 = 0. The upper-half-plane root is
(1+i√3)/2; substitution gives the same z₀. All four logarithmic rows hold
at that pair. This is an algebraic calculation for this specified model,
not a proof of the general rigidity theorem or arbitrary triangulations.

The edge-only preset uses z₀=i and the positive root of
z₁²−(1+i)z₁+1=0. Both edge residuals are tiny, while the cusp residuals are
nonzero. It is an explicit positive-shape example showing why gluing edges
alone does not impose cusp completeness.

## Visible interaction

Cycle the encounter control through Sheet, Strings, and M004. MORE exposes
four trial shape coordinates. The next page offers COMPLETE/FREE and EDGE
ONLY; REGULAR restores the reference pair. The two tetrahedra on the left
show freely varied shapes or the retained complete pair. The right pair is
the complete reference. A failed trial remains visible in the readouts even
when completeness is required: it is rejected rather than relabelled solved.

The normalized vertices infinity, 0, 1 and z are mapped to the Klein boundary
sphere and drawn with straight ideal edges. The view previews ideal shapes;
the independent semantic verifier checks the pinned face-pairing topology,
its torus cusp, primitive peripheral curves and all four equation rows.
It does not represent a deforming complete 3-manifold or prove rigidity from
pixels. General flexible-surface moduli and a different-geometry control
remain separate future parts of the broader feature.

## Ownership and evidence

`icky/mostow_m004.c` owns the model and constraints. Presentation consumes
that result. Explicit binary64 model arithmetic and binary32 projection
preserve the existing owner boundaries. The Idriç attempt is retained with
its unavailable compiler-runtime boundary; this is not Android Idriç code.

The original scene schema 3 persisted the independent trial pair, selected coordinate and
complete/free setting, without repurposing string-art parameters. Older
schemas are rejected explicitly. Existing source/fixture binding, exact
reopening, style-state independence, and legacy geometry tests remain.
The known-bad consumer that ignores cusp conditions is rejected by the
unchanged mathematical witness. Real host GLES checks require visible
geometric changes and exact framebuffer reopening for this encounter.
Host images do not establish physical-phone installation or interaction.
