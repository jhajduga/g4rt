# Install CVMFS on Ubuntu 20.04
[Official docs](https://cvmfs.readthedocs.io/en/stable/cpt-quickstart.html)
```
wget https://ecsft.cern.ch/dist/cvmfs/cvmfs-release/cvmfs-release-latest_all.deb
sudo dpkg -i cvmfs-release-latest_all.deb
rm -f cvmfs-release-latest_all.deb
sudo apt-get update
sudo apt-get install cvmfs
```
Once working in WSL:
```
sudo cvmfs_config wsl2_start
```

Create `/etc/cvmfs/default.local` with the content:
```
CVMFS_REPOSITORIES=sft.cern.ch,geant4.cern.ch
CVMFS_QUOTA_LIMIT=20000
CVMFS_HTTP_PROXY=DIRECT
```
Then prepare directories for mounting:
```
mkdir -p /cvmfs/sft.cern.ch/
mkdir /cvmfs/geant4.cern.ch/
```
For auto mount on WSL put the following in `/etc/wsl.conf`
```
[boot]
command = "mount -t cvmfs sft.cern.ch /cvmfs/sft.cern.ch; mount -t cvmfs geant4.cern.ch /cvmfs/geant4.cern.ch"
```