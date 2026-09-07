#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

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

std::vector<std::string> captureNames(const std::string& text, const std::regex& pattern) {
    std::vector<std::string> names;
    for (auto it = std::sregex_iterator(text.begin(), text.end(), pattern); it != std::sregex_iterator(); ++it) {
        names.push_back((*it)[1].str());
    }
    return names;
}

bool contains(const std::vector<std::string>& values, const std::string& expected) {
    for (const auto& value : values) {
        if (value == expected) {
            return true;
        }
    }
    return false;
}

void requireText(const std::string& text, const std::string& expected) {
    if (text.find(expected) == std::string::npos) {
        throw std::runtime_error("missing URDF field: " + expected);
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "models/two_joint_arm.urdf";
        const auto urdf = readFile(path);

        requireText(urdf, "<robot name=\"two_joint_arm\">");
        const auto links = captureNames(urdf, std::regex("<link\\s+name=\"([^\"]+)\""));
        const auto joints = captureNames(urdf, std::regex("<joint\\s+name=\"([^\"]+)\""));

        for (const auto& link : {"base_link", "shoulder_link", "elbow_link"}) {
            if (!contains(links, link)) {
                throw std::runtime_error(std::string("missing link: ") + link);
            }
        }

        for (const auto& joint : {"shoulder_yaw_joint", "elbow_pitch_joint"}) {
            if (!contains(joints, joint)) {
                throw std::runtime_error(std::string("missing joint: ") + joint);
            }
        }

        requireText(urdf, "<parent link=\"base_link\"/>");
        requireText(urdf, "<child link=\"shoulder_link\"/>");
        requireText(urdf, "<parent link=\"shoulder_link\"/>");
        requireText(urdf, "<child link=\"elbow_link\"/>");
        requireText(urdf, "<axis xyz=\"0 0 1\"/>");
        requireText(urdf, "<axis xyz=\"0 1 0\"/>");
        requireText(urdf, "<limit lower=\"-1.57\" upper=\"1.57\" effort=\"30\" velocity=\"1.2\"/>");
        requireText(urdf, "<limit lower=\"-1.20\" upper=\"1.20\" effort=\"20\" velocity=\"1.5\"/>");

        std::cout << "[urdf] robot=two_joint_arm links=" << links.size() << " joints=" << joints.size() << "\n";
        std::cout << "[urdf_link] base_link shoulder_link elbow_link\n";
        std::cout << "[urdf_joint] shoulder_yaw_joint parent=base_link child=shoulder_link axis=0,0,1\n";
        std::cout << "[urdf_joint] elbow_pitch_joint parent=shoulder_link child=elbow_link axis=0,1,0\n";
        std::cout << "[ok] two-joint URDF structure verified\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
