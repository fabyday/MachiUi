import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

const site = process.env.SITE ?? 'https://fabyday.github.io';
const base = process.env.BASE ?? '/MachiUi';

export default defineConfig({
  site,
  base,
  trailingSlash: 'always',
  integrations: [
    starlight({
      title: 'MachiUI',
      description: 'Native C++ UI engine with a React-style JavaScript runtime.',
      defaultLocale: 'root',
      lastUpdated: true,
      social: [
        {
          icon: 'github',
          label: 'GitHub',
          href: 'https://github.com/fabyday/MachiUi',
        },
      ],
      editLink: {
        baseUrl: 'https://github.com/fabyday/MachiUi/edit/main/docs/',
      },
    }),
  ],
});
