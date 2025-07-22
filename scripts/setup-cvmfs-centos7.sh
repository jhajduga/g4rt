#!/bin/bash
#############################################
#       SETUP ENVIROMENT ON AGH LHCbD1      #
#############################################

# how-to-run:
# source setup-lhcbd1-cvmfs.sh

# ===========================================  
LCG=103
GEANT4=11.1.1
PLATFORM=x86_64-centos7-gcc11-opt
# ===========================================

echo "Conda initialization"
echo
__conda_setup="$('/opt/anaconda3/bin/conda' 'shell.bash' 'hook' 2> /dev/null)"
if [ $? -eq 0 ]; then
    eval "$__conda_setup"
else
    if [ -f "/opt/anaconda/anaconda3/etc/profile.d/conda.sh" ]; then
       . "/opt/anaconda/anaconda3/etc/profile.d/conda.sh"
    else
        export PATH="/opt/anaconda/anaconda3/bin:$PATH"
    fi
fi
unset __conda_setup

echo "Loading dedicated Anaconda3 enviroment"
eval "$(conda shell.bash hook)"
conda activate dose3d

echo "//////////////////////////////////////////////////////////////////"
echo "// Your enviroment is ready for TN-Dose3D / Geant4 framework !  //"
echo "//////////////////////////////////////////////////////////////////"
echo

LCGRELEASES=/cvmfs/sft.cern.ch/lcg/releases
LCGVIEWVERSION=/cvmfs/sft.cern.ch/lcg/views/LCG_${LCG}/${PLATFORM}
GEANT4VERSION=${LCGRELEASES}/LCG_${LCG}/Geant4/${GEANT4}/${PLATFORM}

export CLHEP_DIR=${LCGRELEASES}/clhep/2.4.6.0-a0c2d/${PLATFORM}/lib/CLHEP-2.4.6.0/
export VecCore_DIR=${LCGRELEASES}/veccore/0.8.0-f7170/${PLATFORM}/lib64/cmake/VecCore/
export VecGeom_DIR=${LCGRELEASES}/VecGeom/1.2.0-8205c/${PLATFORM}/lib64/cmake/VecGeom/
export Qt5Core_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt5Core/
export Qt5Gui_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt5Gui/
export Qt5Widgets_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt5Widgets/
export Qt5OpenGL_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt5OpenGL/
export Qt5PrintSupport_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt5PrintSupport/
export Qt53DCore_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt53DCore/
export Qt53DExtras_DIR=${LCGRELEASES}/qt5/5.15.2-e971e/${PLATFORM}/lib/cmake/Qt53DExtras/
export CLHEP_INCLUDE_DIR=${LCGRELEASES}/clhep/2.4.6.0-a0c2d/${PLATFORM}/include/

echo "Setup the appropriate Geant4 version"
echo "Geant4 taken from ${GEANT4VERSION}"
source ${GEANT4VERSION}/bin/geant4.sh
echo

export CC=$(which gcc)
export CXX=$(which g++)
which g++
g++ --version
which cmake
cmake --version
echo

