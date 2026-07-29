import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://exomind-team.github.io',
  base: '/openepd-47',
  integrations: [
    starlight({
      title: 'OpenEPD 4.7',
      description: '4.7 英寸电子纸的开放资料、驱动与社区项目。',
      favicon: '/openepd-47/favicon.svg',
      customCss: ['./src/styles/starlight.css'],
      social: [
        {
          icon: 'github',
          label: 'GitHub',
          href: 'https://github.com/exomind-team/openepd-47',
        },
      ],
      editLink: {
        baseUrl: 'https://github.com/exomind-team/openepd-47/edit/main/website/',
      },
      sidebar: [
        {
          label: '开始开发',
          items: [
            { label: '快速开始', slug: 'quick-start' },
            { label: '构建环境', slug: 'driver/build' },
          ],
        },
        {
          label: '硬件',
          items: [
            { label: '显示屏', slug: 'hardware/display' },
            { label: '接口与引脚', slug: 'hardware/pinout' },
            { label: 'GT911 触摸', slug: 'hardware/gt911' },
            { label: 'TPS65185 电源', slug: 'hardware/power' },
          ],
        },
        {
          label: '资源',
          items: [
            { label: '资料下载', slug: 'resources' },
            { label: '电子书参考工程', slug: 'guides/ebook' },
            { label: '常见问题', slug: 'faq' },
          ],
        },
      ],
    }),
  ],
});
