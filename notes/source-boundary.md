# Authoritative source and direct ICK build

The first-party mathematical source is `icky/mostow_sheet.c`; the host-only
fixture construction is `icky/realize_sheet.c`. Both retain literal `←`
assignment and now use `÷` division. The camera, renderer, NativeActivity and
host test sources use the same division frontend. All 76 owned arithmetic
divisions were migrated; the 10 C/header files and generated fixture include
were inspected. Include paths, strings and comments retain their slashes.

ICK source `c61e448251744a2f40ad743ebef1a027bdcd2f9d` compiles original source
directly. The old Python lexical adapter and its tests remain historical
compatibility controls; no maintained application or fixture producer invokes
it to translate source. The host controls still run its existing negative tests.

`tools/test-host.grease` and `tools/generate-fixture.grease` require
`MOSTOW_HOST_COMPILER`, an absolute qualified ICK compiler path.
`tools/render-host.grease ROOT OUTPUT SECONDS YAW PITCH` additionally requires
`MOSTOW_GL_ROOT` containing the host GL headers/libraries.
The host runtime profile uses `-fno-link-libatomic`; it does not introduce a
different mathematical implementation.

For Android, `android/build-native.grease ROOT NDK ABI OUTPUT` requires both
`MOSTOW_ICK_ARMV7` and `MOSTOW_ICK_ARM64`. It verifies the selected compiler
target and its own builtin headers, then compiles all four owned C translation
units to assembly. Android NDK r29, revision 29.0.14206865, API 21 assembles that
output, compiles its own unmodified NativeActivity glue, and links Bionic,
EGL/GLES and the platform libraries. Its original O2, PIC, strict-warning,
16 KB link-alignment and stripping choices remain. ARM retains the original
A32/NEON/softfp profile; AArch64 reserves Android's x18 register. No source
normalization or generic C fallback is involved.

The shared compiler jobs use
`isomorphisms/ai-ci/ick-android@4ea071a96239f3a29ca6d98454feb59947d87cfe`.
They retain compiler qualification separately from Mostow's actual library,
packaging and runtime results. This raw producer had no Fortify setting;
the migration does not claim a new fortified profile.

The existing seven-argument `android/build-pair.grease` interface is retained.
It still requires clean material source, invokes the canonical packager, keeps
the existing package/version/test signer, and produces separate MIRO A1 and
MIRO C67 APKs. `tools/preflight-pair.grease` executes the actual pinned Flexible
Pipes pipeline with merged ai-ci policy. That policy's fixed `cc` entrypoint is
explicitly routed through `tools/compile-policy.grease` to the selected ICK
driver; policy bytes and verifier sources stay untouched. The adapter changes
no source text.

The fixture's 1,023 positions are data emitted by the authoritative host
generator. Regeneration installs only an accepted candidate. Its deterministic
continuation remains a preprocessing construction; no optimizer runs in the
application or defines continuous motion.

See [division qualification](../qualification/division-20261009.md) for the
executed host, regenerated-data, render and Android-library checks. The M2/S2
folders retain their dated compatibility-build evidence. Installation,
lifecycle, physical GPU, touch and sustained performance on each phone remain
separate acceptance requirements.

The verified Grease artifact uses its inherited `ysh` engine filename; it is
the identified Grease implementation, not a substituted stock Oils runtime.
The traced local host needs `ASAN_OPTIONS=detect_leaks=0`. No Grease leak
acceptance follows from those executions.
