export type ProjectCategory = 'reference' | 'reader' | 'dashboard';

export interface ShowcaseProject {
  title: string;
  summary: string;
  category: ProjectCategory;
  categoryLabel: string;
  hardware: string;
  framework: string;
  license: string;
  repository: string;
  status: '可运行' | '开放征集';
  accent: string;
}

const submissionUrl =
  'https://github.com/exomind-team/openepd-47/issues/new?template=project-submission.yml';

export const projects: ShowcaseProject[] = [
  {
    title: 'OpenEPD 电子书参考工程',
    summary: '随仓库维护的 ESP-IDF 参考固件，覆盖墨水屏刷新、GT911 触摸与本地电子书阅读链路。',
    category: 'reference',
    categoryLabel: '参考工程',
    hardware: 'ESP32-S3 · E0470A01',
    framework: 'ESP-IDF',
    license: 'Apache-2.0',
    repository: 'https://github.com/exomind-team/openepd-47/tree/main/driver/reference',
    status: '可运行',
    accent: '#d56732',
  },
  {
    title: '开放席位：阅读终端',
    summary: '欢迎基于同类 4.7 英寸电子纸屏的阅读器、笔记工具或低功耗内容终端加入展示。',
    category: 'reader',
    categoryLabel: '阅读终端',
    hardware: '兼容硬件',
    framework: '不限',
    license: '开源许可',
    repository: submissionUrl,
    status: '开放征集',
    accent: '#39735a',
  },
  {
    title: '开放席位：信息面板',
    summary: '天气、日历、家庭状态或生产数据——让长续航的信息面板拥有一个公开的展示入口。',
    category: 'dashboard',
    categoryLabel: '信息面板',
    hardware: '兼容硬件',
    framework: '不限',
    license: '开源许可',
    repository: submissionUrl,
    status: '开放征集',
    accent: '#bc994d',
  },
];

export function validateProjects(items = projects): string[] {
  const errors: string[] = [];
  const requiredText: Array<keyof ShowcaseProject> = [
    'title',
    'summary',
    'category',
    'categoryLabel',
    'hardware',
    'framework',
    'license',
    'repository',
    'status',
    'accent',
  ];

  items.forEach((project, index) => {
    for (const field of requiredText) {
      if (typeof project[field] !== 'string' || project[field].trim() === '') {
        errors.push(`project[${index}].${field} is required / 为必填项`);
      }
    }

    try {
      const repository = new URL(project.repository);
      if (repository.protocol !== 'https:') {
        errors.push(`project[${index}].repository must use HTTPS / 必须使用 HTTPS`);
      }
    } catch {
      errors.push(`project[${index}].repository is invalid / URL 无效`);
    }
  });

  return errors;
}
