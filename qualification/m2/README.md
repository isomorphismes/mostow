# M2 evidence and pending boundaries

Host geometry: PASS. Base realization: PASS. Analytic all-time motion bound:
PASS. Actual emitted-float poses/corners: PASS. Intrinsic area refinement:
PASS. Shared GLES rendering in Mesa llvmpipe host EGL: PASS.

Both Android native ABIs build via the explicit NDK compatibility route.
Signing/packaging and producer receipts are recorded separately. The shared
producer gate requires a merged registration of `org.isomorphisms.mostow`
with the established public test certificate. No pending gate is a PASS.

## Exact checkpoint

Source/build commit: `12aa1481bad53334e1506435f8bf8c71060960c4`.
The evidence-only descendant adds receipts, the host preview, and candidates;
it does not change mathematical or application source. Candidate APK bytes and
packager receipts are retained in [candidates](candidates/README.md).

NDK revision: 29.0.14206865, Android API 21. Canonical packager:
`isomorphisms/android-NDK` at `7c61ee43e75f7c2dab9288edb0e10055898b36e6`.
Producer policy: merged `isomorphisms/ai-ci` at
`8bf8be153f792c4def251287ed85543d1fe24f07`. Flexible Pipes:
`c8cb7ac069a798eaf6cc228de9a2c23b2b351587`.

Both real producer attempts exit 1 with
`UNREGISTERED_PACKAGE_LANE package=org.isomorphisms.mostow lane=test`.
Full stdout/stderr are retained per ABI here. This is a failed producer gate,
not a failed native build. Registration is proposed in
[isomorphisms/ai-ci PR #228, “Register Mostow's stable public test Android signer”](https://github.com/isomorphisms/ai-ci/pull/228).
Candidate-registry validation accepts the correct signer, rejects a wrong
Mostow signer, and passes all six existing signing cases. It does not replace
the gate against merged policy.

Host compiler: Ubuntu GCC 13.3.0 with C11, optimization and strict warnings.
Host preview: actual shared GLES2 renderer in Mesa llvmpipe (LLVM 20.1.2,
256 bits), at active time 0. It is [shown here](host-preview.png).
The verified Grease executable SHA-256 is
`7e31cd05b7a9d8fb2a4a9e003a7f3fcb0159138506d17f0fb28da8cbe22aa85c`;
its full source provenance is not established by this consumer job. Execution
used `ASAN_OPTIONS=detect_leaks=0` because the container's process tracing
prevents LeakSanitizer operation. No leak-test PASS is claimed.

A1 installation, launch, touch, background/resume, five-minute frame/memory
report, and actual PowerVR capture: NOT_RUN. C67 corresponding acceptance:
NOT_RUN. This container has no established physical-device channel. No
compiler or build tool is to be installed on either phone.

The host render deliberately uses the same shaders, fixture, kernel, and
camera as the Android application. Its purpose is inspecting source output;
it cannot establish PowerVR behavior or phone touch comfort. The current
patch is a faceted first realization, with small folds near the outer edge;
it is not claimed to be a unique smooth shape or collision-free embedding.

## Physical session after producer acceptance

Use the exact A1 artifact/digest first. Open the app, watch separated folds
for 20–30 seconds, toggle rings/info, orbit, pinch, pause, background, and
return. Record whether the phase remains continuous and whether both sides
remain legible. Capture genuine device output, device identity, GPU, and APK
digest. Run for five foreground minutes and record native `FRAME_RENDER_MS`
and `FRAME_INTERVAL_MS` separately, plus process memory. Target the stated
95th-percentile frame budget of 33.3 ms; report an observed failure honestly.
Repeat on C67 using its own arm64 artifact. One device does not qualify the
other. Replacement installs preserve application identity; do not uninstall
an existing package to work around a signer/version conflict.
