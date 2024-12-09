# llvm_jit

Lightweight emulator designed to execute ARM64 binaries. It has a JIT compiler, memory management, and profiling capabilities. The emulator is built in C and aims to provide a flexible/efficient environment for running ARM64 coe.

### Prerequisites

- GCC/Clang
- LLVM
- A brain

## Example/Usage

To compile and run an ARM64 binary using the LLVM JIT emulator, do the following:

   ```bash
   gcc -o cock cock.c -target aarch64-unknown-linux-gnu
   ```

Run the emulator w/ the compiled binary:

   ```bash
   ./llvm_jit --input cock --output profile.log --profile
   ```

   - `--input`: Specify the path to the ARM64 binary you want to execute.
   - `--output`: (Optional) Specify a file to log profiling information.
   - `--profile`: Enable profiling to gather performance metrics during execution.

View the profiling results in the specified output file to analyze the performance

