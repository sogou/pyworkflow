# PyWorkflow - A Python Binding of C++ Workflow

## Build

```bash
# CentOS 7
yum install cmake3 ninja-build python36 python36-devel python36-pip
yum install gcc-c++ # if needed
# clone this project with all submodules, and then
cd pyworkflow
pip3 install wheel
python3 setup.py bdist_wheel
```

```bash
# CentOS 8
yum install cmake ninja-build python36 python36-devel python3-pip
# clone this project with all submodules, and then
cd pyworkflow
pip3 install wheel
python3 setup.py bdist_wheel
```
