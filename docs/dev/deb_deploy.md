# PPA repository

The G4RT binaries are being published and can be installed via Debian-based PPA package manager.
This is done with hosting the deb packages on the dedicated github repository, [dose3d/ppa](https://github.com/dose3d/ppa). Repository has been prepared based on [this](https://assafmo.github.io/2019/05/02/ppa-repo-hosted-on-github.html) tutorial.

# Add a GPG key to the apt sources keyring
Once you had never done this before on your machine, you have to import our custom gpg key: 
```
gpg --import dose3d-priv-key.asc
```
Note: get the `dose3d-priv-key.asc` file from package mainteiners.

# Generate and publish the new .deb file
## 1. Increase the package version
Edit: [executables/CMakeLists.txt](../../executables/CMakeLists.txt) 
at this line:
```
SET(CPACK_PACKAGE_VERSION "X.X.X")
```

## 2. Run cmake and cpak
Note: once you run the `cpak` without building the sources before, it will do it for you.
## 3. Publish new .deb file
1. Move you .deb file to the ppa repository: the `ppa/ubuntu/` directory
2. Go to `ppa/share` directory and run `./generate_repository.sh`
3. Send your changes to the remote repo:
```
git add --all
git commit -m "upgrade g4rt deb to version x.x.x"
git push origin
```