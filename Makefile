all: wheel

.PHONY: build wheel install uninstall reinstall clean

build:
	python3 setup.py build

wheel:
	python3 setup.py bdist_wheel

install: wheel
	pip3 install dist/pywf-0.0.5-*.whl --user

uninstall:
	pip3 uninstall -y pywf

reinstall: build wheel uninstall install

clean:
	rm -rf ./build ./pywf.egg-info ./dist
