#include <iostream>
#include <string>
#include <vector>

namespace {

struct TransformEdge {
    std::string parent;
    std::string child;
    std::string xyz;
    std::string rpy;
};

}  // namespace

int main() {
    const std::vector<TransformEdge> edges{
        {"base_link", "shoulder_link", "0 0 0.12", "0 0 0"},
        {"shoulder_link", "elbow_link", "0.50 0 0", "0 0 0"},
        {"elbow_link", "tool0", "0.40 0 0", "0 0 0"},
        {"elbow_link", "imu_link", "0.25 0 0.08", "0 0 0"},
    };

    std::cout << "[tf_tree] root=base_link frames=5 edges=" << edges.size() << "\n";
    for (const auto& edge : edges) {
        std::cout << "[tf_edge] parent=" << edge.parent
                  << " child=" << edge.child
                  << " xyz=\"" << edge.xyz
                  << "\" rpy=\"" << edge.rpy << "\"\n";
    }

    std::cout << "[tf2_echo] command=\"ros2 run tf2_ros tf2_echo base_link tool0\"\n";
    std::cout << "[tf2_echo] command=\"ros2 run tf2_ros tf2_echo base_link imu_link\"\n";
    std::cout << "[ok] TF2 frame tree review ready\n";
    return 0;
}
