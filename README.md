[![C++ CI](https://github.com/markCwatson/zephyr-app/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/markCwatson/zephyr-app/actions/workflows/main.yml)

# Example Application for Zephyr RTOS

## Summary

This is a sample application based on Nordic's [peripheral_uart](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/bluetooth/peripheral_uart/README.html) sample app. It has been converted to C++ and consists of a multi-threadded architecture utilizing zephyr's kernel pipe to send data between the UART and NUS threads.

"The Peripheral UART sample demonstrates how to use the Nordic UART Service (NUS). It uses the NUS service to send data back and forth between a UART connection and a Bluetooth® LE connection."

path to Nordic's original sample app: nrf/samples/bluetooth/peripheral_uart

Another great example app is the [ncs-example-application](https://github.com/nrfconnect/ncs-example-application/tree/main).

## Getting started

Before getting started, make sure you have a proper nRF Connect SDK development environment.
Follow the official
[Installation guide](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/getting_started.html). I used the CLI method.

Next, create a directory to serve as your West workspace.

```shell
mkdir zephyr-app-workspace
```

### Get source code

Clone the repository using SSH into the workspace.

```shell
cd zephyr-app-workspace
git clone git@github.com:markCwatson/zephyr-app.git
```

### Initialization (West)

The first step is to initialize the workspace folder (``zephyr-app-workspace``) where
the application and all nRF Connect SDK modules will be cloned. Run the following
command:

```shell
west init -l zephyr-app
cd zephyr-app
west update
```

This operation can take some time to complete as it fetches all of the external modules including the Zephyr RTOS and the Nordic nRF SDK.

### Setup and activate the virtual python environment (optional)

To avoid issues related to python packages across your system, you can create a virtual python environment and activate it

```shell
python3 -m venv ./.venv
source ./.venv/bin/activate
```

### Building and running

To build the application, run the following command:

```shell
west build -b my_custom_board app
```
or

```shell
cd app
west build -b my_custom_board
```

my_custom_board is the custom board definition, located under ./zephyr-app/boards/arm/my_custom_board.

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b my_custom_board app -- -DOVERLAY_CONFIG=debug.conf
```

If the build was successful, you should see a summary of memory usage such as in the following 

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       20924 B         1 MB      2.00%
             RAM:        6348 B       256 KB      2.42%
        IDT_LIST:          0 GB         2 KB      0.00%
```

### Flashing

Once you have built the application, run the following command to flash it:

```shell
west flash
```

### Testing

Zephyr uses [Twister](https://docs.zephyrproject.org/3.3.0/develop/test/twister.html) as the test runner. Twister is only supported on Linux and Windows. Thus, MacOS users can build and run a [Docker](https://docs.docker.com/engine/install/) container that is setup with this environment. This environment is setup the same as the GitHub pipeline to ensure consistency across testing environments. Linux/Windows users can also use this approach to ensure consistency as well.

In the root of the app (i.e. in ``zephyr-app`` folder at the same level as the Dockerfile) build the docker image

```shell
docker build --no-cache -t app . 
```

You can execute the command ``docker image ls`` to confirm the ``app`` image was successfully created.

```
mark@MacBook-Pro zephyr-app % docker images
REPOSITORY   TAG      IMAGE ID       CREATED         SIZE
app          latest   dedf29654791   7 minutes ago   15GB
```

Run the image in a container named `app-container` in detached mode:

```shell
docker run -v ./app:/usr/src/app/app -d --name app-container app
```

The `-v` flag will setup a volume based on `./app` on your local machine. In the docker container, the volume will be mounted to /usr/src/app/app. This means when you change files in the `./app` folder on your local machine, these chnages will be reflected in the docker container as well allowing you to make changes locally and build/test in the container.

You should be able to see the runnning process using ``docker ps``.

```
mark@MacBook-Pro zephyr-app % docker ps
CONTAINER ID   IMAGE  COMMAND               CREATED         STATUS         PORTS   NAMES
29eafa2e1f06   app    "tail -f /dev/null"   4 seconds ago   Up 3 seconds           app-container
```

Now you can attach a shell to the running container and build the app or run tests.

```shell
docker exec -it app-container sh
west build -b my_custom_board app -p
west twister -T app/tests --integration
```

[Note: On Windows, you may need to use `winpty docker exec -it app-container sh`]

You can also specify the target platform. The example used here is [native_posix_64](https://docs.zephyrproject.org/latest/boards/posix/native_posix/doc/index.html) which is hardware agnostic.

```shell
west twister -T app/tests -p native_posix_64
```

You can get a list of the testcases registered as part of the project using

```shell
west twister --list-tests -T app/tests
```

To stop and delete the container and delete the image:

```shell
docker stop app-container &&
docker rm app-container &&
docker rmi app
```