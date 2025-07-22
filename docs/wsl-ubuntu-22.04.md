# Installing custom WSL distribution with Geant4 and Anaconda

The custom WSL built on top of the Ubuntu 22.04 distribution has been prepared with Geant4 v11.1.1 and Anaconda being installed. You can download it from [figshare.com/dose3d/wsl-geant4](https://figshare.com/articles/software/Custom_WSL_Ubuntu_22_04_distribution_with_Geant4/22762019)

After downloading the `tar` file you can simply import the distribution by running the `Power Shell` command:
```
wsl --import YourCustomWslName /Path/To/Wsl/InstalationDir /Path/To/Wsl/TarFile
```
e.g.
```
wsl --import Ubuntu-22.04-G4RTUI C:\Users\brachwal\WSLs\WSL_Ubuntu-22-04-G4UI C:\Users\brachwal\Downloads\WSL_Ubuntu-22.04.tar
```
Notes:
* The instalation path has to be unique for distribution you want to install
* The default user is geant4 with password geant4
* You can always personalize the custom distro, see description bellow.

## Personalize the WSL distribution
### Creating new user
You can add your own user to this distro instance. However, it's important to note that Anaconda instalation has been installed in `geant4` space. Doing some tricks you can utilize it without any problems. Example belowe is given with creating the user named brachwal:
```
sudo useradd -m -G sudo,geant4 brachwal
sudo passwd brachwal
```
Switch the default user being loaded durint the WSL start:
```
sudo vi /etc/wsl.conf
```
Edit this fragment:
```
[user]
default = brachwal
```
It may happer that the `bash` shell is not being set properly. Hence, after the brachwal login run the command `chsh` and set the following:
```
/bin/bash
```

### Creating local conda environemts
In order to utilize the geant4 miniconda instalation you can simply run the script `source /home/geant4/miniconda3/etc/profile.d/conda.sh`  
Now, by default env would be placed still in geant4 user space, you can switch this to your new location:  
```
mkdir -p ~/.conda/envs
```
and edit this file `vi .condarc` and put the following:
```
envs_dirs:
  - ~/.conda/envs
```

###  Git basic configuration
Remember to update your git credentials:
```
git config --global user.name "John Doe"
git config --global user.email johndoe@example.com
git config --global core.editor vi
```
