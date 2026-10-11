# Paired build candidates — producer acceptance blocked

These are review checkpoints, not accepted installation handoffs. Do not
install them until the central producer gate passes. Both use package
`org.isomorphisms.mostow`, versionCode 1, versionName 0.1, and the existing
public test certificate. Both were built from exact source
`12aa1481bad53334e1506435f8bf8c71060960c4`.

| Target | ABI | Bytes | APK SHA-256 | State |
|---|---|---:|---|---|
| MIRO A1 | armeabi-v7a | 53692 | `f6b2437a124acd37b07cc3296ea7153f886a5aad4744de5b75b079963802dca1` | Build/package PASS; producer FAIL; physical NOT_RUN |
| MIRO C67 | arm64-v8a | 57786 | `9d629d58ec0782b4b8b8ed0e0071d03b6855c85990315982eec836f78d09e734` | Build/package PASS; producer FAIL; physical NOT_RUN |

Signer certificate SHA-256:
`de9b1d47c5a65e6d46a204b79dd9ee566b9d3c9832ba81ebc4213d3392e92ff9`.
Each adjacent `.receipt.tsv` is the canonical packager's actual output.
There is no producer PASS receipt: merged policy rejects this unregistered
package. See the parent [qualification record](../README.md).

After registration merges, run the unchanged Flexible Pipes Android producer
preflight on these exact APK bytes and receipts; retain both successful producer
receipts. Then qualify A1 first: replacement installation, launch, genuine GPU
capture, orbit/pinch, pause/rings/info, background/resume, and five-minute
frame/memory observation. Follow with C67 using its own exact artifact. A1
physical acceptance does not qualify C67. No device-side compilation is part
of either follower.
