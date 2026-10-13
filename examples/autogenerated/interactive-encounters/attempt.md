# Interactive encounters — SUN CHESSAPIG–MOSTOW-01

The requested implementation language and renderer are existing ICKY C and
NativeActivity/GLES2. This explicitly selected route takes precedence over an
Idriç port. No alternate mathematical kernel is introduced.

Values: an immutable intrinsic sheet; sampled positions and renderer normals;
a face identity with three barycentric weights; a projective boundary map;
ordered string identities; a camera; an illumination vector; visual treatment;
an explicit seed; source and fixture revisions.

Operations: sample(sheet, time) → checked render buffers and diagnostics;
anchor(face, weights, buffers) → position and interpolated renderer normal;
pick(buffers, camera, pixel) → nearest visible face anchor;
boundary_action(matrix, projective_point) → checked boundary point;
geodesic(endpoints, model) → checked points with unchanged identity;
save(scene, path) and reopen(path) → validated application-owned scene;
export(framebuffer, scene, path) → still image and companion scene.

Sampling and geometry are pure. File output, GPU drawing, and input are distinct
effects. Equivalent matrix representatives act identically. Equal endpoints
are degenerate, antipodes determine a diameter, negative determinant is outside
the supported group, and numerically unresolved classifications stay uncertain.
An extrinsic overlap never joins abstract faces. Treatment does not mutate
mathematical buffers, time, selection, topology, or diagnostics.

Target: the existing off-device pinned ICK → NDK route, ARMv7 and ARM64.
Phone execution and source/host/build acceptance retain separate receipts.
Implementation and acceptance are in progress; this record claims no PASS.
