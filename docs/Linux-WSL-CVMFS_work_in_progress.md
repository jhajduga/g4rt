## Preparing the environment based on CVMFS

1) WSL instalation, see [docs/Ubuntu-CVMFS.md](Ubuntu-CVMFS.md)

2) It's assumed the following is installed in localhost
```
1) gcc/g++
2) X11
3) OpenGL
4) Motif
5) Xorg
```

Install all with this command:
```
sudo apt-get install -y build-essential libx11-dev libegl1-mesa-dev libmotif-dev xorg-dev
```

3) Install Anaconda and create environment
Miniconda with Python 3.9: 
```
wget https://repo.anaconda.com/miniconda/Miniconda3-py39_23.1.0-1-Linux-x86_64.sh
bash Miniconda3-py39_23.1.0-1-Linux-x86_64.sh
rm Miniconda3-py39_23.1.0-1-Linux-x86_64.sh

```
re-open your terminal window and test with `conda list`.

Create `dose3d` environemt (`conda_env.yml` is located in project main directory):
```
conda env create --name dose3d --file conda_env.yml
```

## Setup environement and appliacation build
First, setup the cmvfs env with running:
```
source setup-cvmfs-ubuntu2004.sh
```
Then activate conda environment:
```
conda activate dose3d
```
Initialize external submodules:
```
git submodule update --init --recursive
```

Run cmake and build:
```
mkdir build; cd build
cmake ../
make -j 4
```