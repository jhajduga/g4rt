Exist only for J.Hajduga PhD propouse

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

## Notes

- The physics list can be modularized further using builder classes.
- It assumes use of **G4 units**, and is optimized for the default world region.
- TODO: Uptade rest of the code here. 