import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: '求职准备',
    description: '具身智能 / 人形机器人方向 C++ 跳槽笔记',
    lang: 'zh-CN',
    base: '/job_/',

    ignoreDeadLinks: true,
    srcExclude: [
      'practice/ai_components.md',
      'practice/ai_sdk.md',
      'practice/browser_inference.md',
      'practice/function_calling.md',
      'practice/rag_frontend.md',
      'projects/desktop_ai_companion.md',
      'projects/music_visualizer.md',
      'projects/music_visualizer/**/*.md',
    ],

    head: [['link', { rel: 'icon', type: 'image/svg+xml', href: '/job_/favicon.svg' }]],

    vite: {
      ssr: {
        noExternal: ['vitepress-plugin-mermaid', 'mermaid'],
      },
    },

    themeConfig: {
      nav: [
        { text: '首页', link: '/' },
        { text: '学习计划', link: '/roadmap/weekday_2h_plan' },
        { text: '实战项目', link: '/projects/' },
        {
          text: 'JD 分析',
          items: [
            { text: '岗位汇总', link: '/jd/' },
            { text: '主投 JD-009', link: '/jd/jd_009_robot_software_system_engineer' },
            { text: '目标公司', link: '/companies/' },
          ],
        },
        {
          text: '核心笔记',
          items: [
            { text: 'C++ 内存管理', link: '/cpp/memory_management' },
            { text: 'C++ 并发', link: '/cpp/concurrency' },
            { text: 'Linux 系统编程', link: '/cpp/linux_system' },
            { text: 'ROS2 基础', link: '/cpp/ros2_basics' },
            { text: 'IPC / DDS 前置', link: '/cpp/ipc' },
            { text: 'Qt 与 QML', link: '/cpp/qt_and_qml' },
            { text: '基础练习', link: '/practice/' },
          ],
        },
        { text: '简历生成器', link: '/resume.html', target: '_blank' },
      ],

      sidebar: {
        '/cpp/': [
          {
            text: '核心笔记',
            items: [
              { text: '内存管理 & RAII', link: '/cpp/memory_management' },
              { text: '移动语义', link: '/cpp/move_semantics' },
              { text: '并发', link: '/cpp/concurrency' },
              { text: '虚函数', link: '/cpp/virtual_functions' },
              { text: '模板', link: '/cpp/templates' },
              { text: '性能优化', link: '/cpp/performance' },
              { text: '网络编程', link: '/cpp/network_programming' },
              { text: '进程间通信 IPC', link: '/cpp/ipc' },
              { text: '编译、链接与 CMake', link: '/cpp/build_and_cmake' },
              { text: '调试工具', link: '/cpp/debugging' },
              { text: 'Qt 与 QML', link: '/cpp/qt_and_qml' },
              { text: 'ROS2 基础', link: '/cpp/ros2_basics' },
              { text: 'Python 速成', link: '/cpp/python_for_cpp_dev' },
              { text: 'Linux 系统编程', link: '/cpp/linux_system' },
              { text: '传感器基础', link: '/cpp/sensors' },
            ],
          },
        ],
        '/roadmap/': [
          {
            text: '学习计划',
            items: [
              { text: '工作日 2 小时计划', link: '/roadmap/weekday_2h_plan' },
              { text: '每日练习记录', link: '/roadmap/daily' },
              { text: '总路线', link: '/roadmap/roadmap' },
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
              { text: 'JD-009 机器人软件系统开发', link: '/jd/jd_009_robot_software_system_engineer' },
            ],
          },
        ],
        '/practice/': [
          {
            text: '机器人系统练习',
            items: [
              { text: '总览', link: '/practice/' },
              { text: '01 Pub/Sub 模型（Python）', link: '/practice/pubsub_model' },
              { text: '02 ROS2 三节点 Demo', link: '/practice/ros2_three_nodes' },
              { text: '03 Service & Action', link: '/practice/ros2_service_action' },
              { text: '08 状态机（机械臂场景）', link: '/practice/state_machine' },
              { text: '09 线程安全队列（MPMC）', link: '/practice/blocking_queue' },
              { text: '10 共享内存 IPC', link: '/practice/shm_ipc' },
              { text: '11 Observer 模式', link: '/practice/observer' },
              { text: '12 RoboMon（落地项目）', link: '/practice/robocon' },
            ],
          },
        ],
        '/companies/': [
          {
            text: '目标公司',
            items: [
              { text: '总览（沪 / 深 / 穗）', link: '/companies/' },
            ],
          },
        ],
        '/projects/': [
          {
            text: '实战项目',
            items: [
              { text: '阶段规划总览', link: '/projects/' },
              { text: 'ROS2 Runtime Demo', link: '/projects/ros2_runtime_demo' },
            ],
          },
          {
            text: '简历级项目',
            items: [
              { text: '方向 A：机械臂抓取仿真（第 9-12 周）', link: '/projects/arm_grasp_sim' },
              { text: '方向 B：ROKAE SDK × ROS2 项目', link: '/projects/rokae_ros2_sdk_apps' },
            ],
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
