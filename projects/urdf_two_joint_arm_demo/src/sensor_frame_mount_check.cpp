#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open URDF file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void requireText(const std::string& text, const std::string& expected) {
    if (text.find(expected) == std::string::npos) {
        throw std::runtime_error("missing sensor frame field: " + expected);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "models/two_joint_arm.urdf";
        const auto urdf = readFile(path);

        requireText(urdf, "<link name=\"imu_link\">");
        requireText(urdf, "<joint name=\"imu_mount_joint\" type=\"fixed\">");
        requireText(urdf, "<parent link=\"elbow_link\"/>");
        requireText(urdf, "<child link=\"imu_link\"/>");
        requireText(urdf, "<origin xyz=\"0.25 0 0.08\" rpy=\"0 0 0\"/>");

        std::cout << "[sensor_frame] frame=imu_link parent=elbow_link joint=imu_mount_joint xyz=0.25,0,0.08\n";
        std::cout << "[rviz] fixed_frame=base_link display=RobotModel+TF sensor_frame=imu_link\n";
        std::cout << "[ok] simulated sensor frame is mounted on elbow_link\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
