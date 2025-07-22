# G4RT TODO Task List

## 🔧 Configuration and Setup

- **TLD.hh:17** - Resolve conflict between ROOT and CADMesh (macro naming and include order issues)
- **ConfigSvc.hh:18** - Add std::ostream flexibility

## 🎯 Actions and Control

- **ControlPoint.hh:33** - Introduce FieldType as enum type (see definition in Types.hh)
- **RunAction.cc:71** - Review and verify code implementation

## 📊 Data Analysis

- **BeamAnalysis.cc:126** - Check if this is the same as preStepPoint-GetTotalEnergy()
- **RunAnalysis.cc:21** - Implement RUN_CSV_ANALYSIS
- **RunAnalysis.cc:23** - Implement RUN_NTUPLE_ANALYSIS  
- **RunAnalysis.cc:25** - Implement RUN_HDF5_ANALYSIS
- **StepAnalysis.cc:63** - Define basic histograms

## 🏗️ Geometry

### Linac

- **MlcHD120.cc:40** - Complete implementation
- **MlcSimplified.cc:20** - Get values from configuration
- **README.md:2** - Add table summarizing each model parameterization
- **BeamCollimation.cc:83** - Complete implementation

### Patient/Detector

- **D3DCell.cc:157** - Pass pv and extract global coordinates from it
- **D3DCell.cc:182** - Extract this from Detector::name scope
- **D3DDetector.cc:26** - Complete implementation
- **D3DDetector.cc:209** - Implement method

### Phantom

- **DishCubePhantom.cc** - Implement methods (lines 24, 80, 99, 103, 107)
- **IbaImRT.cc:59** - Filter elements to be removed from IbaImRT
- **IbaImRT.cc:60** - Create module to fetch DB and CSV paths from PhantomWorld
- **SciSlicePhantom.cc:21,64** - Implement methods
- **WaterPhantom.cc:144** - Implement method
- **WaterPhantom.cc:152** - Verify implementation
- **PatientTest.hh:34** - Make std::unique
- **VPatientSD.hh:53** - Check if this is really needed here

### VoxelHit

- **VoxelHit.cc:184** - Store all particles and interactions (not just electrons)
- **VoxelHit.cc:209** - Store Parent ID or Primary ID for visualization purposes
- **VoxelHit.cc:241,257** - Handle these errors
- **VoxelHit.cc:268** - Replace running 1/2-average with true arithmetic mean
- **VoxelHit.cc:506** - Check dose formula implementation
- **VoxelHit_README.md:151** - Continue documentation

### PhaseSpace

- **SavePhSpAnalysis.cc:54** - Move code from SavePhSpSD::ProcessHits
- **SavePhSpSD.cc:30** - Proper handling of data dumping into phsp directory within NTuple

### WorldConstruction

- **LinacGeometry.cc:154** - Refactor code to smart pointers
- **LinacGeometry.hh:36** - No geometry rotation - particles should be rotated after going through Jaws and MLC
- **PatientGeometry.cc:323** - Update GetPhysicalVolume() method
- **PatientGeometry.cc:439,539** - Make generic for any patient
- **SavePhSpConstruction.cc:70** - Move definition to final/specific model definition
- **SavePhSpConstruction.cc:72** - Revise logic for creating SavePhSpSD instances
- **WorldConstruction.cc:340,348** - Make this work for entire geometry tree

## ⚛️ Physics

- **IaeaPrimaryGenerator.cc:27** - Check these functions
- **IaeaPrimaryGenerator.cc:32** - Check how to setup multiple PHSP files
- **IonPrimaryGenerator.cc:27** - Finalize source specification
- **IonPrimaryGenerator.cc:33** - Configure settings
- **PhysicsList.cc:14** - Consider migrating to fully modular physics list

## 🛠️ Services

- **DicomSvc.cc:65** - Refactor context-specific code
- **DicomSvc.cc:138,147,380** - Complete implementation
- **DicomSvc.cc:384** - Implement FieldType::RTPlan
- **DicomSvc.cc:397** - Implement FieldType::CustomPlan
- **DicomSvc.hh:63** - Apply DRY principle
- **GeoSvc.cc:244** - Refactor temporary code
- **GeoSvc.cc:369** - Use enum types
- **RunSvc.cc:271** - Define operation modes
- **RunSvc.cc:465** - Check condition (always true for now)
- **RunSvc.cc:511** - Implement methods for exporting specific world volumes
- **RunSvc.cc:565** - Fix error when closing phasespace file
- **RunSvc.cc:600** - Complete implementation
- **RunSvc.hh:25,26** - Implement TpFractionCounter and DaqTimeCounter
- **Services.cc:243** - Note about RDF::MakeCsvDataFrame

## 🔧 Utilities

- **UIManager.cc:106** - Implement runSvc-GetCurrentRun()

## 📁 Data and Configuration

- **gpsCLinac_pre.mac:80** - Check /gps/ene/emspec 0
- **basic_iba_job.toml:27** - If exists - read and load
- **basic_gps.toml:25** - Define type

## 🔗 External Libraries ()

- **G4IAEAphspReader.hh:227** - Create setter for multiple files and parallel readout
- **G4IAEAphspReader.cc:142** - Add possibility to introduce theSourceReadId
- **G4IAEAphspReader.cc:543** - Place filtering logic here
- **loguru.cpp:77** - Use defined(_POSIX_VERSION)
- **loguru.cpp:1061** - Store thread name on weird platforms
- **loguru.cpp:1904** - Implement signal handlers on Windows
- **loguru.hpp:1198** - Fix HACK

---------------------------------------------

## Additional (non in-code TODO) tasks

### 1. **Write full technical documentation to support PhD thesis writing**

 **Labels:** `documentation`, `PhD`, `high-priority`
 Create complete internal documentation for all major modules:

