
# G4RT - Linac simulator based on Geant4 framework

## Custom WSL and Anaconda environment
Typical Geant4-based application is being in the Linux OS. Hence, once you already have your distribution with Geant4 being installed, you can utilize the `conda_env.yml` to install external packages within the conda environmet and the install the `G4RT` trough the PPA (Debian users) or build from source. You can get it from:  
```
wget https://raw.githubusercontent.com/dose3d/ppa/main/share/conda_env.yml
```  
For Windows OS users the custom WSL with all `G4RT` prerequisities being installed is available, see [docs/wls-ubuntu-22.04.md](docs/wsl-ubuntu22.04-G4.md)

## Install on Debian-based Linux from PPA
You can use package manager to install latest released version of this software (see [https://github.com/dose3d/ppa](https://github.com/dose3d/ppa)).

## Build from source
Note: All prerequisites are assumed to be installed (Geant4 and conda environment).
* clone this repo: `https://git.plgrid.pl/scm/tnd3d/g4rt.git`
* Define/update/activate conda environment based on the `yml` file (see current version of the [conda_env.yml](https://github.com/dose3d/ppa/blob/main/share/conda_env.yml)
* In the project directory `mkdir build; cd build; cmake ../; make -j 4`

## Run application tests
There are number of tests being developed in the G4RT application. You can start these tests by running the comman from the `build` directory:
```
ctest
```

## Run the g4rt application
See [run_g4rt.md](docs/run_g4rt.md)