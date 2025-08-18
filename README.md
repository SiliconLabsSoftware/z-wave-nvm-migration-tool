# Z-Wave NVM Migration Tool
The Z-Wave NVM migration tool simplifies the process of reading an NVM3 image and converting it into a human-readable JSON format. It also provides functionality to upgrade NVM3 files to newer versions, facilitating controller migration while preserving Z-Wave network information.
****

## Build Instructions
### Testing Environment: 
- Linux system: (Debian-12, Ubuntu-22.04+ to 24.04+) on amd64 , armhf architecture
- Hardware: ZGM230S module (BRD4207A) + WSTK (BRD4002A)

> **_Note:_** The following commands are executed on the Ubuntu terminal.
### Install from release packages
Download the corresponding release package (.deb) for your platform from the [Release page](https://github.com/SiliconLabsSoftware/z-wave-nvm-migration-tool/releases).

Install the `.deb` package on your system using the following commands:
```sh
sudo apt-get update
sudo dpkg -i ./*.deb \
  || sudo apt install -f -y --no-install-recommends 
```

### Build from source
The project is CMake based, to prepare the environment, have a look at [./helper.mk](./helper.mk)'s details for needed steps to set up the developer system before using CMake normally.

At the moment stable version of Debian (12) is supported, so it should work also in other Debian based distros (Ubuntu, RaspiOS, WSL2 etc) and should be easy to adapt to other distributions.

**Requirements**:

Install dependencies
```sh
$ sudo apt install make git sudo
```
> Note: json-c requires cmake from v3.9. To build with "Debug mode", please edit 
```option(ENABLE_DEBUG "Debug Mode is ON/OFF" ON)```

**Download and build process**
- Download the `tar.gz` or the `.zip` file for your platform from the [Release page](https://github.com/SiliconLabsSoftware/z-wave-nvm-migration-tool/releases).
- Unzip the file
- Build via steps below

```sh
$ ./helper.mk setup
$ ./helper.mk VERBOSE=1 # Default build tasks verbosely (depends on setup)
```
An executable will be created in build folder. 
```
z-wave-nvm-migration-tool/
├── CHANGES.md
├── CMakeLists.txt
├── Dockerfile
├── LICENSE
├── README.md
├── build
├── cmake
├── extra
├── helper.mk
├── sonar-project.properties
├── tests
├── zw_nvm_converter
├── zwave_data_description_scheme.json
├── zwave_nvm_tool.c
└── zwave_nvm_tool_version.h
```
> **_Note:_** The pre-built binary is placed in pre_built folder.

> **_Note:_** `zwave_data_description_scheme.json` is the schema file containing description of all properties of Z-Wave objects that exist in an NVM3 file and is used for the migration process.

## Usage
```sh
Z-Wave NVM3 Migration Tool
Usage: ./z-wave-nvm-migration-tool [options] <arguments>
Options:
    -e  <nvm_file_input>
        Convert: READ an NVM3 file and output data in JSON format.
        Example: ./z-wave-nvm-migration-tool -e input.bin

    -i <json_file_input> <part_number>
        Convert: Parse data from a JSON file (aligned with schema) and generate an NVM3 file for the specified hardware.
        Supported parts: EFR32XG13, EFR32XG14, EFR32XG23, EFR32XG28
        Example: ./z-wave-nvm-migration-tool -i input.json EFR32XG23

    -u <json_file_input> <target_protocol_version> <schema_file>
        Upgrade: Upgrade a JSON data file to a newer version using a schema.
        If target_protocol_version == current_protocol_version: populate missing keys with default values.
        Example: ./z-wave-nvm-migration-tool -u input.json 7.19.0 zwave_data_description_scheme.json

    -m <nvm_file_input> <target_protocol_version> <part_number> <schema_file>
        Migration: Update an NVM3 file to a new version using a schema.
        If target_protocol_version == current_protocol_version: populate missing keys with default values.
        Example: ./z-wave-nvm-migration-tool -m input.bin 7.19.0 EFR32XG23 schema.json

    -o <output_file>
        Specify an output file; otherwise, the output file name will be generated automatically.
        This option can be used along with other options.
        Example: ./z-wave-nvm-migration-tool -e input.bin -o output.json

    -h
        Display this help message
```

## Some Specific Use Cases 
### Use case 1: Migrating NVM3 for restoring on a different controller
#### 1. Manually (to inspect the data in the NVM3 file):
- The NVM3 image `nvm_7_18.bin` is obtained from a controller running version 7.18.0 using either `commander` or the serialAPI tool `zw_programmer`. For details, see [Reading NVM3 image from controller](#reading-nvm3-image).

- To restore on a controller with version 7.22.0, an updated NVM3 image `nvm_7_22.bin` must be generated.

**Step 1**:	Read data from `nvm_7_18.bin` and save to json file

```sh
$ ./z-wave-nvm-migration-tool -e nvm_7_18.bin -o nvm_7_18.json
```
- The NVM3 data from Z-Wave SDK 7.18 (Filesys v4) will be saved in JSON format.

**Step 2**: Upgrading the NVM3 file to version 7.22.0:

```sh
$ ./z-wave-nvm-migration-tool -u nvm_7_18.json 7.22.0 zwave_data_description_scheme.json -o nvm_7_22_0.json
```
> **_Note:_** If the upgrade mode `-u` is used with the same version as the current protocol version, any missing objects for that version will be added to the JSON file according to the schema.

> **_Note:_** The `-o` option is required to specify the output filename (e.g., `nvm_7_22_0.json`). If option `-o` is omitted, the tool will automatically generate an output filename.

**Step 3**: Using the newly upgraded JSON data file (7.22.0) to generate an NVM3 file
```sh
$ ./z-wave-nvm-migration-tool -i nvm_7_22_0.json EFR32XG23 -o nvm_7_22_0.bin 
```
- NVM3 data file has been generated for hardware `EFR32XG23` 
> **_Note:_** Supported parts: EFR32XG13 and EFR32XG14 (Series 1); EFR32XG23 and EFR32XG28 (Series 2).
- This NVM3 data file can be flashed to other controllers using `commander` or serialAPI using `zw_programmer`

**Step 4**: Upgrade (OTW) the controller (EFR32XG23) to version 7.22.0 and upload/flash the newly upgraded NVM3 image. 
Refer to [Flashing NVM3 image to Controller](#flashing-nvm-image-to-controller)

#### 2. Migrate using `-m` option:
```sh
$ ./z-wave-nvm-migration-tool -m nvm_7_18.bin 7.22.0 EFR32XG23 zwave_data_description_scheme.json -o nvm_7_22_0.bin 
```

### Use case 2: Adding missing data for a version 7.21.0 for 700 Series (ZG13/ZGM130S/ZG14) 
NVM3 images generated by 700 Series controllers with firmware version 7.21.0 will lack the application data object. For instance, consider the file `nvm_7_21_0.bin`.

This file can be updated either by manual modification or by utilizing the `-u` option to add missing objects according to the schema.

#### 1. Manually (to inspect the data in the NVM3 file): 

**Step 1**:	Read data from `nvm_7_21_0.bin` and save to a JSON file

```sh
$ ./z-wave-nvm-migration-tool -e nvm_7_21_0.bin -o nvm_7_21_0.json
```

**Step 2**: Populate missing keys using the `-u` option:

```sh
$ ./z-wave-nvm-migration-tool -u nvm_7_21_0.json 7.21.0 zwave_data_description_scheme.json -o nvm_7_21_0_adding_missing_keys.json
```

**Step 3**: Using the newly upgraded JSON data file (7.21.0) to generate an NVM3 file
```sh
$ ./z-wave-nvm-migration-tool -i nvm_7_21_0_adding_missing_keys.json EFR32XG23 -o nvm_7_21_0_adding_missing_keys.bin 
```
- NVM3 data file has been generated for `EFR32XG23` 

#### 2. Using option `-m`: 
```sh
$ ./z-wave-nvm-migration-tool -m nvm_7_21_0.bin 7.21.0 EFR32XG13 zwave_data_description_scheme.json -o nvm_7_21_0_adding_missing_keys.bin 
```

## Reading NVM3 image 

### 1. Using Simplicity Commander
```sh
commander nvm3 read -o nvm.bin
```
### 2. Using zw_programmer
```sh
zw_programmer -s <serial_device> -r nvm.bin
```
## Flashing NVM3 image to Controller

> **_Note:_** The address of the NVM3 region differs by series:
- **Series 2 (800 Series):** `0x08074000`
- **Series 1 (700 Series):** `0x00074000`
```sh
# For 800 Series (Series 2)
commander flash nvm.bin --address=0x08074000
# For 700 Series (Series 1)
commander flash nvm.bin --address=0x00074000
```
### 2. Using zw_programmer
```sh
zw_programmer -s <serial_device> -w nvm.bin
```
