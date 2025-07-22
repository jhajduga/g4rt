## TOML job structure
The main job configuration of the G4RT utilizing the [TOML](https://toml.io/en/) standard (instead of the G4 macro files). See the basic structure of the job: [docs/toml_job_structure.md](docs/toml_job_structure.md)

## Get IAEA phsp 
Once the simulation conditions are defined for `BeamType = "IAEA"` in order to run the simulation you need to download these data.  
You can download it from Google Drive, [LINK](https://drive.google.com/drive/folders/1YYZcZWz9aDPvN27rMXhd_7E1X6a3SEDd).

The IAEA phsp reference has to be specified within the job toml file (without file extenstion), e.g.:
```
[RunSvc]
BeamType = "IAEA"
PhspInputFileName = "your_path/phsp/Clinac2300-PhSp-Primo/TNSIM157_10x10s2-f0"
```

## Run simulation with G4RT

To run simulation with the job defined in toml file, simply run from cmd line (being inside the build directory):
```
./executables/g4rt -j 4-o /home/brachwal/Workspace/Dose3D -f -t ../jobs/testbeam/3cells_in_z.toml
```
where:
* `-j` number of CPU threads to run simulation on,
* `-o` output directory (defaul value is defined as `project_dir/output/`),
* `-t` path to `.toml` file with job configuration,
* `-f` run full simulation mode.  

To print all available options for command line:
```
./executables/run-toml-mode --help
```

## Predefined Dose3D jobs

* Default single module (4x4x4 cells). [See docs/Dose3D_single_module.md](docs/Dose3D_single_module.md)
* May 2023 testbeam and related studies. [See docs/Testbeam_052023.md](docs/Testbeam_052023.md)