- Simulator architecture overview
- Geometry (parameterized, imported, CT-based)
- Scoring systems and logic
- Output handling (ROOT, HDF5, CSV)
- Configuration (TOML, DICOM plan integration)
- Example runs and diagrams for thesis inclusion

---------------------------------------------

### 2. **Transplant or refactor HDF5 output from fork into main repo**

 **Labels:** `data-output`, `HDF5`, `refactor`
 Bring HDF5 output functionality (currently implemented in a separate fork with HighFive) into the mainline repo.

- Align it with `NTupleEventAnalysis` and central flag registry
- Ensure per-thread safety
- Provide config-based switch via Analysis class selection (already supported)

---------------------------------------------

### 3. **Export simulation events (tracks, secondaries) from ROOT to JSON for web viewers**

 **Labels:** `export`, `visualization`, `three.js`
 Implement export of per-event data to JSON for use in Three.js or other 3D viewers:

- Include voxel hits, particle types, parent ID, origin, etc.
- Slice/filter by event or particle
- Use PyROOT or native ROOT I/O to extract and serialize

---------------------------------------------

### **4. Compare Geant4 multithreading vs sub-event multitasking (with and without Intel TBB)**

 **Labels:** `multithreading`, `performance`, `benchmark`, `research`

 **Description:**
 Investigate and benchmark alternative parallelization models in the simulation system:

- **Standard Geant4 multithreading** (`G4MTRunManager`)
- **Custom sub-event multitasking**, e.g., manually spawning and processing subranges of events/tasks

 Additionally, compare how Geant4 behaves when compiled:

- with **Intel TBB** support enabled
- without TBB (default POSIX threads or std::thread)

 **Goals:**

- Assess performance (event throughput, memory usage)
- Evaluate thread-safety of scoring and I/O layers under both models
- Identify architectural constraints or potential bugs
- Determine if manual multitasking provides meaningful benefits

 **Checklist:**

- [ ] Document baseline: how Geant4 is compiled and linked in both configurations
- [ ] Run simulations on fixed workload (e.g., 10M particles) using each model
- [ ] Collect timing, CPU/memory usage, and scaling data
- [ ] Log any race conditions or thread-contention patterns
- [ ] Prepare plots/tables for inclusion in thesis or performance report
- [ ] Compare for different scenarios:
  - [ ] Greater/lesser voxelization.
  - [ ] Greater/less detailed physics.
  - [ ] Greater/lesser energy.
  - [ ] Greater/lesser particle lifespan.

### 5. **Benchmark output backends: ROOT vs HDF5 vs CSV**

 **Labels:** `benchmark`, `data-output`, `performance`
 Perform I/O benchmarks of output systems using representative datasets:

- Time to write per event
- File size
- Post-processing complexity
   Output results in table and plot form. Useful for making informed default choices and for thesis discussion.

---------------------------------------------

### 6. **Add scoring map compression for sparse volumes**

 **Labels:** `scoring`, `performance`, `compression`
 Implement optional compression of sparse scoring maps (e.g., RLE, coordinate-delta encoding, or HDF5 compression).

- Activate via config flag
- Compare performance and size benefits
- Validate correctness via checksum tests

---------------------------------------------

### **7. Add unit tests for core systems: flags, geometry, run configuration**

 **Labels:** `testing`, `core`, `gtest`, `high-priority`

 **Description:**
 Our current unit test coverage is minimal and fragmented.
 This issue tracks the systematic addition of unit tests for all critical parts of the simulation framework.
 Focus first on:

- AnalysisFlagRegistry logic (flag registration, per-thread behavior)
- Geometry service (volume loading, material assignment)
- Run configuration parsing (TOML settings, overrides, default values)

 **Goals:**

- Ensure each component has isolated, repeatable tests
- Use `gtest` as the base framework (already partially used?)
- Make all tests runnable via CMake with `-DBUILD_TESTING=ON`
- Lay groundwork for CI integration in future

 **Initial Checklist:**  

- [ ] Add test cases for `AnalysisFlags`, `BeamAnalysis`, `BeamAnalysis`... and so on
- [ ] Add tests for geometry loading
  - [ ] Load minimal phantom
  - [ ] Check scoring volume registration
  - [ ] Validate material map
- [ ] Add tests for TOML-based run configuration
  - [ ] Required fields
  - [ ] Defaults
  - [ ] Invalid config detection
- [ ] Add CMake targets for test builds
- [ ] Add test runner target (`ctest` integration)

 **Future:**

- [ ] Add tests for scoring output correctness
- [ ] Add round-trip tests for export/import*
- [ ] Measure coverage with `gcov`/`lcov` or `llvm-cov`

#### *

 **Title:** Add round-trip tests for export/import of dose and geometry data
 **Labels:** `testing`, `data-io`, `validation`

 **Description:**
 Add automated round-trip tests to ensure that exported data (dose maps, geometry, metadata) can be re-imported without loss or distortion.

 **Examples:**

- Export dose to RTDOSE → Re-import and compare voxel values
- Export dose to CSV → Read back → Compare stats
- Export geometry/material map → Reload → Check ID / material integrity

 **Checklist:**

- [ ] Create minimal dose phantom, export, re-import
- [ ] Compare voxel count, max/min/mean
- [ ] Add test to CI suite (`gtest`, Python, or CLI)

---------------------------------------------

### 8. **Allow skipping geometry / phantom loading in dry-run mode**

 **Labels:** `cli`, `testing`, `core`
 Implement a `--dry-run` mode that parses the configuration and sets up services, but skips geometry loading and event loop.

- Useful for config testing and CI
- Should fail if TOML or plan is invalid
- Output a brief summary and exit

---------------------------------------------
