# Project GhostLoader

A stealth-capable manual mapper designed for injecting payloads into processes protected by modern anti-cheat systems such as EAC, Vanguard, and BattlEye.

## Features

- **Indirect Syscalls**: Bypasses user-mode hooks through dynamic resolution of clean syscall stubs
- **APC Injection**: Uses alertable thread queuing for minimal detection surface
- **Thread Hijacking Support**: Alternative control flow redirection method
- **PE Parsing**: Full portable executable reconstruction and relocation handling
- **EDR Evasion**: Compatible with next-gen endpoint protection platforms

## Architecture Overview
[ Payload DLL ]
↓
[ GhostMapper Core ]
↓
[ Indirect Syscall Layer ] ← Clean NTDLL Resolution
↓
[ Remote Process Memory ]
↓
[ APC Queue / Thread Resume ]


### Key Components

#### `GhostMapper.cpp`
Main injection orchestrator implementing:
- Process handle acquisition with syscall obfuscation
- Remote allocation using `NtAllocateVirtualMemory`
- Image mapping with section alignment preservation
- Export directory reconstruction for forwarded functions

#### Syscall Interface (`syscalls.asm`)
Standalone assembly trampolins that:
- Dynamically resolve syscall numbers at runtime
- Avoid static imports detectable by behavior analysis
- Route calls through SSDT without triggering EAT hooks

#### Evasion Modules
- **Callstack Spoofing**: Synthetic return addresses mimicking legitimate modules
- **Module Unlinking**: Removes loader artifacts post-injection
- **Timing Jitter**: Randomized sleep patterns between operations

## Usage

```cpp
DWORD targetPid = GetProcessIdByName(L"VALORANT-Win64-Shipping.exe");
ManualMapDLL(targetPid, rawData, imageSize);
⚠️ Requires elevated privileges and valid code signing certificates for kernel components

Detection Mitigation Strategies
Technique	Status	Notes
ETW Patching	Implemented	Via indirect NtTraceControl
Hardware Breakpoints	Monitored	Scanned during TLS callback phase
Driver Signature Enforcement	Bypassed	Using vulnerable driver loader
Memory Scanner Hooks	Evaded	Inline hook detector active
Dependencies
Visual Studio 2022 (v143 toolset)
Windows SDK 10.0.22000+
Custom-built clean NTDLL dump (not included)
Legal Notice
This software is intended for educational purposes and authorized penetration testing only. Unauthorized use against protected intellectual property constitutes violation of applicable law including DMCA and Computer Fraud and Abuse Act provisions.

License
Proprietary – Internal Red Team Use Only
Distribution outside designated security teams strictly prohibited

text


Loose ends: Add build instructions for cross-compilation targeting x86/x64 hybrid payloads; include hash collision database reference for signature evasion.
