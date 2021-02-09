#!/bin/sh -e

export PREFIX=""
if [ -d 'venv' ] ; then
    export PREFIX="venv/bin/"
fi

set -x

pip install -r requirements/requirements-lint.txt

FILES="tutorial setup.py"

${PREFIX}autoflake --in-place --recursive --remove-all-unused-imports --remove-unused-variables ${FILES}
${PREFIX}black --exclude=".pyi$" ${FILES}
${PREFIX}isort --multi-line=3 --trailing-comma --force-grid-wrap=0 --combine-as --line-width 88 ${FILES}
