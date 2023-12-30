# MultiEffect

## Description

Here's the code for MultiVersio for the Desmodus Versio platform. Build using Visual Studio Code.
Thanks to Emilie Gilliet (spectrings and many filters are derived from her wonderful work).
Thanks to the Daisy developers for their lovely platform. And thanks to Noise Engineering for their awesome products.

## More info at:

https://www.modwiggler.com/forum/viewtopic.php?t=249058

## Development

This repo is intended to be developed within a VS Code [devcontainer](https://code.visualstudio.com/docs/devcontainers/containers).   To maintain and / or build the code, first install VS Code and the Dev Containers extension, then:

1. Check out this repo and open it in VS Code.
2. Open the project in a container using `CMD-SHIFT-P` and selecting `Dev Containers: Open Folder in Container`.
3. Once the container builds and the environment opens, you can develop against the project.

### Building

To build the project, run:

```shell
make
```

### Generate documentation

`doxygen` is used for documentation.  To build locally, run:

```shell
make docs
```

Documentation is created in the `./docs/` folder.  Documentation can be viewed at [docs/html/index.html]( docs/html/index.html).

## Contribution

Feel free to update and improve the code! Please share your contributions so we can improve on this little thing :)

## Usage

Created by user Tinmaar159 on Modwiggler:
<img src="https://www.modwiggler.com/forum/download/file.php?id=127880&mode=view" style="width: 100%;"/>

## Author

- Paolo Ingraito - Initial implementation
- Pete Baker - Refactoring and maintenance