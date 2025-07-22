#!/bin/bash
#############################################
#       SETUP ENVIROMENT WITH CVMFS         #
#############################################
#
LCG=102
ARCH=x86_64-ubuntu2004
GCC=gcc9
ROOT=6.26.04
GEANT4=11.0.2
#
# how-to-run: source setup-cvmfs-ubuntu2004.sh
# Note: you should run this script before activating conda environment!
# ===========================================
# Prerequesities: 
#   (It's assumed the following is installed in localhost)
#   1) gcc/g++  | sudo apt-get install build-essential
#   2) X11      | sudo apt-get install -y libx11-dev
#   3) OpenGL   | sudo apt-get install -y libegl1-mesa-dev
#   4) Motif    | sudo apt-get install -y libmotif-dev
#   5) Xorg     | sudo apt-get install -y xorg-dev
# ===========================================

LCGVIEWVERSION=/cvmfs/sft.cern.ch/lcg/views/LCG_${LCG}/${ARCH}-${GCC}-opt

echo "[g++]:"
export CC=$(which gcc)
ąexport CXX=$(which g++)
g++ --version
echo
echo "[LCG=${LCG} view]:"
echo "Setup script ${LCGVIEWVERSION}/setup.sh"
source ${LCGVIEWVERSION}/setup.sh # setup CMake and number of depandancies
echo
echo "[CMake]:"
cmake --version

echo
echo "[ROOT]:"
ROOTVERSION=/cvmfs/sft.cern.ch/lcg/releases/LCG_${LCG}/ROOT/${ROOT}/${ARCH}-${GCC}-opt
source ${ROOTVERSION}/bin/thisroot.sh
echo "${ROOTVERSION}"

echo
echo "[Geant4]:"
GEANT4VERSION=/cvmfs/sft.cern.ch/lcg/releases/LCG_${LCG}/Geant4/${GEANT4}/${ARCH}-${GCC}-opt
source ${GEANT4VERSION}/bin/geant4.sh
echo "${G4INSTALL}"

#unset unnecessery paths:
export PYTHONHOME=""
export PYTHONPATH=""

echo
echo "*************  Your enviroment is setup with LCG=${LCG}  *************"
echo "*************  You can now activate conda environment  ************"
echo
