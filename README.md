# Mostow

Interactive mathematics from a living hyperbolic sheet and visible gluing
through surface flexibility, local/coarse distortion, and Mostow rigidity.

Eight linked encounters share inspectable semantic state: a radius-2 living sheet with
clean/ink rendering and explanatory normals, and boundary-map string art with
linked Klein/Poincaré views, and a figure-eight completeness encounter with
freely varied ideal shapes, edge/cusp residuals and a retained complete
reference, a genus-2 octagon quotient with finite cover and boundary navigation,
two-pants Fenchel–Nielsen deformation, metric distortion ellipses, the proved
integer-to-real quasi-isometry, and a navigable rigidity argument. See
[the figure-eight mathematical boundary](notes/figure-eight-completeness.md) and
[mechanical encounter controls and ownership](notes/mechanical-encounters.md).
Phone controls include selection, orbit/pinch,
pause/direct scrub, lighting, map parameters, save/reopen and PNG export with
a companion scene. See [interaction and replay](notes/interactive-encounters.md).
Physical-device acceptance remains pending; host rendering is not phone evidence.

The mathematical source is compositional ICKY C compiled directly by ICK,
with NDK assembly and platform linking. See [source boundary](notes/source-boundary.md),
[motion certificate](notes/motion-certificate.md), and
[current division qualification](qualification/division-20261009.md).

The Android shell adapts NativeActivity/EGL/GLES2 responsibilities from
`functorial-games/seifert` at `aea9528072bdf9f18dd4b2934e8708f242c81c8b`.
It adds actual continuous animation, normals, lighting, and orbit/zoom rather
than claiming Seifert already supplied those application features.

No wind simulation, runtime metric optimizer, ray-traced renderer, generic
scene framework, or second handwritten mathematical implementation is used.
The initial realization accepts extrinsic intersections; triangle adjacency
and intrinsic distances remain independent of those intersections.

The reading notes are:

- [Mostow rigidity](books/mostow-rigidity.md)
- [Hyperbolic crochet](books/hyperbolic-crochet.md)
- [Papadopoulos on quasiconformal geometry](books/papadopoulos-quasiconformal.md)

The full staged design is in [STAR M1](notes/STAR-M1.md). The fixed `m004`
construction now checks pinned oriented face pairings, edge/cusp topology,
primitive peripheral curves, derived gluing rows, completeness and ideal volume.
The octagon verifies χ = −2 and area 4π. Pants holonomy and transverse lengths
respond to all three lengths and twists; a full twist changes the marking.
Floating-point residuals are numerical checks, not rigorous certification or a
proof of the global rigidity theorem. See the per-encounter source notes.

## Build and evidence boundaries

Invoke scripts through a verified Grease runtime, with explicit absolute paths.
`tools/test-host.grease` takes the checkout path. `android/build-pair.grease`
takes checkout, NDK, canonical android-NDK substrate, SDK, build-tools, public
test keystore, and output directory; `MOSTOW_GREASE` identifies that runtime.
`MOSTOW_HOST_COMPILER`, `MOSTOW_ICK_ARMV7`, and `MOSTOW_ICK_ARM64` select the
qualified native and Android ICK drivers. The maintained producers compile
original `←` and `÷` source directly. Host mutation tests also use
`MOSTOW_GREASE`; recording uses the pinned `MOSTOW_KITCHEN` movie assembler.
It builds both maintained ABIs, verifies the established test certificate via
the canonical packager, and emits per-APK receipts. It does not install or
qualify either phone. The producer preflight additionally requires Mostow's
package/signer registration in merged ai-ci policy.

The package is `org.isomorphisms.mostow`, versionCode 4, versionName 0.4.
The certificate is the existing public development identity used by Seifert,
not a new locally generated key. Future updates must preserve package and
signer and use a nondecreasing versionCode.

`tools/run-fp-host.grease` takes explicit Mostow, Flexible Pipes, Kitchen, Ike
and independent AI-CI checkouts. It verifies immutable dependency revisions,
then runs the prepared Kitchen/Ike stage through FP. This executes retained
tests, all four kernels, constraint-removal mutations, semantic GLES changes,
exact framebuffer reopening and the separately owned AI-CI oracle. The exact
source/compiler/artifact identities and raw receipts are retained under
`build/host`. No host result establishes A1 or C67 physical acceptance.

## External geometry references

The [external geometry catalogue](docs/external-geometry-examples.md) describes
constructions, intrinsic navigation, topology, illustration and physical models.
[Upstream source revision pins](docs/external-geometry-source-pins.tsv) are links
and metadata only. This repository does not vendor those external engines or
claim their execution, redistribution review, or independent acceptance.
