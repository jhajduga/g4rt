<!-- 🛠️ Internal document for J. Hajduga's PhD simulation platform documentation propouses. -->

# Physics List Overview

This document describes the configuration and purpose of each physics component registered
in the `PhysicsList::ConstructProcess()` method, used in Geant4-based simulations for medical physics
applications (0–10 MeV range).

## Summary

The physics list includes:

- **Electromagnetic interactions**
- **Decay and radioactive processes**
- **Optical effects (e.g., Cerenkov, Scintillation)**
- **Hadronic and ion physics (elastic and inelastic)**
- **Atomic de-excitation (Auger, fluorescence, PIXE)**
- **Fine-tuned EM parameters for high accuracy**

---

## Components Breakdown

### 1. Transportation

`AddTransportation()`

- Mandatory in Geant4 to allow particle propagation through geometry.

---

### 2. Electromagnetic Physics

`m_emPhysicsModelCtr->ConstructProcess()`

- Custom electromagnetic physics constructor.
- Typically includes standard EM processes like ionization, multiple scattering, Bremsstrahlung, etc.

---

### 3. Extra Electromagnetic Processes

`G4EmExtraPhysics`

- Includes optional processes:
  - Cerenkov radiation
  - Scintillation
  - Synchrotron radiation

---

### 4. Decay Physics

`G4DecayPhysics`

- Standard decays for particles such as:
  - Muons
  - Pions
  - Kaons

---

### 5. Radioactive Decay

`G4RadioactiveDecayPhysics`

- Models nuclear decays:
  - Beta decay
  - Alpha decay
  - Isomeric transitions

---

### 6. Hadronic Elastic and Inelastic Interactions

#### a. Elastic

`G4HadronElasticPhysicsHP`

- High-precision elastic scattering, mainly for neutrons.

`G4IonElasticPhysics`

- Elastic scattering for ions (protons, alphas, heavier ions).

#### b. Inelastic

`G4HadronPhysicsQGSP_BIC_HP`

- High-precision inelastic hadronic models (includes neutrons, protons).

`G4IonPhysics`

- Inelastic and ionization interactions for ions.

---

### 7. Stopping Physics

`G4StoppingPhysics`

- Handles the stopping of low-energy particles:
  - Muon capture
  - Annihilation at rest
  - Other low-energy termination effects

---

### 8. EM Parameter Configuration

`G4EmParameters* param = G4EmParameters::Instance()`

| Setting | Description |
|--------|-------------|
| `SetAuger(true)` | Enables Auger electron emission |
| `SetFluo(true)` | Enables X-ray fluorescence |
| `SetPixe(true)` | Enables PIXE (proton-induced X-rays) |
| `SetAugerCascade(true)` | Enables full inner-shell cascade |
| `SetDeexcitationActiveRegion(...)` | Enables de-excitation for all regions |
| `SetDeexcitationIgnoreCut(true)` | Ignores production threshold for secondary particles |
| `SetLowestElectronEnergy(10*eV)` | Tracks electrons down to 10 eV |
| `SetStepFunction(0.2, 100*um)` | Controls electron step size for better energy loss accuracy |
| `SetUseMottCorrection(true)` | Applies Mott scattering correction for electrons |
| `SetUseICRU90Data(true)` | Uses updated ICRU90 data for electron stopping powers |

---

## Optional Features

### Step Limitation

`AddStepMax()` *(commented out by default)*

- Can be used to enforce user-defined step size limits for specific volumes or particles.

---

## Purpose

This physics list is designed for **high-accuracy medical physics simulations**, including:

- Dosimetry
- Voxelized phantoms
- Radiotherapy planning studies
- Radioisotope interaction tracking

It balances **precision** (e.g., low-energy thresholds, Mott corrections) with **completeness**
(e.g., inclusion of Cerenkov and PIXE).

---

## 🔄 Step Limitation Mechanisms

Controlling particle step length is essential for balancing simulation **accuracy** and **performance**, especially in dose-sensitive applications.

This simulation supports three distinct mechanisms for step control:

---

### 1. `SetStepFunction(...)` – *EM-dependent dynamic limiter*

**Applies to:** Electrons and other charged particles via EM processes
**Type:** *Soft constraint*

```cpp
G4EmParameters::Instance()->SetStepFunction(0.2, 100*um);
```

- Restricts the step length based on material properties and the gradient of energy loss (`dE/dx`).
- Does not impose a hard limit — longer steps may occur in uniform energy regions.
- Automatically respected by standard EM physics (e.g., ionization, multiple scattering).
- Improves precision in **low-energy tracking**, especially in thin or layered geometries.

---

### 2. `G4StepLimiterPhysics` – *Global hard step cap*

