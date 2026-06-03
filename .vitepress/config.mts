import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: '求职准备',
    description: '具身智能 / 人形机器人方向 C++ 跳槽笔记',
    base: '/job_/',
    lang: 'zh-CN',

    head: [['link', { rel: 'icon', href: '/job_/favicon.ico' }]],

    themeConfig: {
      nav: [
        { text: '首页', link: '/' },
        { text: 'C++ 专项', link: '/cpp/memory_management' },
        { text: '学习路线', link: '/roadmap/roadmap' },
        { text: '项目计划', link: '/projects/' },
        { text: 'JD 汇总', link: '/jd/' },
      ],

      sidebar: {
        '/cpp/': [
          {
            text: 'C++ 面试专项',
            items: [
              { text: '内存管理 & RAII', link: '/cpp/memory_management' },
              { text: '移动语义', link: '/cpp/move_semantics' },
            ],
          },
        ],
        '/roadmap/': [
          {
            text: '规划',
            items: [{ text: '学习路线', link: '/roadmap/roadmap' }],
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
