[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Phantoms](https://img.shields.io/badge/Phantoms-2-%23e67e22)](#-phantoms)
[![Patients](https://img.shields.io/badge/Patients-Example-%239b59b6)](#-patients)

# 🎯 Dose3D – Phantoms And Patients

**A curated collection of 3D phantom models and example DICOM CT patient datasets for dose visualization and simulation QA.**

This repository contains:
- 🧪 **Two modular phantoms** (Modular Water Phantom & IBA ImRT Phantom) stored in `.csv`, `.xlsx` and `.3mf` formats
- 🧬 **Example DICOM CT datasets** to illustrate input structure for simulations
- 📜 **Utilities for building formatted Excel databases and visualizing dose simulations in 3D**

---

## 📁 Repository Structure

```

.
├── LICENSE
├── README.md                                               📖 you're reading this
├── data/
│   ├── iba_sim_cp10x10_d3ddetector_cell.csv                📈 sample simulation results
│   └── iba_sim_vis.png                                     🖼️ rendered dose visualization
├── patient/
│   ├── dicom/
│   │   └── patient_01_CT/
│   │       └── README.md                                   ℹ️ scan metadata
│   └── phantoms/
│       ├── IBA_ImRT_phantom/
│       │   ├── D3DF_bodies.csv                             📑 raw 3D geometry
│       │   ├── D3DF_bodies.xlsx                            📊 formatted mapping + metadata
│       │   ├── D3DF_components.csv                         ⚙️ component table
│       │   └── Phantom_v18.3mf                             🖨️ 3MF 3D model
│       └── modular_water_phantom/
│           ├── D3DF_bodies.csv
│           ├── D3DF_bodies.xlsx
│           ├── D3DF_components.csv
│           └── WaterPhantom_v8.3mf
└── utils/
    ├── build_db_from_fusion_export.py                      🧠 convert raw CSV to structured XLSX
    ├── config/
    │   ├── Bookerly.ttf                                    ✍️ font for 3D labels
    │   ├── material_dictionary_iba.json
    │   └── material_dictionary_modular_water_phantom.json
    └── mesh_visualizer/
        └── phantom_model_vis.py                            🎥 render 3D scene with dose overlay

````

---

## 🔍 Phantoms

Each phantom folder includes:
- `D3DF_bodies.csv`: raw geometry and attributes
- `D3DF_bodies.xlsx`: exported and cleaned mapping DB
- `Phantom_vXX.3mf`: 3D-printable model exported from CAD

| Model Name                 | Folder                                       | Description                                  |
|:---------------------------|:---------------------------------------------|:---------------------------------------------|
| 🧪 **Modular Water Phantom** | `patient/phantoms/modular_water_phantom/`   | Modular design for water-based QA.           |
| ⚙️ **IBA ImRT Phantom**       | `patient/phantoms/IBA_ImRT_phantom/`        | IBA’s insertable QA phantom.                 |

---

## 🧑‍⚕️ Patients

This repo includes placeholder structure for anonymized DICOM CT studies (for dose calculation pipelines).

- **patient_01_CT**: Head/neck
- Add more patients to `patient/dicom/`

Each folder should contain:
- A series of `.dcm` files (numbered slices)
- A `README.md` with scan info (modality, slice thickness, etc.)

---

## 📜 Scripts

This repository includes utility scripts to help with:

* 🧠 converting raw 3D component data into structured Excel files,
* 🎥 visualizing simulated dose distributions over phantom geometry.

---

### 🧠 `utils/build_db_from_fusion_export.py`

**Purpose**:
Converts raw `.csv` geometry/component exports (e.g. from CAD/DB) into a cleaned, standardized `.xlsx` format.

**Highlights**:

* Normalizes column names (e.g. `ComponentName` → `Component_Name`)
* Adds empty fields for scintillator/channel mapping (to be filled manually)
* Generates a unique hash for each row for reproducibility
* Reorders and optionally hides selected columns
* Saves two Excel sheets: main data + editable metadata
* Supports material name translation using a JSON dictionary

**Input**:

* Raw `.csv` file with 3D and component data
* Optional material mapping `.json` file

**Output**:

* `.xlsx` with two sheets: `scintillator_mapping_db` and `Metadata`

**Usage**:

````bash
python utils/build_db_from_fusion_export.py \
  --csv_path patient/phantoms/IBA_ImRT_phantom/D3DF_bodies.csv \
  --output patient/phantoms/IBA_ImRT_phantom/D3DF_bodies.xlsx \
  --material_map utils/config/material_dictionary_iba.json
````

---

### 🎥 `utils/mesh_visualizer/phantom_model_vis.py`

**Purpose**:
Displays an interactive 3D scene with geometry colored by simulation results and saves a rendered image with a dose scale.

**Highlights**:

* Automatically resolves `.csv` and `.xlsx` paths from a single `--db` input (files should have the same name with different extension)
* Reads mesh and mapping data, applies color based on associated values
* Adds coordinate axes and a labeled colorbar
* Smart camera framing based on scene bounds
* Shows interactive preview and saves a high-resolution `.png`

**Input**:

* `--db`: path to either `.csv` or `.xlsx` (both must exist)
* `--sim`: simulation results `.csv`
* Optional: `--create_voxel` (feature under development)

**Output**:

* Rendered `.png` with colored model and colorbar

**Usage**:

````bash
python utils/mesh_visualizer/phantom_model_vis.py \
  --db patient/phantoms/modular_water_phantom/D3DF_bodies.xlsx \
  --sim data/iba_sim_cp10x10_d3ddetector_cell.csv \
  --output data/iba_sim_vis.png \
  --loglevel DEBUG
````

---

## 🔗 Submodule Integration

This repository can be used as a Git submodule. Example (for Dose3D simulator):

````bash
# In the simulator repo root:
git submodule add git@github.com:Dose3D-Future/d3df-patients.git submodules/d3df-patients
git commit -m "Add d3df-patients as submodule"
````

To initialize on clone:

````bash
git clone --recursive git@github.com:dose3d/g4rt.git
# or, after clone:
git submodule update --init --recursive
````

To update to latest commit of this submodule:

````bash
git submodule update --remote submodules/d3df-patients
git add submodules/d3df-patients
git commit -m "Update phantom submodule"
git push
````

---

## 🤝 Contributing

* Add new phantom models under `patient/phantoms/`
* Drop additional DICOM series under `patient/dicom/`
* Contribute scripts to `utils/`
* Open a PR with a meaningful description and preview if possible ✨

---

## 📄 License

This project is licensed under the **MIT License**.
See [LICENSE](LICENSE) for details.

---

*Let science be reproducible. And a little bit pretty.* ✨

