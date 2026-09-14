#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("failed to open world file: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void requireText(const std::string& text, const std::string& expected) {
    if (text.find(expected) == std::string::npos) {
        throw std::runtime_error("missing SDF field: " + expected);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "worlds/two_joint_arm.world";
        const auto sdf = readFile(path);

        requireText(sdf, "<sdf version=\"1.6\">");
        requireText(sdf, "<world name=\"two_joint_arm_smoke_world\">");
        requireText(sdf, "<uri>model://sun</uri>");
        requireText(sdf, "<uri>model://ground_plane</uri>");
        requireText(sdf, "<model name=\"two_joint_arm_placeholder\">");
        requireText(sdf, "<link name=\"base_link\">");
        requireText(sdf, "<link name=\"arm_link\">");
        requireText(sdf, "<joint name=\"base_to_arm_fixed\" type=\"fixed\">");
        requireText(sdf, "<pose>0 0 0 0 0 0</pose>");

        std::cout << "[gazebo_world] world=two_joint_arm_smoke_world includes=sun,ground_plane model=two_joint_arm_placeholder\n";
        std::cout << "[gazebo_model] links=base_link,arm_link joint=base_to_arm_fixed pose=0,0,0,0,0,0\n";
        std::cout << "[gazebo_launch] command=\"gazebo worlds/two_joint_arm.world\"\n";
        std::cout << "[ok] Gazebo model smoke world structure verified\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
