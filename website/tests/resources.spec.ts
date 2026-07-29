import { expect, test } from '@playwright/test';

test('资料中心区分仓库下载与外部来源', async ({ page }) => {
  await page.goto('./resources/');

  await expect(page.getByRole('heading', { level: 1, name: '资料中心' })).toBeVisible();
  await expect(page.getByTestId('resource-item')).toHaveCount(5);

  const displaySpec = page.getByTestId('resource-item').filter({ hasText: 'E0470A01-AF-S' });
  await expect(displaySpec.getByRole('link', { name: '仓库下载' })).toHaveAttribute(
    'href',
    /resources\/vendor\/display\/E0470A01-AF-S-specification\.pdf$/,
  );

  const gt911 = page.getByTestId('resource-item').filter({ hasText: 'GT911' });
  await expect(gt911.getByRole('link', { name: '查看外部来源' })).toHaveAttribute(
    'href',
    /^https:\/\/www\.fortec-integrated\.de\//,
  );
});

test('资料中心在移动端保持单列可读', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.goto('./resources/');

  const first = page.getByTestId('resource-item').first();
  await expect(first).toBeVisible();
  await expect(first).toHaveCSS('overflow-x', 'visible');
});