**Applies to:** All particles (if configured)
**Type:** *Hard constraint*

```cpp
m_stepLimitPhysicsCtr->SetApplyToAll(true);
m_stepLimitPhysicsCtr->SetMaxAllowedStep(0.5 * mm);  // Optional
```

- Registers the `G4StepLimiter` process for all particles.
- Enforces a maximum step size uniformly, regardless of material or energy.
- Useful for **simple, global step limiting** (e.g. to match voxel granularity).
- Cleaner and easier to manage than custom per-particle logic.

---

### 3. `StepMax` – *Custom discrete process*

**Applies to:** Particles selected manually
**Type:** *Hard constraint*

```cpp
StepMax* stepMaxProcess = new StepMax();
stepMaxProcess->SetMaxStep(0.5 * mm);
pmanager->AddDiscreteProcess(stepMaxProcess);
```

- Fully user-defined discrete process.
- Can be selectively added only to specific particles or particle types.
- Gives full control over applicability, thresholds, and behaviors.
- Particularly useful in **geometry-sensitive studies**, **debugging**, or **region-specific restrictions**.

---

### ⚖️ Interaction Between Methods

If multiple step-limiting mechanisms are active for a given particle, **the shortest step limit takes precedence**.

For example:

| Mechanism              | Limit    |
| ---------------------- | -------- |
| `SetStepFunction`      | \~1.2 mm |
| `StepMax`              | 0.5 mm   |
| `G4StepLimiterPhysics` | 0.8 mm   |
| **Effective step**     | → 0.5 mm |

---

### 🧭 Recommended Use Strategy

| Use Case                            | Recommended Mechanism(s)               |
| ----------------------------------- | -------------------------------------- |
| General EM precision                | `SetStepFunction`                      |
| Global maximum step control         | `G4StepLimiterPhysics`                 |
| Region- or particle-specific limit  | `StepMax`                              |
| Fast prototyping / rough simulation | EM physics only (no additional limit)  |
| Dose scoring in voxel geometries    | Combine `SetStepFunction` + hard limit |

---

### 🚧 Implementation Notes

- Currently, `StepMax` is available but **disabled by default**.
- If activated (`AddStepMax()`), it applies to **non-short-lived particles** only.
- If `G4StepLimiterPhysics` is used, the `SetApplyToAll(true)` ensures broad coverage.

### 🧠 Why Step Limitation Matters

Accurate simulation of particle transport is critical in medical physics applications such as **dose mapping**, **scintillation signal modeling**, and **CT-based voxel geometry**.

In particular:

- **Long steps** may skip over small voxels or localized dose hotspots.
- **Very short steps** improve accuracy but increase runtime significantly.
- Step control mechanisms allow tuning this trade-off depending on use case.

**For example:**

- In **voxelized water phantoms**, a step size larger than voxel size may miss energy deposition events entirely.
- In **plastic scintillators**, improper step resolution may distort light yield simulations.

By combining `SetStepFunction` (accuracy-driven) with a hard limiter (`StepMax` or `StepLimiterPhysics`), the simulation ensures **high fidelity of energy loss**, **dose scoring**, and **optical photon generation**.

### 💡 Add to: "Optional Features"

Rozbuduj istniejące `Optional Features` tak:

---

## Optional Features

### Optical Physics

`G4OpticalPhysics`

- Enables optical photon tracking and interaction processes:

  - Boundary interactions (reflection, refraction, absorption)
  - Wavelength shifting (if configured)
  - Rayleigh scattering
  - Bulk absorption
- Required if simulating:

  - Scintillators
  - Cerenkov detectors
  - Optical readouts
- Automatically registers the `G4OpticalPhoton` particle.

## 🧪 Example Configurations (new section)

To show how users/devs could apply the physics list:

---

### Example Config: Default EM with Step Control

```ini
[RunSvc]
Physics = emstandard_opt4
EnableExtraProcesses = true
EnableStepLimiter = true
MaxStepLength = 0.5*mm
```

- Uses `G4EmStandardPhysics_option4`
- Enables Cerenkov and Scintillation via `G4EmExtraPhysics` + `G4OpticalPhysics`
- Enforces 0.5 mm hard step limit on all particles

---

### Example Config: Lightweight (fast) mode

```ini
[RunSvc]
Physics = LowE_Penelope
EnableExtraProcesses = false
EnableStepLimiter = false
```

- No optical processes, no hadronic physics, no hard step limit.
- Good for estimating EM-only energy deposition or quick beam transport tests.

---

## 📈 Future Extensions

- Optional DNA-level physics for nanodosimetry (e.g., `G4EmDNAPhysics`)
