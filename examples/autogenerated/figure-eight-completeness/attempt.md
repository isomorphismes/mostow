# Figure-eight completeness encounter

Purpose: compare freely varied ideal-tetrahedron shapes with the complete
`m004` shape pair using actual edge and peripheral equations. This is a new
encounter, separate from the living sheet and string-art work.

Domain: two oriented ideal shapes in the upper complex half-plane. Each has
three edge parameters: z, 1/(1−z), (z−1)/z. A diagnostic contains two edge
logarithmic residuals against 2πi and two cusp residuals against zero. Invalid
or nonfinite shapes produce unavailable diagnostics, never a zero error.
The display uses a separate trial pair and the known complete reference pair;
requiring completeness does not silently turn a failed trial into a solution.

The four integer exponent rows are recorded from SnapPy's official `m004`
documentation. The positive complete solution is z₀=z₁=(1+i√3)/2. Combining
the meridian equation with the first edge equation gives z₁²−z₁+1=0; positivity
selects the upper root. This is a specific census model, not a general proof
of Mostow rigidity or an interval-certified triangulation implementation.

The Idriç attempt `M004Shape.idric` retains an explicitly named binary64 host
prototype. The located source compiler at
`94dfd99bd3e376507fedc8611053b7173b2519f0` cannot start: its retained launcher
references an absent `idris2_app/idris2.so` and exits 127 before checking.
No source error or Android Idriç execution is claimed.

The executable consumer follows Mostow's established compositional ICK C
owner and explicit binary64 model boundary, with binary32 projection only
in the renderer. This is an explicit native implementation, not Idriç
lowering, RefC or an oracle pretending to be the requested compiler.
Independent fixtures check the regular complete pair, a positive edge-only
incomplete pair, invalid orientation/nonfinite input, and logarithmic winding.

Needed language work: a qualified complex scalar/record ABI and direct target
lowering for these checked shape and residual operations. Acceptance must run
the actual emitted module at the declared ABI before replacing this consumer.
