# Mostow

Interactive mathematics from a living hyperbolic sheet and visible gluing
through surface flexibility, local/coarse distortion, and Mostow rigidity.

The first implemented slice is a radius-2 ruffled patch, built for the MIRO A1
and paired MIRO C67. It supports one-finger orbit, two-finger pinch, pause,
intrinsic ring shading, and explicit current L/K diagnostics. Physical-device
acceptance remains pending; host rendering is not phone evidence.

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

The full staged design is in [STAR M1](notes/STAR-M1.md). Its later gluing, FN, coarse-map, tetrahedral,
and boundary encounters are planned, not implemented by this first slice.

## Build and evidence boundaries

Invoke scripts through a verified Grease runtime, with explicit absolute paths.
`tools/test-host.grease` takes the checkout path. `android/build-pair.grease`
takes checkout, NDK, canonical android-NDK substrate, SDK, build-tools, public
test keystore, and output directory; `MOSTOW_GREASE` identifies that runtime.
`MOSTOW_HOST_COMPILER`, `MOSTOW_ICK_ARMV7`, and `MOSTOW_ICK_ARM64` select the
qualified native and Android ICK drivers. The maintained producers compile
original `←` and `÷` source directly.
It builds both maintained ABIs, verifies the established test certificate via
the canonical packager, and emits per-APK receipts. It does not install or
qualify either phone. The producer preflight additionally requires Mostow's
package/signer registration in merged ai-ci policy.

The package is `org.isomorphisms.mostow`, versionCode 1, versionName 0.1.
The certificate is the existing public development identity used by Seifert,
not a new locally generated key. Future updates must preserve package and
signer and use a nondecreasing versionCode.
