# MinHook source dependency

Unmodified x64-required sources from the official MinHook v1.3.4 tag:

- Upstream: https://github.com/TsudaKageyu/minhook
- Commit: c3fcafdc10146beb5919319d0683e44e3c30d537
- Download: https://codeload.github.com/TsudaKageyu/minhook/zip/c3fcafdc10146beb5919319d0683e44e3c30d537
- Archive SHA-256: cdcb160f734d81bd4d235dfea79e3f5a661c8ef0ab74fa814272aa5449069034

Only include/MinHook.h, buffer/hook/trampoline sources and headers, and the
hde64 disassembler dependencies are vendored. LICENSE.txt retains the MinHook
and HDE notices. No original game code or derived binary is included.

Only the ordinary runtime ASI links this dependency. The protected probe and
launcher do not. The build copies the notice beside its ASIs and fingerprints
the vendored tree and notice. Future binary packaging must retain that notice.
The runtime keeps its forwarding detour/trampoline resident until process exit;
it does not unload this library or remove the hook during scene changes.
