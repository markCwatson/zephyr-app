# Use Zephyr's pre-built image (baed on Ubuntu Linux)
# see https://github.com/zephyrproject-rtos/docker-image
FROM ghcr.io/zephyrproject-rtos/ci:v0.26.5

# Set the working directory
WORKDIR /usr/src/app

# Copy the current directory contents into the container
COPY . .

# Run setup script
RUN chmod +x ./scripts/pipeline-setup.py && ./scripts/pipeline-setup.py

# Set an entrypoint to keep the container running and allow exec
ENTRYPOINT ["tail", "-f", "/dev/null"]
