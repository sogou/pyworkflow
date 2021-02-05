'''Root module of pywf'''
from .cpp_pyworkflow import *

def py_func():
    print("This is a pure python function")

inner_init()
del inner_init
