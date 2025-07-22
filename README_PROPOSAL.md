# G4RT – Geant4-based Radiotherapy Toolkit / RadioTherapy

## 🚀 Overview

G4RT is a modular Monte Carlo simulation platform for modeling radiotherapy devices, based on the [Geant4](https://geant4.web.cern.ch/) framework. It supports a wide range of beamline geometries, patient-specific setups, and exportable outputs. The platform is actively being deweloped to support database-driven construction and direct import of CAD models in 3MF format.

---

## 🌐 Features

* ✨ **Geometry from 3MF and DB**: Supports building device geometry from structured databases or directly from 3MF CAD files.
* 🔢 **Multithreaded simulation**: Parallelized execution using Geant4's built-in multithreading.
* 🔍 **Flexible configuration**: All jobs are described in TOML files stored in the `jobs/` directory.
* 💡 **Export modes**: Geometry can be exported as:

  * CSV
  * ROOT TFile
  * GDML
* 🌐 **CLI interface** with argument parsing via `cxxopts`.

---

## ⚙️ Build Instructions

### Prerequisites

* Linux (Strongly recomended)
* CMake >= 3.16
* Geant4 >= 11.0
* Conda >= 4.10 (with `mamba` recommended)
* Python >= 3.8

### Installation

````bash
# Clone with submodules
git clone --recursive git@github.com:dose3d/g4rt.git
# or, after clone:
git submodule update --init --recursive

# Setup environment
mamba env create -f g4rt_env.yml
conda activate g4rt

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)
````

---

## 📂 Project Structure

````text
g4rt/
├── app/              # Simulation apps with main() functions
│   └── basic/        # Example app using TOML and CLI
├── core/             # Core logic: geometry, physics, services
├── data/             # Input assets (materials, mesh, fonts)
├── jobs/             # Job configuration files (*.toml)
├── output/           # Output of simulations (TFiles, logs, etc.)
├── scripts/          # Utility scripts
├── externals/        # External dependencies and libraries
├── docs/             # Documentation (e.g., WSL setup)
├── submodules/       # Git submodules: 3MF, parsers
````

---

## 🚧 Example Usage

### Basic simulation with TOML config

````bash
./build/app/g4rt \
  --FullSimulation \
  --TOML jobs/phantom_job.toml \
  --OutputDir output/phantom_run \
  --nCPU 8
````

### Geometry export (GDML)

````bash
./build/app/g4rt \
  --BuildGeometry \
  --GeoExportGdml \
  --TOML jobs/export_gdml.toml \
  --OutputDir output/gdml
````

---

## 🔮 Output

Simulation results and logs are written to the directory specified by `--OutputDir`. Depending on options, this may include:

* PLZ uptade me 

---

## 🎓 Documentation & Contribution

* 📄 Full documentation (will be available before Jacob's @jhajduga dissertation defense) under `docs/`
* 🔧 Contributions welcome? Open an issue or submit a PR

---

## 📃 License

MIT License. See [LICENSE](../LICENSE) file for details.

### ☢️ Geant4 Notice

This software makes use of the Geant4 toolkit (https://geant4.web.cern.ch/),
which is distributed under the Geant4 Software License. For details, please refer to:
https://geant4.web.cern.ch/license


---

## 💬 Contact

For inquiries, bug reports, or contributions, please visit the [GitHub Issues](https://github.com/dose3d/g4rt/issues) page.
