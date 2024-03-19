# VIGAS

Port of the Genesis-Plus-GX project (https://github.com/ekeeke/Genesis-Plus-GX) in C++, starting at commit 6972a5b0d69f633e5ee21d97c9b1068f7147c7db.

## Building Windows

### Requirements

* Git
* [Visual Studio 2022 Community or Visual Studio 2019 Community](https://www.visualstudio.com/downloads/)
* [Cmake 3.15 or later](https://cmake.org/download/)
* Windows 11 SDK

### Build

#### 1. Clone the repository
```
git clone https://github.com/christophemaymard/vigas.git
```

#### 2. Move to the repository
```
cd vigas
```

#### 3. Initialize the repository
```
git submodule update --init
```

#### 4. Generate the project buildsystem in the `build` directory

For Visual Studio 2022 Community:
```
cmake -B build -G "Visual Studio 17 2022" -A x64
```

For Visual Studio 2019 Community:
```
cmake -B build -G "Visual Studio 16 2019" -A x64
```

#### 5. Build the project

Open the Visual Studio solution `vigas.sln` (in the `build` directory).

Right-click on the `vigas` project and click on *Build*.

The program should be in a folder in `build/bin`, the name of the folder depends on the choosen configuration (*Debug*, *MinSizeRel*, *Release* or *RelWithDebInfo*).
