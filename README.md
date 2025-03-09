# Scheme RayTracer

Couple goals in this project:  

1. **Scheme Language Interpreter**  
   Build a small interpreter capable of parsing and executing Scheme code.

2. **LLVM IR Generator for Scheme**  
   Translate Scheme code into LLVM IR to leverage LLVM’s optimization and code generation capabilities.

3. **RayTracer in Scheme**  
   Implement a basic ray tracer, with the logic written in Scheme and then either interpreted or compiled via the LLVM backend.

4. **Custom LLVM Architecture**  
   Experiment with a custom architecture (arch) by extending or modifying LLVM to target this new platform.

5. **Simulator for the Custom Architecture**  
   Provide a simulation environment for the custom architecture, enabling execution and testing of compiled programs without specialized hardware.

## Project Structure

- **.vscode/**  
  Contains Visual Studio Code settings and configuration files.

- **doc/**  
  Includes design notes, ideas, and TODO lists to guide development.

- **include/**  
  Houses external libraries or modified components:
  - `imgui/` — UI library for graphical interfaces.
  - `llvm-project-mod/` — Custom modifications or extensions to LLVM.

- **lang/**  
  Contains the core logic for the Scheme language tasks:
  - **codegen/** — Modules responsible for generating LLVM IR from Scheme.
  - **scripts/** — Auxiliary scripts for building or testing.
  - **src/** — Source files for the interpreter or related utilities.
  - **test/** — Testing framework and example Scheme programs.
  - **tools/** — Additional tools that assist with code analysis or debugging.

- **sim/**  
  Focuses on simulating the custom architecture:
  - **cpu/**, **instruction/**, etc. — Components defining CPU instructions, registers, and execution flow.
  - **gui/** — Optional graphical interface for visualization or debugging.
  - **main.cpp** — Entry point for running the simulator.
  - **ISA.hpp** — Instruction Set Architecture definitions for the custom platform.

- **README.md**  
  This file, describing the project’s goals and layout.

## Getting Started

1. **Clone the Repository:**  
   ```bash
   git clone <repository-url>
   ```

2. **Explore the Directories:**  
   - For the Scheme interpreter and IR generation, see the `lang/` folder.
   - For architecture simulation, check `sim/`.

3. **Build and Run (High-Level Idea):**  
   Since CMake is used for building, you can typically do:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```
