## 🔗 Submodule Integration

### What is a submodule?

A submodule is simply a reference in one Git repo (the **parent**) to a specific commit in another Git repo (the **submodule**). That reference lives in two places:

1. A file called `.gitmodules` in the parent, which stores the remote URL and local path.  
2. A “gitlink” entry in the parent’s index pointing at the exact commit SHA of the submodule.

This lets you keep one repo inside another, but still treat it as an independent project.

---

### 📦 Adding this repo as a submodule to your simulator

````bash
# In your simulator’s repo root:
git submodule add git@github.com:Dose3D-Future/d3df-patients.git submodules/d3df-patients
git commit -m "Add d3df-patients as submodule under submodules/d3df-patients"
````

That will create or update a file called `.gitmodules`:


```bash
[submodule "submodules/g4rtd3df-patients"]
    path = submodules/g4rtd3df-patients
    url  = git@github.com:Dose3D-Future/d3df-patients.git
````


---

### ⏬ Cloning or checking out the parent repo

When someone clones the parent repo, submodules come in “empty” by default. They must run:

````bash
git clone git@github.com:dose3d/g4rt.git
cd g4rt
git submodule update --init --recursive
````

Or shorter:

````bash
git clone --recursive git@github.com:dose3d/g4rt.git
````

---

### 🔄 Updating the submodule in the **parent** repo

If this phantom‐patient repo has new commits and you want your simulator to pick them up:

````bash
# In your simulator repo:
cd submodules/g4rtd3df-patients
git fetch origin
git checkout main        # or whatever branch you track
git pull                 # now submodules/g4rtd3df-patients points to tip of main
cd ../..                 
git add submodules/g4rtd3df-patients    # stage the new gitlink
git commit -m "Bump d3df-patients to latest version"
git push
````

Alternatively, from the parent root:

````bash
git submodule update --remote submodules/g4rtd3df-patients
git add submodules/g4rtd3df-patients
git commit -m "Update d3df-patients submodule to latest"
git push
````

---

### 🛠 Working **inside** this repo (as a submodule)

If you need to modify the phantoms/patients repo itself:

````bash
# From within submodules/g4rtd3df-patients (or clone this repo standalone):
cd submodules/g4rtd3df-patients
# make your edits…
git add .
git commit -m "Add new patient CT series"
git push origin main
````

Then go back to the parent and follow the “🔄 Updating…” steps above so the parent picks up your changes.

---

### 🧹 Removing or de-initializing submodules

* **Remove entry from .gitmodules & .git/config**
* **Delete the tracked directory and commit**

````bash
git submodule deinit -f submodules/g4rtd3df-patients
git rm -f submodules/g4rtd3df-patients
rm -rf .git/modules/submodules/g4rtd3df-patients
git commit -m "Remove d3df-patients submodule"
````