# tiny-fmm

Online document: **[readthedocs](http://tiny-fmm.readthedocs.io/)**

<!--intro-start-->

## Usage

Install:

```bash
python3 -m pip install tiny-fmm # install from pypi
```

CLI interface: (created with [python-fire](https://github.com/google/python-fire))

```bash
TODO
```

<!--intro-end-->

---

# Make a release

(We now use Github Workflow to release to pypi. Skip the rest if you don't want to manually compile wheels.)

## On linux

Install docker then

```
make python_build_all_in_linux
make upload_wheels
```

## On macOS

Install c++ compiler and cmake.

Install conda and envs:

```
# conda create -y -n py36 python=3.6
# conda create -y -n py37 python=3.7
conda create -y -n py38 python=3.8
conda create -y -n py39 python=3.9
conda create -y -n py310 python=3.10
conda env list
```

Then

```
make python_build_all_in_macos
make upload_wheels
```

## On windows

Install visual studio and cmake, (also git for windows, maybe).

Install conda and envs same as on macOS, then:

```
make python_build_all_in_windows
make upload_wheels
```
