#!/usr/bin/make -f
# -*- makefile coding: utf-8 -*
# ex: set tabstop=4 noexpandtab:
# SPDX-License-Identifier: Zlib
# SPDX-License-URL: https://spdx.org/licenses/Zlib
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

default: help all/default
	@echo "$@: TODO: Support more than $^ by default"
	@date -u

SELF?=${CURDIR}/helper.mk

project?=z-wave-nvm-migration-tool
url?=https://github.com/SiliconLabsSoftware/z-wave-nvm-migration-tool

version?=$(shell git describe --tags --always 2> /dev/null || echo "0")

# Allow overloading from env if needed
VERBOSE?=1

cmake?=cmake
cmake_options?=-B ${build_dir}
ctest?=ctest

build_dir?=build
sudo?=sudo

setup_rules+=setup/debian
default_rules+=configure prepare all test check dist

# TODO: Keep aligned to latest debian stable
debian_codename?=bookworm

# packages/helper:
debian_packages+=make sudo git file
# packages/deps/build:
debian_packages+=build-essential
debian_packages+=cmake
# packages/deps/libs:
debian_packages+=libjson-c-dev
# packages/deps/more:
debian_packages+=gcovr

app_dir?=opt

# To be overloaded from env
BRANCH_NAME?=tmp/local/${USER}/main
SONAR_HOST_URL?=https://sonarcloud.io
SONAR_TOKEN?=${USER}

sonar_bw_url?=${SONAR_HOST_URL}/static/cpp/build-wrapper-linux-x86.zip
sonar_bw_file?=$(shell basename -- ${sonar_bw_url})
sonar_bw_app?=${app_dir}/build-wrapper-linux-x86/build-wrapper-linux-x86-64
sonar_bw_out_file?=${SONAR_OUT_DIR}/compile_commands.json

sonar_scanner_url?=https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-7.1.0.4889-linux-x64.zip
sonar_scanner_file?=$(shell basename -- ${sonar_scanner_url})
sonar_scanner_app?=${app_dir}/sonar-scanner-7.1.0.4889-linux-x64/bin/sonar-scanner

ifndef SONAR_OUT_DIR
gcovr?=gcovr
coverage_file?=${build_dir}/coverage.xml
sonar_bw_cmdline?=
COVERAGE?=ON
else
setup_rules+=sonar/setup
default_rules+=coverage sonar/deploy sonar/dist
gcovr?=gcovr --sonarqube
sonar_bw_cmdline?=${sonar_bw_app} --out-dir "${SONAR_OUT_DIR}"
coverage_file?=${SONAR_OUT_DIR}/coverage.xml
endif

ifdef COVERAGE
cmake_options+=-DCOVERAGE=${COVERAGE}
endif

# Allow overloading from env if needed
ifdef VERBOSE
CMAKE_VERBOSE_MAKEFILE?=${VERBOSE}
cmake_options+=-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
endif

ifdef BUILD_TESTING
cmake_options+=-DBUILD_TESTING=${BUILD_TESTING}
cmake_options+=-DSKIP_TESTING=${SKIP_TESTING}
endif

run_file?=${build_dir}/${project}
run_args?=--help


help: ./helper.mk
	@echo "# ${project}: ${url}"
	@echo "#"
	@echo "# Usage:"
	@echo "#  ${<D}/${<F} setup # To setup developer system (once)"
	@echo "#  ${<D}/${<F} VERBOSE=1 # Default build tasks verbosely (depends on setup)"
	@echo "#  ${<D}/${<F} help/all # For more info"
	@echo "#"

help/all: README.md
	@echo "# ${project}: ${url}"
	@echo ""
	@head $^
	@echo ""
	@echo "# Available helper.mk rules at your own risk:"
	@grep -o '^[^ ]*:' ${SELF} \
		| grep -v '\$$' | grep -v '^#' | grep -v '^\.' \
		| grep -v '=' | grep -v '%'
	@echo "#"
	@echo "# Environment:"
	@echo "#  PATH=${PATH}"
	@echo "#  version=${version}"
	@echo ""
	@echo ""

