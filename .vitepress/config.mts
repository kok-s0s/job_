import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: '求职准备',
    description: '具身智能 / 人形机器人方向 C++ 跳槽笔记',
    lang: 'zh-CN',

    head: [['link', { rel: 'icon', type: 'image/svg+xml', href: '/favicon.svg' }]],

    themeConfig: {
      nav: [
        { text: '首页', link: '/' },
        { text: 'C++ 专项', link: '/cpp/memory_management' },
        { text: '动手实践', link: '/practice/pubsub_model' },
        { text: '学习路线', link: '/roadmap/roadmap' },
        { text: '24k 目标', link: '/roadmap/salary_target' },
        { text: '项目计划', link: '/projects/' },
        { text: 'JD 汇总', link: '/jd/' },
        { text: '简历生成器', link: '/resume.html', target: '_blank' },
      ],

      sidebar: {
        '/cpp/': [
          {
            text: 'C++ 面试专项',
            items: [
              { text: '内存管理 & RAII', link: '/cpp/memory_management' },
              { text: '移动语义', link: '/cpp/move_semantics' },
              { text: '并发', link: '/cpp/concurrency' },
              { text: '虚函数', link: '/cpp/virtual_functions' },
              { text: '模板', link: '/cpp/templates' },
              { text: '性能优化', link: '/cpp/performance' },
              { text: '设计模式', link: '/cpp/design_patterns' },
              { text: 'Lambda 与函数式', link: '/cpp/lambda_and_functional' },
              { text: 'STL 容器', link: '/cpp/stl_containers' },
              { text: '网络编程', link: '/cpp/network_programming' },
              { text: '进程间通信 IPC', link: '/cpp/ipc' },
              { text: '编译、链接与 CMake', link: '/cpp/build_and_cmake' },
              { text: '调试工具', link: '/cpp/debugging' },
              { text: '数据结构与算法', link: '/cpp/algorithms' },
              { text: 'Qt 与 QML', link: '/cpp/qt_and_qml' },
              { text: 'ROS2 基础', link: '/cpp/ros2_basics' },
              { text: 'Python 速成', link: '/cpp/python_for_cpp_dev' },
              { text: 'Linux 系统编程', link: '/cpp/linux_system' },
              { text: 'OpenCV 基础', link: '/cpp/opencv_basics' },
              { text: '传感器基础', link: '/cpp/sensors' },
              { text: 'Git 工作流', link: '/cpp/git_workflow' },
            ],
          },
        ],
        '/roadmap/': [
          {
            text: '规划',
            items: [
              { text: '学习路线', link: '/roadmap/roadmap' },
              { text: '24k 目标 & 知识储备', link: '/roadmap/salary_target' },
            ],
          },
        ],
        '/jd/': [
          {
            text: 'JD 存档',
            items: [
              { text: '汇总对比', link: '/jd/' },
              { text: 'JD-001 京东具身智能', link: '/jd/jd_001_cpp_embodied_intelligence' },
              { text: 'JD-002 上位机软件工程师', link: '/jd/jd_002_software_engineer_upper_computer' },
              { text: 'JD-003 Qt 工业智能平台', link: '/jd/jd_003_cpp_qt_industrial_platform' },
              { text: 'JD-004 Qt 主管·人形机器人', link: '/jd/jd_004_qt_supervisor_humanoid_robot' },
              { text: 'JD-005 视觉运控工程师', link: '/jd/jd_005_cpp_software_engineer_vision' },
              { text: 'JD-006 后端 Java/Go', link: '/jd/jd_006_backend_java_go' },
              { text: 'JD-007 Linux 应用工程师', link: '/jd/jd_007_linux_application_engineer' },
              { text: 'JD-008 机器人软件工程师', link: '/jd/jd_008_robot_software_embodied' },
            ],
          },
        ],
        '/practice/': [
          {
            text: '动手实践',
            items: [
              { text: 'Pub/Sub 模型', link: '/practice/pubsub_model' },
            ],
          },
        ],
        '/projects/': [
          {
            text: '项目',
            items: [{ text: '项目计划', link: '/projects/' }],
          },
        ],
      },

      socialLinks: [
        { icon: 'github', link: 'https://github.com/kok-s0s/job_' },
      ],

      footer: {
        message: '具身智能 C++ 跳槽准备',
      },

      search: { provider: 'local' },

      outline: { label: '本页目录', level: [2, 3] },
    },

    mermaid: {},
  })
)
