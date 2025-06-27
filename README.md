# Z-WAVE NVM TOOL 
The Z-Wave NVM3 tool simplifies the process of reading NVM3 data and converting it into a human-readable JSON format. It also provides functionality to upgrade NVM files to newer versions, facilitating controller migration while preserving Z-Wave network information.
****

## Build Instructions
### Testing Environment: 
- PC: Ubuntu 18.04.5 LTS (Bionic Beaver) and 22.04.1 LTS (Jammy Jellyfish)
- Hardware: ZGM230S module (BRD4207A) + WSTK (BRD4002A)

> **_Note:_** The following commands are executed on the Ubuntu terminal.
### Installation
**Requirements**: 
Install dependencies 
```sh
$ sudo apt-get install cmake build-essential 
```
Install json-c library:
```sh
$ git clone https://github.com/json-c/json-c.git
$ cd json-c
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```
> Note: json-c requires cmake from v3.9. To build with "Debug mode", please edit 
```option(ENABLE_DEBUG "Debug Mode is ON/OFF" ON)```

**Download and build process**
- Download the .zip file to your computer
- Unzip the file
- Build via steps below
```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
```
An executable will be created in build folder. 
```
z-wave-nvm-migration-tool/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── doc
├── zw_nvm_converter
├── zwave_data_description_scheme.json
└── zwave_nvm_tool.c
```
> **_Note:_** The pre-built binary is placed in pre_built folder.

> **_Note:_** `zwave_data_description_scheme.json` is the schema file containing description of all properties of Z-Wave objects that exist in a NVM file and is used for the migration process.

## Usage
Run following command to show all command and usage
```sh
./zwave_nvm_tool -h 
Usage: ./zwave_nvm_tool [options] <arguments>

Options:
    -e  <nvm_file_input>
        Convert: READ an NVM file and output data in JSON format.
        Example: ./zwave_nvm_tool -e input.bin 

    -i <json_file_input> <part_number>
        Convert: Parse data from a JSON file (aligned with schema) and generate an NVM file for the specified hardware.
        Supported parts: EFR32XG13, EFR32XG14, EFR32XG23, EFR32XG28
        Example: ./zwave_nvm_tool -i input.json EFR32XG23 

    -u <json_file_input> <target_protocol_version> <schema_file>
        Upgrade: Upgrade a JSON data file to a newer version using a schema.
        If target_protocol_version == current_protocol_version: populate missing keys with default values.
        Example: ./zwave_nvm_tool -u input.json 7.19.0 zwave_data_description_scheme.json

    -m <nvm_file_input> <target_protocol_version> <part_number> <schema_file>
        Migration: Update an NVM file to a new version using a schema.
        If target_protocol_version == current_protocol_version: populate missing keys with default values.
        Example: ./zwave_nvm_tool -m input.bin 7.19.0 EFR32XG23 schema.json

    -o <output_file>
        Specify an output file; otherwise, the output file name will be generated automatically.
        This option can be used along with other options.
        Example: ./zwave_nvm_tool -e input.bin -o output.json

    -h
        Display this help message.
```

## Some Specific Use Cases 
### Use case #1: Migrating NVM for restoring on a different controller
#### 1. Manually with each step (only for understanding how the tool works):
- Assume that we have the NVM image `nvm_7_18.bin` that is read from one controller using either `commander` or serialAPI using `zw_programmer`

- And we want to generate a NVM image for version 7.22 `nvm_7_22.bin` to restore on another controller without losing Z-Wave network information.

Step 1:	Read data from `nvm_7_18.bin` and save to json file

```sh
$ ./zwave_nvm_tool -e nvm_7_18.bin -o nvm_7_18.json
```
- NVM3 data of Z-Wave SDK 7.18 (Filesys v4) will be saved as JSON format.

Step 2: Upgrading the NVM file to version 7.22.0:

```sh
$ ./zwave_nvm_tool -u nvm_7_18.json 7.22.0 zwave_data_description_scheme.json -o nvm_7_22_0.json
```
> **_Note:_** If using the mode upgrade `-u` for the same version, missing object for that version will be added to the json file.

Step 3: Using the newly upgraded JSON data file (7.22.0) to generate a NVM file
```sh
$ ./zwave_nvm_tool -i nvm_7_22_0.json EFR32XG23 -o nvm_7_22_0.bin 
```
- NVM3 data file has been generated for hardware `EFR32XG23` 
- This NVM3 data file can be flash to other controllers using `commander` or serialAPI using `zw_programmer`

#### 2. Migrate using `-m` option:
```sh
$ ./zwave_nvm_tool -m nvm_7_18.bin 7.22.0 EFR32XG23 zwave_data_description_scheme.json -o nvm_7_22_0.bin 
```

### Use case 2: Adding missing data for a version 7.21.0 for 700 Series (XG13/XG14) 
For 700 Series with 7.21.0, application data object is missing, assume we have `nvm_7_21_0.bin` that is missing application data object.

We can either modify it manually or using option `-u` to update it aligned with the schema format.

#### 1. Manually (if you want to inspect the data in the NVM file): 

Step 1:	Read data from `nvm_7_21_0.bin` and save to json file

```sh
$ ./zwave_nvm_tool -e nvm_7_21_0.bin -o nvm_7_21_0.json
```

Step 2: Populating json file with missing objects using option `-u`:

```sh
$ ./zwave_nvm_tool -u nvm_7_21_0.json 7.21.0 zwave_data_description_scheme.json -o nvm_7_21_0_adding_missing_keys.json
```

Step 3: Using the newly upgraded JSON data file (7.21.0) to generate a NVM file
```sh
$ ./zwave_nvm_tool -i nvm_7_21_0_adding_missing_keys.json EFR32XG23 -o nvm_7_21_0_adding_missing_keys.bin 
```
- NVM3 data file has been generated for `EFR32XG23` 

#### 2. Using option `-m`: 
```sh
$ ./zwave_nvm_tool -m nvm_7_21_0.bin 7.21.0 EFR32XG13 zwave_data_description_scheme.json -o nvm_7_21_0_adding_missing_keys.bin 
```

## Reading NVM image 

### 1. Using Simplicity Commander
```sh
commander nvm3 read -o nvm.bin
```
### 2. Using zw_programmer
```sh
zw_programmer -s <serial_device> -r nvm.bin
```
## Flashing NVM image to Controller

### 1. Using Simplicity Commander
```sh
# For 800 Series
commander flash nvm.bin --address=0x08074000
# For 700 Series
commander flash nvm.bin --address=0x00074000
```
### 2. Using zw_programmer
```sh
zw_programmer -s <serial_device> -w nvm.bin
```