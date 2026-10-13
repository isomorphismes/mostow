# Inspectable encounters — SUN CHESSAPIG–MOSTOW-01

The immutable sheet fixture/kernel remain unchanged. Clean/ink share sampled
buffers. Washes, intrinsic-coordinate fibres, highlights and silhouette/boundary
ink use an explicit seed. Rings denote intrinsic radius, separate from shading.

Picking retains a perspective-correct face/barycentric anchor. Nearest visible
depth wins; ties within 1e-7 NDC choose the lowest face ID. Crossings never join
faces. Selection/normal strokes obey depth. The unit-sphere inset uses the same
interpolated, normalized renderer normal and stays visible when occluded.

MORE cycles three phone pages. Controls expose direct time scrub, pause,
light azimuth/height, explanatory overlays, save/reopen and PNG export. Strings
offer continuous angle/boost/shear controls, independent viewpoint rotation and
shift, and Klein/Poincaré switching. Pinch changes display scale. Presets cover
identity, elliptic, parabolic and hyperbolic maps.

Boundary samples use [cos(angle/2):sin(angle/2)] on RP1, including infinity
[1:0]. After matrix action, the disk endpoint is (x²−y²,2xy)/(x²+y²). A and −A
act equally. The map is R(angle/2) D(exp(boost/2)) U(shear). Viewpoint acts on
both endpoints afterward and does not modify the retained map.

The fixed-point equation c x²+(d−a)x−b=0 has discriminant trace²−4 det.
For positive determinant and a nonidentity map its negative/zero/positive sign
means elliptic/parabolic/hyperbolic. Infinity is counted separately when c=0.
Tests independently count projective roots. Small integer entries bounded by
2²⁰ have exact product/discriminant arithmetic in double. Scalar identity and
equal-diagonal triangular parabolic maps have structural checks. Other near-zero
determinants/discriminants report UNCERTAIN with 64/128 epsilon margins; a rounded
zero is insufficient. Singular, reversed and nonfinite exact inputs are rejected.

Poincaré points are k/(1+sqrt(1−|k|²)) for Klein chord points k; inverse is
2p/(1+|p|²). Circle centers satisfy C·u=C·v=1 and |C|²−radius²=1. Tests check
incidence and orthogonality. Antipodes give a diameter; equal endpoints within
1e-12 are marked degenerate. Stroke thickness proves no analytic envelope.

The plain scene record retains compiled source/fixture, map, time, anchor/string,
camera/model, light, treatment and seed. Save is atomic and file-synchronized;
startup/reopen validate source/schema/fixture and reject older incompatible
records. Pause persists state. Gesture pointers are not semantic state.
EXPORT reads the actual framebuffer before swap, writing `mostow-export.png`
and `mostow-export.scene` to NativeActivity's external app-files directory,
or internal files if external storage is unavailable. The screen reports status;
the app log names exact paths. Exports deliberately replace these two files.

The reusable implemented boundary is `native/mostow_scene.h`: boundary actions,
model conversion, stable string IDs and face anchors are platform-free. GLES
and Android own resources/lifecycle/file paths. No scene engine or second sheet
kernel is required. Finite-mesh L/K still describe the reference metric and
motion certificate; they do not establish Mostow rigidity.

Semantic replay is deterministic. Byte-identical pixel replay is tested only
within one Mesa llvmpipe context; no cross-GPU pixel identity is promised.
The host recording exercises shared control operations, not physical touch.
Device installation/update, lifecycle, gestures, export visibility, memory and
sustained timing remain separate physical acceptance obligations.

Sources actually inspected on 13 October 2026: Elliot Kienzle's
[Hyperbolic String Art](https://chessapig.github.io/files/hyperbolic_string_art.pdf),
text pp. 1–4 including the model construction and reversed trace inequalities;
[To draw a torus](https://chessapig.github.io/talks/torus), talk-page description
only; existing Mostow source boundary, motion certificate and STAR-M1.
The classification derivation is independently checked. No PDF, figures,
artist code/media or private corpus is distributed. Redistribution permission
was not established. Slides were not read. Treatments are independently authored.
