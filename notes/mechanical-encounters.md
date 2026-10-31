# Mechanical encounter ownership

The scene selects eight encounters: the retained sheet, boundary strings and
figure-eight example, then the genus-2 quotient, Fenchel–Nielsen pants,
local distortion, integer inclusion, and rigidity argument. The first button
cycles them; MORE changes the control page. All new parameters are persisted
as hexadecimal floating-point values in scene schema 4. Older scene schemas
are explicitly rejected rather than silently losing parameters.

`icky/mostow_octagon.c`, `icky/mostow_pants.c`, `icky/mostow_distortion.c` and
`icky/mostow_m004.c` own the mathematics. `icky/mostow_scene.c` owns parameter
selection and prescribed transformations. GLES draws their samples and
diagnostics. The shader and strokes do not decide topology, completeness,
holonomy, conformality or a quasi-isometry constant.

The quotient panel is a cut fundamental polygon. Matching labels identify
reversed boundary directions. The cover panel displays the central tile and
eight side neighbors, or just its central tile. Tapping the left disk or using
the direction buttons transports a point through the actual side isometries.
The accumulated lift matrix and crossing word describe that transport. The
word uses cancellation of adjacent inverses; it is not a canonical solution
of the surface-group word problem. The nine drawn tiles are a finite window,
not the entire universal cover. Overlapping strokes do not identify points.

The pants panel shows four actual right hexagons in cut drawings. It does not
claim an isometric embedding of a closed hyperbolic surface in three-space.
Lengths vary from 0.75 to 3 and signed arclength twists from −2 to 2 through
the phone controls. The three bars encode the three transverse closed
geodesic lengths returned by the holonomy calculation. A twist can change
those lengths while the displayed cut hexagons stay congruent. All three
lengths and twists remain separately editable; the second control page
selects twist 3. Changed marked geodesic lengths demonstrate a change of
marked structure, rather than proving two unmarked surfaces nonisometric.

The distortion panel displays tangent planes in their respective
metric-orthonormal frames. Its source/target metrics are either both identity
or source diag(4,1) and target identity. Rotation, positive uniform scaling,
shear, reflection followed by shear, and a singular differential can be
selected. A singular differential has no positive minor singular value and
no finite dilatation; the panel reports the refusal and does not draw a
fabricated ellipse. The signed area and orientation remain distinct from K.

For the integer inclusion, the global proof is short. For integers m and n,
their distance |m−n| is unchanged in ℝ, so L = 1 and A = 0. The intervals
[n−1/2,n+1/2] cover ℝ, so every real point lies within B = 1/2 of the image.
The finite line panel illustrates this proof; its finite samples do not
establish the infinite assertion. The computational witness has its own
documented binary64 domain in the kernel.

The rigidity account keeps completeness, finite volume, dimension at least
three, and a specified fundamental-group isomorphism visible. Its pages give
the cited boundary/straightening/volume route and explicitly distinguish it
from executable finite checks. In the cusped case the required boundary
extension is a cited finite-volume lemma; a bare orbit map does not supply
that conclusion. The volume route follows the primary references already
pinned in STAR M1: Thurston chapter 6, sections 6.3–6.4, and Milnor's notes in
chapter 7. Those citations do not turn the numerical examples into a proof.

On the tetrahedron page, both examples start with the regular shape. A
nontrivial Möbius transformation preserves the recomputed cross-ratio. The
other example first applies the orientation-preserving real-linear map
w ↦ w + k conjugate(w), |k| < 1, then the same Möbius transformation.
The mathematical kernel returns both actual transformed sphere vertices and
the recomputed cross-ratio. The volume is evaluated from that shape, rather
than inferred from a drawing. A nonconformal boundary homeomorphism is not
being claimed to descend to an isomorphism of complete manifolds.

Host acceptance replays the shared control operations, requires changes in
the geometric framebuffer region when semantic parameters change, and
requires exact framebuffer reopening from each saved new state. Those checks
are rendering evidence. Their current execution results, source revision,
compiler/artifact identities and independent mathematical acceptance belong
in the corresponding run receipts. They are not physical-device evidence.
