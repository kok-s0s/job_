#!/usr/bin/env bash
set -euo pipefail

UBUNTU_VERSION=$(lsb_release -rs)
case "${UBUNTU_VERSION}" in
  22.04)
    ROS_DISTRO_NAME=humble
    ;;
  24.04)
    ROS_DISTRO_NAME=jazzy
    ;;
  *)
    echo "Unsupported Ubuntu ${UBUNTU_VERSION}. Use Ubuntu 22.04 for ROS2 Humble or Ubuntu 24.04 for ROS2 Jazzy." >&2
    exit 1
    ;;
esac

sudo apt update
sudo apt install -y curl ca-certificates lsb-release software-properties-common
sudo add-apt-repository -y universe

ROS_APT_SOURCE_VERSION=$(
  curl -s https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest |
    grep -F "tag_name" |
    awk -F\" '{print $4}'
)

if [[ -z "${ROS_APT_SOURCE_VERSION}" ]]; then
  echo "Failed to resolve latest ros-apt-source release." >&2
  exit 1
fi

curl -L -o /tmp/ros2-apt-source.deb \
  "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/ros2-apt-source_${ROS_APT_SOURCE_VERSION}.$(. /etc/os-release && echo "${UBUNTU_CODENAME:-${VERSION_CODENAME}}")_all.deb"

sudo dpkg -i /tmp/ros2-apt-source.deb
sudo apt update
sudo apt install -y "ros-${ROS_DISTRO_NAME}-desktop" ros-dev-tools python3-colcon-common-extensions

SOURCE_LINE="source /opt/ros/${ROS_DISTRO_NAME}/setup.bash"
if ! grep -q "${SOURCE_LINE}" "${HOME}/.bashrc"; then
  echo "${SOURCE_LINE}" >>"${HOME}/.bashrc"
fi

# shellcheck disable=SC1090
set +u
source "/opt/ros/${ROS_DISTRO_NAME}/setup.bash"
set -u

echo "ROS_DISTRO=${ROS_DISTRO}"
command -v ros2
command -v colcon
echo "ROS2 ${ROS_DISTRO_NAME} setup finished. Open a new shell or run: ${SOURCE_LINE}"
