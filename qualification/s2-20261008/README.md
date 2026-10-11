# S2 requalification — 8 October 2026

The retained APKs now pass the actual current-policy producer. The old M2
failure is historical and remains intact in `../m2/`.

Application source: `12aa1481bad53334e1506435f8bf8c71060960c4`.
Starting review head: `969a88648ea278301ea1359c4ad388ad55217b5b`.
This change updates only the host compiler selection and evidence; it changes
no application kernel, fixture, shader, Android code or visual choice.

## Executed checks

The existing Grease host runner executed the current tests with the explicitly
selected ICK scalar compiler. Both Grease runners now require
`MOSTOW_HOST_COMPILER`; unset configuration fails instead of choosing generic cc.
The existing Python lexical adapter remains visible compatibility debt and
is not an ICK frontend or a new Python mathematical implementation.

PASS: 1,023-vertex/1,881-face disk topology; minimum reference triangle angle
32.9655618061 degrees; hyperbolic-distance checks; intrinsic area refinement;
all eight mode-box corners; 1,000 emitted-float poses; unit normals; finite
inputs; capacity rejection; deterministic repeated samples; the deliberately
overstretched mutant and uncertified mode rejection; camera/pinch matrices.

The generator ran again to completion. `git diff --exit-code` confirmed an
unchanged fixture, SHA-256
`d539265d7d195b6771cc6ac04f6af84384e33f1fc78ec5bed71a22d1b7d89d58`.
The analytic all-time interval is `[0.95905332138, 1.04170826899]`.
Across the 1,000 float poses the largest observed length factor is
`1.01644526396`, anisotropy `1.0300229957`, and displacement after 20 seconds
`0.0231463270946`.

Those differential bounds control intrinsic path lengths on the abstract
triangulated PL surface, including paths crossing faces by summing segment
lengths. They do not certify the smooth hyperbolic disk globally, collision
avoidance, or an injective embedding into Euclidean 3-space. Extrinsic
intersections remain permitted and visible. The two-dimensional flexibility
illustration is not a proof of Mostow rigidity for complete finite-volume
hyperbolic manifolds in dimensions at least three.

The actual shared GLES2 renderer was compiled with the same declared ICK
driver and executed using Mesa llvmpipe `(LLVM 20.1.2, 256 bits)`, active time
zero, default yaw/pitch, 576×1152. The [new capture](host-preview.png) preserves
the existing faceted folds, ring overlay and diagnostic panel. Both ruffle
appearance and interaction feel remain user review items. This is not a phone
GPU or touch test.

## Binary provenance

ICK driver SHA-256:
`acde6cf158c527a415105cd692796c3c998a636f6887ab0d30262773e3611e91`;
cc1 `4fbf0c20391dc3e46d0840df795ebbc7e9b878d54858f6e0006f143501feb08c`.
Explicit separately supplied libatomic archive:
`ea5855f573bce90a1dad67d29929dd165e547f2300fc2b2e661851c913cdaed3`.
These hash-pinned scalar binaries are not a new clean-source ICK/runtime build
or qualification of its complex-number path.

Grease runtime SHA-256:
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`.
Its inherited binary entrypoint is named `ysh`; it is the already identified
Grease artifact, not substituted stock Oils. `ASAN_OPTIONS=detect_leaks=0`
is required on this traced host; no leak acceptance is claimed.

Host test executable: `6881ccd61c1670b64e157364e076e01ec9f9b49337248209924328d1d6866547`.
Host renderer executable: `2fbe5f4f86d80aa7ee2138d8f3bdb5483cb82c3f7f52b4429b189b6e24095400`.
PNG capture: `230107afd4688e679fa5d0893e85bc4084cdaa035cb014b50d2f0d8975eefa4d`.
Host GL headers/runtime were extracted locally from checksum-verified Ubuntu
libglvnd 1.7.0-1build1 packages. An attempted system installation failed its
privilege transition; no system-install success is claimed.

## Actual producer receipts

Flexible Pipes `9775aa324f523cbc6c758e89461915484a5a0fc7` invoked the real
`android-apk-preflight` pipeline. Both stage exits are zero; raw stdout/stderr,
pipeline digests and receipts are retained under `pipeline/`.

Merged ai-ci policy: `01608a2493fa409463f70e8fbfd8a123ef59ee85`.
Merged canonical packager: `7c61ee43e75f7c2dab9288edb0e10055898b36e6`.
The existing APKs were originally compiled by NDK 29.0.14206865, API 21.
Verification used build-tools 35.0.0 from the official archive with checked
SHA-1 `2cfaa0bbb2336e9ec18ed3ecea84fa2e2af607bc`.

| Device/ABI | APK SHA-256 | Current producer |
| --- | --- | --- |
| MIRO_A1 / armeabi-v7a | f6b2437a124acd37b07cc3296ea7153f886a5aad4744de5b75b079963802dca1 | PASS; miro-a1-producer.tsv |
| MIRO_C67 / arm64-v8a | 9d629d58ec0782b4b8b8ed0e0071d03b6855c85990315982eec836f78d09e734 | PASS; miro-c67-producer.tsv |

Both: package `org.isomorphisms.mostow`, versionCode 1, public test certificate
`de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9`.
The gate checks the real APK signature, receipt agreement, NativeActivity,
exact ABI set and no DEX. The verifier's inherited `cc` invocation was routed
explicitly to the hash-pinned ICK driver, with its separate runtime path.

Replacement identity comparison is NOT_VERIFIED: no previous accepted producer
receipt was supplied. Installation, launch, lifecycle, physical GPU, touch,
sustained timing/memory and visual acceptance on each phone remain NOT_RUN.
There is still no required GitHub workflow for this repository. No release,
PR merge, or final user visual approval is inferred from producer PASS.