setup/debian:
	cat /etc/debian_version
	-${sudo} apt update
	${sudo} apt install -y ${debian_packages}

setup/debian/bookworm: setup/debian
	date -u

setup: ${setup_rules}
	date -u

${sonar_bw_app}:
	@echo "${project}: log: TODO: https://community.sonarsource.com/t/are-sonar-tools-versionned-on-public-sources/144031"
	mkdir -p "${@D}"
	cd "${app_dir}" \
	&& wget -c "${sonar_bw_url}" \
	&& unzip "${sonar_bw_file}"
	file -E "$@"

${sonar_scanner_app}:
	mkdir -p "${@D}"
	cd "${app_dir}" \
	&& wget -c "${sonar_scanner_url}" \
	&& unzip "${sonar_scanner_file}"
	file -E "$@"

sonar/configure: configure
	ls -l ${sonar_bw_out_file}

sonar/setup: ${sonar_bw_app} ${sonar_scanner_app}
	file -E $^

${SONAR_OUT_DIR}/compile_commands.json: coverage
	ls $@

sonar/dist: ${coverage_file} sonar-project.properties .scannerwork/report-task.txt
	@echo "${project}: log: %@: Files for sonar/deploy"
	ls -l "${sonar_bw_out_file}"
	ls -l "${coverage_file}"
	find "${build_dir}" -iname "*.gc*" | wc -l # Can not be processed outside build env
	tar cvfz "${SONAR_OUT_DIR}/${@F}.tar.gz" $^

.scannerwork/report-task.txt: ${sonar_scanner_app}
	ls "${sonar_bw_out_file}"
	ls "${coverage_file}"
	@echo "${project}: log: $@: Uploading data to ${SONAR_HOST_URL}"
	${<} \
		--define sonar.branch.name=${BRANCH_NAME} \
		--define sonar.cfamily.compile-commands="${sonar_bw_out_file}" \
		--define sonar.coverageReportPaths="${coverage_file}" \
		--define sonar.host.url="${SONAR_HOST_URL}" \
		--define sonar.token="${SONAR_TOKEN}" \
		--debug
	find .scannerwork -type f

sonar/deploy: .scannerwork/report-task.txt 
	@echo "${project}: log: TODO: https://community.sonarsource.com/t/new-github-action-for-c-docker-project/136307/5?u=rzr"
	ls -l $<

configure: ${build_dir}/CMakeCache.txt
	file -E $<

configure/clean:
	rm -rf ${build_dir}/CMake*

reconfigure: configure/clean configure
	@date -u

${build_dir}/CMakeCache.txt: CMakeLists.txt
	${sonar_bw_cmdline} ${cmake} ${cmake_options}
	ls -l $@

all: ${build_dir}/CMakeCache.txt
#	${sonar_bw_cmdline} ${cmake} --build ${<D} \
#		|| cat ${build_dir}/CMakeFiles/CMakeOutput.log
	${sonar_bw_cmdline} ${cmake} --build ${<D}

.PHONY: all

${build_dir}/%: all
	file -E "$@"

${build_dir}: ${build_dir}/CMakeCache.txt
	file -E "$<"

test: ${build_dir} all
	${ctest} --test-dir ${<}/${project_test_dir}

check: ${run_file}
	${<D}/${<F} --help

coverage: ${coverage_file}
	ls -l "$<"

${coverage_file}: test all
	${gcovr} --print-summary -o "$@"
	ls -l "$@"

dist/cmake: ${build_dir}
	${cmake} --build $< --target package

dist/deb: ${build_dir}
	${cmake} --build $< --target package
	mkdir -p ${<}/${@D}
	cp -av ${<}/*.deb ${<}/${@D}/

dist: dist/deb

distclean:
	rm -rf ${build_dir}

prepare:
	git --version
	${cmake} --version

all/default: ${default_rules}
	@date -u

run:
	file -E ${run_file}
	${run_file} ${run_args}
